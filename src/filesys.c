#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "filesys.h"

FILE * fp;
int exiting = 0;
Navigation navigation;

static const char *image_basename(const char *path) {
    const char *name = path;

    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    return name;
}

int main(int argc, char * argv[]) {
    if(argc != 2) {
        printf("invalid number of arguments.\n");
        return -1;
    } 

    if((fp = fopen(argv[1], "rb+")) == NULL) {
        perror(argv[1]);
        return -1;
    }
    if (!init_navigation(fp, &navigation)) {
        printf("failed to read FAT32 boot sector\n");
        fclose(fp);
        return -1;
    }
    const char *image_name = image_basename(argv[1]);
    while (!exiting) {
        if (strcmp(navigation.current_path, "/") == 0) {
            printf(CLR_EXEC"%s" CLR_RESET":" CLR_DIR"/" CLR_RESET"> ", image_name);
        } else {
            printf(CLR_EXEC"%s" CLR_RESET":" CLR_DIR"%s/" CLR_RESET"> ", image_name, navigation.current_path);
        }

        char *input = get_input();
        tokenlist *tokens = get_tokens(input);
        parse_tokens(tokens);

        free(input);
        free_tokens(tokens);
    }
    fclose(fp);
    return 0;
}

int get_expected_args(const char *cmd) {
    if (cmd == NULL) {
        return -1;
    }
    if (strcmp(cmd, "info") == 0 ||
        strcmp(cmd, "exit") == 0 ||
        strcmp(cmd, "ls") == 0 ||
        strcmp(cmd, "lsof") == 0) {
        return 0;
    }
    if (strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "mkdir") == 0 ||
        strcmp(cmd, "creat") == 0 ||
        strcmp(cmd, "close") == 0 ||
        strcmp(cmd, "rm") == 0 || 
        strcmp(cmd, "rmdir") == 0) {
        return 1;
    }
    if (strcmp(cmd, "open") == 0 ||
        strcmp(cmd, "lseek") == 0 ||
        strcmp(cmd, "read") == 0 ||
        strcmp(cmd, "write") == 0 ||
        strcmp(cmd, "mv") == 0) {
        return 2;
    }
    return -1;
}

void parse_tokens(tokenlist* tokens) {
    if (tokens->size < 1) 
        return;

    char *cmd = tokens->items[0];

    int expected_args = get_expected_args(cmd);
    if (expected_args == -1) {
        printf("%s: command not found\n", cmd);
        return;
    }
    if (tokens->size-1 < expected_args) {
        printf("%s: missing arguments\n", cmd);
        return;
    }
    else if (expected_args != 0 && tokens->size-1 > expected_args) {
        printf("%s: too many arguments\n", cmd);
        return;
    }

    if (strcmp(cmd, "info") == 0) {
        info(&navigation); 
    }
    else if (strcmp(cmd, "exit") == 0) {
        free(navigation.open_files);
        exiting = 1;
    }
    else if (strcmp(cmd, "ls") == 0) {
        ls_cmd(fp, &navigation);
    }
    else if (strcmp(cmd, "cd") == 0) {
        cd_cmd(fp, &navigation, tokens->items[1]);
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        mkdir_cmd(fp, &navigation, tokens->items[1]);
    }
    else if (strcmp(cmd, "creat") == 0) {
        creat_cmd(fp, &navigation, tokens->items[1]);
    }
    else if (strcmp(cmd, "open") == 0) {
        open_cmd(fp, &navigation, tokens->items[1], tokens->items[2]);
    }
    else if (strcmp(cmd, "close") == 0) {
        close_cmd(fp, &navigation, tokens->items[1]);
    }
    else if (strcmp(cmd, "lsof") == 0) {
        lsof_cmd(&navigation);
    }
    else if (strcmp(cmd, "lseek") == 0) {
        lseek_cmd(fp, &navigation, tokens->items[1], atoi(tokens->items[2]));
    }
    else if (strcmp(cmd, "read") == 0) {
        read_cmd(fp, &navigation, tokens->items[1], atoi(tokens->items[2]));
    }
    else if (strcmp(cmd, "write") == 0) {

    }
    else if (strcmp(cmd, "mv") == 0) {

    }
    else if (strcmp(cmd, "rm") == 0) {

    }
    else if (strcmp(cmd, "rmdir") == 0) {

    }
}
