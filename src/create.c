#include <ctype.h>
#include <stdio.h>
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

    for (uint32_t count = 0; count < nav->total_clusters; count++) {
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
        if (next_cluster < 2 || next_cluster >= nav->total_clusters + 2) {
            return -1;
        }
        cluster = next_cluster;
    }

    return -1;
}

void creat(FILE *fp, Navigation *nav, const char *filename) {
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
