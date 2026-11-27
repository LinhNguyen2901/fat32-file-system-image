#include <stdio.h>
#include <stdint.h>
#include "navigation.h"

void info(Navigation *nav) {
    printf("Position of root cluster (cluster #): %u\n", nav->root_cluster);
    printf("Bytes per sector: %u\n", nav->bytes_per_sector);
    printf("Sectors per cluster: %u\n", nav->sectors_per_cluster);
    printf("Total # of clusters in data region: %u\n", nav->data_clusters);
    printf("# of entries in one FAT: %u\n", (nav->fat_size_in_sectors * nav->bytes_per_sector) / 4);
    printf("Size of image (in bytes): %lu\n", (uint64_t)nav->total_sectors * nav->bytes_per_sector);
}

