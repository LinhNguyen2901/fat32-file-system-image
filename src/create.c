#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "navigation.h"

static int valid_filename(const char *filename) {
    size_t length = strlen(filename);

    if (length == 0 || length > 8) {
        return 0;
    }

    for (size_t i = 0; i < length; i++) {
        if (filename[i] <= ' ' ||
            strchr("\"*+,./:;<=>?[\\]|", filename[i]) != NULL) {
            return 0;
        }
    }
    return 1;
}

static int find_free_entry(FILE *fp, const Navigation *nav, long *entry_offset) {
    size_t cluster_size =
        (size_t)nav->bytes_per_sector * nav->sectors_per_cluster;
    uint32_t cluster = nav->current_cluster;

    for (uint32_t count = 0; count < nav->data_clusters; count++) {
        long offset = (long)cluster_to_sector(nav, cluster) *
                      nav->bytes_per_sector;
        if (fseek(fp, offset, SEEK_SET) != 0) {
            return -1;
        }

        for (size_t i = 0; i < cluster_size / 32; i++) {
            unsigned char entry[32];

            if (fread(entry, 1, sizeof(entry), fp) != sizeof(entry)) {
                return -1;
            }
            if (entry[0] == 0 || entry[0] == 0xE5) {
                *entry_offset = offset + (long)(i * 32);
                return 1;
            }
        }

        uint32_t next_cluster = get_fat_entry(fp, nav, cluster);
        if (next_cluster >= 0x0FFFFFF8) {
            return 0;
        }
        if (next_cluster < 2 || next_cluster >= nav->data_clusters + 2) {
            return -1;
        }
        cluster = next_cluster;
    }

    return -1;
}

static int find_free_cluster(FILE *fp, const Navigation *nav,
                             uint32_t *free_cluster) {
    for (uint32_t cluster = 2; cluster < nav->data_clusters + 2; cluster++) {
        if (get_fat_entry(fp, nav, cluster) == 0) {
            *free_cluster = cluster;
            return 1;
        }
    }
    return 0;
}

static int write_fat_entry(FILE *fp, const Navigation *nav, uint32_t cluster) {
    unsigned char end_of_chain[4] = {0xFF, 0xFF, 0xFF, 0x0F};

    for (uint8_t fat = 0; fat < nav->number_of_fats; fat++) {
        long offset = (long)(nav->first_fat_sector +
                      fat * nav->fat_size_in_sectors) *
                      nav->bytes_per_sector + (long)cluster * 4;
        if (fseek(fp, offset, SEEK_SET) != 0 ||
            fwrite(end_of_chain, 1, sizeof(end_of_chain), fp) !=
                sizeof(end_of_chain)) {
            return 0;
        }
    }
    return fflush(fp) == 0;
}

static void set_entry_cluster(unsigned char entry[32], uint32_t cluster) {
    entry[20] = (unsigned char)(cluster >> 16);
    entry[21] = (unsigned char)(cluster >> 24);
    entry[26] = (unsigned char)cluster;
    entry[27] = (unsigned char)(cluster >> 8);
}

void mkdir_cmd(FILE *fp, Navigation *nav, const char *dirname) {
    unsigned char existing[32];
    unsigned char entry[32] = {0};
    uint32_t new_cluster;
    long entry_offset;

    if (!valid_filename(dirname)) {
        printf("mkdir: invalid directory name (use 1-8 characters)\n");
        return;
    }

    int found = find_entry(fp, nav, dirname, existing);
    if (found == 1) {
        printf("mkdir: %s already exists\n", dirname);
        return;
    }
    if (found < 0) {
        printf("mkdir: error reading directory\n");
        return;
    }

    int available = find_free_entry(fp, nav, &entry_offset);
    if (available == 0) {
        printf("mkdir: current directory is full\n");
        return;
    }
    if (available < 0) {
        printf("mkdir: error reading directory\n");
        return;
    }
    if (!find_free_cluster(fp, nav, &new_cluster)) {
        printf("mkdir: no free clusters\n");
        return;
    }

    size_t cluster_size =
        (size_t)nav->bytes_per_sector * nav->sectors_per_cluster;
    unsigned char *cluster_data = calloc(cluster_size, 1);
    if (cluster_data == NULL) {
        printf("mkdir: not enough memory\n");
        return;
    }

    memset(cluster_data, ' ', 11);
    cluster_data[0] = '.';
    cluster_data[11] = 0x10;
    set_entry_cluster(cluster_data, new_cluster);

    memset(cluster_data + 32, ' ', 11);
    cluster_data[32] = '.';
    cluster_data[33] = '.';
    cluster_data[43] = 0x10;
    set_entry_cluster(cluster_data + 32, nav->current_cluster);

    long cluster_offset = (long)cluster_to_sector(nav, new_cluster) *
                          nav->bytes_per_sector;
    int cluster_written =
        fseek(fp, cluster_offset, SEEK_SET) == 0 &&
        fwrite(cluster_data, 1, cluster_size, fp) == cluster_size &&
        fflush(fp) == 0;
    free(cluster_data);
    if (!cluster_written || !write_fat_entry(fp, nav, new_cluster)) {
        printf("mkdir: error writing directory data\n");
        return;
    }

    memset(entry, ' ', 11);
    for (size_t i = 0; dirname[i] != '\0'; i++) {
        entry[i] = (unsigned char)toupper((unsigned char)dirname[i]);
    }
    entry[11] = 0x10;
    set_entry_cluster(entry, new_cluster);

    if (fseek(fp, entry_offset, SEEK_SET) != 0 ||
        fwrite(entry, 1, sizeof(entry), fp) != sizeof(entry) ||
        fflush(fp) != 0) {
        printf("mkdir: error writing directory entry\n");
    }
}

void creat_cmd(FILE *fp, Navigation *nav, const char *filename) {
    unsigned char existing[32];
    unsigned char entry[32] = {0};
    long entry_offset;

    if (!valid_filename(filename)) {
        printf("creat: invalid filename (use 1-8 characters, no extension)\n");
        return;
    }

    int found = find_entry(fp, nav, filename, existing);
    if (found == 1) {
        printf("creat: %s already exists\n", filename);
        return;
    }
    if (found < 0) {
        printf("creat: error reading directory\n");
        return;
    }

    int available = find_free_entry(fp, nav, &entry_offset);
    if (available == 0) {
        printf("creat: current directory is full\n");
        return;
    }
    if (available < 0) {
        printf("creat: error reading directory\n");
        return;
    }

    memset(entry, ' ', 11);
    for (size_t i = 0; filename[i] != '\0'; i++) {
        entry[i] = (unsigned char)toupper((unsigned char)filename[i]);
    }
    entry[11] = 0x20;

    if (fseek(fp, entry_offset, SEEK_SET) != 0 ||
        fwrite(entry, 1, sizeof(entry), fp) != sizeof(entry) ||
        fflush(fp) != 0) {
        printf("creat: error writing directory entry\n");
    }
}
