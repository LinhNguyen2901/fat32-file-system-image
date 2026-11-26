#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint8_t number_of_fats;
    uint32_t fat_size_in_sectors;
    uint32_t root_cluster;
    uint32_t current_cluster;
    uint32_t first_fat_sector;
    uint32_t first_data_sector;
    uint32_t total_clusters;
    char current_path[256];
} Navigation;

int init_navigation(FILE *fp, Navigation *nav);
uint32_t get_fat_entry(FILE *fp, const Navigation *nav, uint32_t cluster);
uint32_t cluster_to_sector(const Navigation *nav, uint32_t cluster);
/* Returns 1 if found, 0 if missing, and -1 if the directory could not be read. */
int find_entry(FILE *fp, const Navigation *nav, const char *name,
               unsigned char result[32]);

#endif
