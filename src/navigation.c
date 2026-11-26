#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include "navigation.h"

uint32_t get_fat_entry(FILE *fp, const Navigation *nav, uint32_t cluster) {
    unsigned char entry[4];
    long offset = (long)nav->first_fat_sector * nav->bytes_per_sector +
                  (long)cluster * 4;

    if (fseek(fp, offset, SEEK_SET) != 0 ||
        fread(entry, 1, sizeof(entry), fp) != sizeof(entry)) {
        return 0;
    }

    return ((uint32_t)entry[0] |
            (uint32_t)entry[1] << 8 |
            (uint32_t)entry[2] << 16 |
            (uint32_t)entry[3] << 24) & 0x0FFFFFFF;
}

static void get_short_name(const unsigned char *entry, char name[13]) {
    int length = 0;

    for (int i = 0; i < 8 && entry[i] != ' '; i++) {
        name[length++] = entry[i];
    }
    if (entry[8] != ' ') {
        name[length++] = '.';
        for (int i = 8; i < 11 && entry[i] != ' '; i++) {
            name[length++] = entry[i];
        }
    }

    name[length] = '\0';
}

static int names_match(const char *first, const char *second) {
    while (*first != '\0' && *second != '\0') {
        if (toupper((unsigned char)*first) != toupper((unsigned char)*second)) {
            return 0;
        }
        first++;
        second++;
    }

    return *first == *second;
}

int init_navigation(FILE *fp, Navigation *nav) {
    unsigned char boot[512];

    if (fseek(fp, 0, SEEK_SET) != 0) {
        return 0;
    }
    if (fread(boot, 1, sizeof(boot), fp) != sizeof(boot)) {
        return 0;
    }

    uint16_t reserved_sectors = *(uint16_t *)(boot + 14);

    nav->bytes_per_sector = *(uint16_t *)(boot + 11);
    nav->sectors_per_cluster = *(uint8_t *)(boot + 13);
    nav->number_of_fats = *(uint8_t *)(boot + 16);
    nav->fat_size_in_sectors = *(uint32_t *)(boot + 36);
    nav->root_cluster = *(uint32_t *)(boot + 44);
    nav->current_cluster = nav->root_cluster;
    nav->first_fat_sector = reserved_sectors;
    nav->first_data_sector =
        reserved_sectors + nav->number_of_fats * nav->fat_size_in_sectors;

    uint32_t total_sectors = *(uint32_t *)(boot + 32);
    uint32_t data_sectors =
        total_sectors - nav->first_data_sector;
    nav->total_clusters = data_sectors / nav->sectors_per_cluster;

    strcpy(nav->current_path, "/");
    return 1;
}

uint32_t cluster_to_sector(const Navigation *nav, uint32_t cluster) {
    return nav->first_data_sector +
           (cluster - 2) * nav->sectors_per_cluster;
}

int find_entry(FILE *fp, const Navigation *nav, const char *name,
               unsigned char result[32]) {
    size_t cluster_size =
        (size_t)nav->bytes_per_sector * nav->sectors_per_cluster;
    uint32_t cluster = nav->current_cluster;

    for (uint32_t count = 0; count < nav->total_clusters; count++) {
        long offset = (long)cluster_to_sector(nav, cluster) *
                      nav->bytes_per_sector;
        if (fseek(fp, offset, SEEK_SET) != 0) {
            return -1;
        }

        for (size_t i = 0; i < cluster_size / 32; i++) {
            unsigned char entry[32];
            char entry_name[13];

            if (fread(entry, 1, sizeof(entry), fp) != sizeof(entry)) {
                return -1;
            }
            if (entry[0] == 0) {
                return 0;
            }
            if (entry[0] == 0xE5 || entry[11] == 0x0F ||
                (entry[11] & 0x08) != 0) {
                continue;
            }

            get_short_name(entry, entry_name);
            if (names_match(entry_name, name)) {
                memcpy(result, entry, sizeof(entry));
                return 1;
            }
        }

        uint32_t next_cluster = get_fat_entry(fp, nav, cluster);
        if (next_cluster >= 0x0FFFFFF8) {
            return 0;
        }
        if (next_cluster < 2 || next_cluster >= nav->total_clusters + 2) {
            return -1;
        }
        cluster = next_cluster;
    }

    return -1;
}

void cd(FILE *fp, Navigation *nav, const char *dirname) {
    unsigned char entry[32];
    uint32_t cluster;

    if (strcmp(dirname, ".") == 0) {
        return;
    }
    if (strcmp(dirname, "..") == 0 &&
        nav->current_cluster == nav->root_cluster) {
        return;
    }

    int found = find_entry(fp, nav, dirname, entry);
    if (found == 0) {
        printf("cd: %s: directory not found\n", dirname);
        return;
    }
    if (found < 0) {
        printf("cd: error reading directory\n");
        return;
    }
    if ((entry[11] & 0x10) == 0) {
        printf("cd: %s is not a directory\n", dirname);
        return;
    }

    cluster = ((uint32_t)entry[20] << 24) |
              ((uint32_t)entry[21] << 16) |
              ((uint32_t)entry[27] << 8) | entry[26];

    if (strcmp(dirname, "..") == 0 && cluster == 0) {
        cluster = nav->root_cluster;
    }
    if (cluster < 2 || cluster >= nav->total_clusters + 2) {
        printf("cd: invalid directory entry\n");
        return;
    }

    if (strcmp(dirname, "..") == 0) {
        char *last_slash = strrchr(nav->current_path, '/');
        if (last_slash == nav->current_path) {
            nav->current_path[1] = '\0';
        } else {
            *last_slash = '\0';
        }
    } else {
        size_t path_length = strlen(nav->current_path);
        size_t name_length = strlen(dirname);
        size_t separator = path_length > 1 ? 1 : 0;

        if (path_length + separator + name_length >=
            sizeof(nav->current_path)) {
            printf("cd: path is too long\n");
            return;
        }
        if (separator != 0) {
            strcat(nav->current_path, "/");
        }
        strcat(nav->current_path, dirname);
    }

    nav->current_cluster = cluster;
}

void ls(FILE *fp, const Navigation *nav) {
    size_t cluster_size =
        (size_t)nav->bytes_per_sector * nav->sectors_per_cluster;
    uint32_t cluster = nav->current_cluster;

    printf(".\n..\n");

    for (uint32_t count = 0; count < nav->total_clusters; count++) {
        long offset = (long)cluster_to_sector(nav, cluster) *
                      nav->bytes_per_sector;
        if (fseek(fp, offset, SEEK_SET) != 0) {
            printf("ls: error reading directory\n");
            return;
        }

        for (size_t i = 0; i < cluster_size / 32; i++) {
            unsigned char entry[32];
            if (fread(entry, 1, sizeof(entry), fp) != sizeof(entry)) {
                printf("ls: error reading directory\n");
                return;
            }
            if (entry[0] == 0) {
                return;
            }
            if (entry[0] == 0xE5 || entry[11] == 0x0F ||
                (entry[11] & 0x08) != 0 ||
                (entry[0] == '.' && (entry[1] == ' ' || entry[1] == '.'))) {
                continue;
            }

            char name[13];
            get_short_name(entry, name);
            printf("%s\n", name);
        }

        uint32_t next_cluster = get_fat_entry(fp, nav, cluster);
        if (next_cluster >= 0x0FFFFFF8) {
            return;
        }
        if (next_cluster < 2 || next_cluster >= nav->total_clusters + 2) {
            printf("ls: invalid directory cluster chain\n");
            return;
        }
        cluster = next_cluster;
    }

    printf("ls: invalid directory cluster chain\n");
}
