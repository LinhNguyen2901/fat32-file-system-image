#include <string.h>
#include <stdlib.h>
#include "navigation.h"

static int is_valid_mode(const char *flags) {
    return strcmp(flags, "-r") == 0 ||
           strcmp(flags, "-w") == 0 ||
           strcmp(flags, "-rw") == 0 ||
           strcmp(flags, "-wr") == 0;
}
static int can_read_mode(const char *mode) {
    return strcmp(mode, "-r") == 0 ||
           strcmp(mode, "-rw") == 0 ||
           strcmp(mode, "-wr") == 0;
}
static int resolve_entry(FILE *fp, const Navigation *nav, const char *cmd, const char *filename, unsigned char entry[32], char short_name[13]) {
    int found = find_entry(fp, nav, filename, entry);
    if (found < 0) {
        printf("%s: error reading directory\n", cmd);
        return 0;
    }
    if (found != 1) {
        printf("%s: %s doesn't exist\n", cmd, filename);
        return 0;
    }
    if ((entry[11] & 0x10) != 0) {
        printf("%s: %s is a directory\n", cmd, filename);
        return 0;
    }
    get_short_name(entry, short_name);
    return 1;
}
static int get_open_file_index(const Navigation *nav, const char *short_name) {
    for (int i = 0; i < nav->open_count; i++) {
        if (nav->open_files[i].dir_cluster == nav->current_cluster &&
            names_match(short_name, nav->open_files[i].filename)) {
            return i;
        }
    }
    return -1;
}

void open_cmd(FILE *fp, Navigation *nav, const char *filename, const char *flags) {
    if (!is_valid_mode(flags)) {
        printf("open: invalid mode %s\n", flags);
        return;
    }
    unsigned char existing[32];
    char entry_name[13];
    if (!resolve_entry(fp, nav, "open", filename, existing, entry_name)) {
        return;
    }
    if (get_open_file_index(nav, entry_name) >= 0) {
        printf("open: %s is already open\n", filename);
        return;
    }

    OpenFile new_file = {0};;
    strcpy(new_file.filename, entry_name);
    strcpy(new_file.path, nav->current_path);
    strcpy(new_file.mode, flags);
    new_file.first_cluster = entry_to_cluster(existing);
    new_file.dir_cluster = nav->current_cluster;
    memcpy(&new_file.size, existing + 28, sizeof(uint32_t));
    new_file.offset = 0;

    OpenFile *resized = realloc(nav->open_files, (nav->open_count + 1) * sizeof(OpenFile));
    if (resized == NULL) {
        printf("open: memory allocation failed\n");
        return;
    }
    resized[nav->open_count] = new_file;
    nav->open_files = resized;
    nav->open_count++;
}

void close_cmd(FILE *fp, Navigation *nav, const char *filename) {
    unsigned char existing[32];
    char entry_name[13];
    if (!resolve_entry(fp, nav, "close", filename, existing, entry_name)) {
        return;
    }

    int index = get_open_file_index(nav, entry_name);
    if (index < 0) {
        printf("close: %s isn't open\n", filename);
        return;
    }
    get_short_name(existing, entry_name);
    for (int j = index; j < nav->open_count - 1; j++) {
        nav->open_files[j] = nav->open_files[j + 1];
    }

    if (nav->open_count == 1) {
        free(nav->open_files);
        nav->open_files = NULL;
    } 
    else {
        OpenFile *resized = realloc(nav->open_files, (nav->open_count - 1) * sizeof(OpenFile));
        if (resized != NULL)
            nav->open_files = resized;
    }
    nav->open_count--;
}

void lsof_cmd(Navigation *nav) {
    if (nav->open_count < 1) {
        printf("lsof: no files are opened\n");
        return;
    }
    printf("%-6s %-12s %-5s | %10s | %s\n", "index", "filename", "mode", "offset", "path");
    for (int i = 0; i < nav->open_count; i++) {
        OpenFile file = nav->open_files[i];
        printf(" [%02d]  %-12s %-5s | %10u | %s\n", i, file.filename, file.mode, file.offset, file.path);
    }
}

void lseek_cmd(FILE *fp, Navigation *nav, const char *filename, int offset) {
    unsigned char existing[32];
    char entry_name[13];
    if (!resolve_entry(fp, nav, "lseek", filename, existing, entry_name)) {
        return;
    }
    int index = get_open_file_index(nav, entry_name);
    if (index < 0) {
        printf("lseek: %s isn't open\n", filename);
        return;
    }
    if (offset < 0)
        printf("lseek: offset cannot be negative\n");
    else if (offset > nav->open_files[index].size)
        printf("lseek: %d bigger than file size %d\n", offset, nav->open_files[index].size);
    else 
        nav->open_files[index].offset = offset;
}

void read_cmd(FILE *fp, Navigation *nav, const char *filename, int size) {
    if (size < 0) {
        printf("read: invalid size\n");
        return;
    }
    unsigned char existing[32];
    char entry_name[13];
    if (!resolve_entry(fp, nav, "lseek", filename, existing, entry_name)) {
        return;
    }
    int index = get_open_file_index(nav, entry_name);
    if (index < 0) {
        printf("lseek: %s isn't open\n", filename);
        return;
    }

    // Clamping offset < read area < size
    OpenFile *file = &nav->open_files[index];
    if (!can_read_mode(file->mode)) {
        printf("read: %s is not opened for reading\n", filename);
        return;
    }
    if (file->offset >= file->size || size == 0) {
        return;
    }
    uint32_t cluster = file->first_cluster;
    uint32_t bytes_left = file->offset + (uint32_t)size > file->size? file->size - file->offset : (uint32_t)size;
    uint32_t offset = file->offset;
    for (; offset >= nav->cluster_size; offset -= nav->cluster_size) {
        cluster = get_fat_entry(fp, nav, cluster);
        if (cluster >= 0x0FFFFFF8 || cluster < 2) {
            printf("read: invalid file cluster chain\n");
            return;
        }
    }

    unsigned char *buffer = malloc(nav->cluster_size);
    if (buffer == NULL) {
        printf("read: memory allocation failed\n");
        return;
    }
    for (uint32_t count = 0; count < nav->data_clusters && bytes_left > 0; count++) {
        uint32_t read_size = (bytes_left < nav->cluster_size - offset) ? bytes_left : nav->cluster_size - offset;
        long disk_offset = (long)cluster_to_sector(nav, cluster) * nav->bytes_per_sector + offset;
        if (fseek(fp, disk_offset, SEEK_SET) != 0 || fread(buffer, 1, read_size, fp) != read_size) {
            printf("\nread: read error\n");
            free(buffer);
            return;
        }
        fwrite(buffer, 1, read_size, stdout);
        bytes_left -= read_size;
        file->offset += read_size;
        offset = 0;
        if (bytes_left > 0) {
            uint32_t next_cluster = get_fat_entry(fp, nav, cluster);
            if (next_cluster >= 0x0FFFFFF8) {
                printf("\n");
                free(buffer);
                return;
            }
            if (next_cluster < 2 || next_cluster >= nav->data_clusters + 2) {
                printf("\nread: invalid file cluster chain\n");
                free(buffer);
                return;
            }
        cluster = next_cluster;
        }
    }
    printf("\n");
    free(buffer);
}
