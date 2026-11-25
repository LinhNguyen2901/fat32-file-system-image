#include <stdio.h>
#include <stdint.h>

// uint8_t is the size of 1 byte
void info(FILE *fp) {
    uint8_t boot[512];
    if (fseek(fp, 0, SEEK_SET) != 0 ||
        fread(boot, 1, sizeof(boot), fp) != sizeof(boot)) {
        printf("Failed to read boot sector\n");
        return;
    }
    
    uint16_t bytes_per_sector = *(uint16_t *)(boot + 11);
    uint8_t  sectors_per_cluster = *(uint8_t  *)(boot + 13);
    uint16_t reserved_sectors = *(uint16_t *)(boot + 14);
    uint8_t  num_fats = *(uint8_t  *)(boot + 16);
    uint32_t total_sectors = *(uint32_t *)(boot + 32);
    uint32_t fat_size_in_sectors = *(uint32_t *)(boot + 36);

    printf("Position of root cluster (cluster #): %u\n", *(uint32_t*)(boot + 44));
    printf("Bytes per sector: %u\n", bytes_per_sector);
    printf("Sectors per cluster: %u\n", sectors_per_cluster);
    printf("Total # of clusters in data region: %u\n", 
            (total_sectors - (reserved_sectors + num_fats * fat_size_in_sectors)) / sectors_per_cluster);
    printf("# of entries in one FAT: %u\n", (fat_size_in_sectors * bytes_per_sector) / 4);
    printf("Size of image (in bytes): %lu\n", (uint64_t)total_sectors * bytes_per_sector);
}

