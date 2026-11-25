#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "filesys.h"

FILE * fp;
int exiting = 0;

int main(int argc, char * argv[]) {
    if(argc != 2) {
        printf("invalid number of arguments.\n");
        return -1;
    } 

    if((fp = fopen(argv[1], "rb")) == NULL) {
        printf("%s does not exist\n.", argv[1]);
        return -1;
    }
    while (!exiting) {
        printf("> ");

        char *input = get_input();
        tokenlist *tokens = get_tokens(input);
        for (int i = 0; i < tokens->size; i++) {
            printf("%s\n", tokens->items[i]);
        }
        parse_tokens(tokens);

        free(input);
        free_tokens(tokens);
    }
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
       info(fp); 
    }
    else if (strcmp(cmd, "exit") == 0) {
        exiting = 1;
    }
    else if (strcmp(cmd, "ls") == 0) {

    }
    else if (strcmp(cmd, "lsof") == 0) {

    }
    else if (strcmp(cmd, "cd") == 0) {

    }
    else if (strcmp(cmd, "mkdir") == 0) {
    
    }
    else if (strcmp(cmd, "creat") == 0) {

    }
    else if (strcmp(cmd, "close") == 0) {

    }
    else if (strcmp(cmd, "rm") == 0) {

    }
    else if (strcmp(cmd, "rmdir") == 0) {

    }
    else if (strcmp(cmd, "open") == 0) {

    }
    else if (strcmp(cmd, "lseek") == 0) {
    
    }
    else if (strcmp(cmd, "read") == 0) {

    }
    else if (strcmp(cmd, "mv") == 0) {

    }
    else if (strcmp(cmd, "write") == 0) {

    }
}
