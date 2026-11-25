#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "navigation.h"

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
