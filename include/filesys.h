#ifndef FILESYS_H
#define FILESYS_H

#include "navigation.h"

int get_expected_args(const char *cmd); 
void parse_tokens(tokenlist* tokens);

void info(FILE *fp);

void ls(FILE *fp);
void cd(FILE *fp, const char *dirname);

void mkdir(FILE *fp, const char *dirname);
void creat(FILE *fp, const char *filename);

void open(FILE *fp, const char *filename, const char *flags);
void close(const char *filename);
void lsof(void);
void lseek(const char *filename, int offset);
void read(FILE *fp, const char *filename, int size);

void write(FILE *fp, const char *filename, const char *data);
void mv(FILE *fp, const char *src, const char *dest);

void rm(FILE *fp, const char *filename);
void rmdir(FILE *fp, const char *dirname);

#endif // FILESYS_H

