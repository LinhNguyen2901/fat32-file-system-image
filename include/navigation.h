#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint32_t root_cluster;
    uint32_t current_cluster;
    uint32_t first_fat_sector;
    uint32_t first_data_sector;
    uint32_t total_clusters;
    char current_path[256];
} Navigation;

int init_navigation(FILE *fp, Navigation *nav);

#endif
