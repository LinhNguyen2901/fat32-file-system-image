#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "navigation.h"

static uint32_t get_fat_entry(FILE *fp, const Navigation *nav, uint32_t cluster) {
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

static void print_short_name(const unsigned char *entry) {
    char name[13];
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
    printf("%s\n", name);
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
    uint8_t number_of_fats = *(uint8_t *)(boot + 16);

    nav->bytes_per_sector = *(uint16_t *)(boot + 11);
    nav->sectors_per_cluster = *(uint8_t *)(boot + 13);
    nav->fat_size_in_sectors = *(uint32_t *)(boot + 36);
    nav->root_cluster = *(uint32_t *)(boot + 44);
    nav->current_cluster = nav->root_cluster;
    nav->first_fat_sector = reserved_sectors;
    nav->first_data_sector =
        reserved_sectors + number_of_fats * nav->fat_size_in_sectors;

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

            print_short_name(entry);
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
