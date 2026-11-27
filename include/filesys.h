#ifndef FILESYS_H
#define FILESYS_H

#include "navigation.h"

int get_expected_args(const char *cmd); 
void parse_tokens(tokenlist* tokens);

void info(Navigation *navigation);

void ls_cmd(FILE *fp, const Navigation *nav);
void cd_cmd(FILE *fp, Navigation *nav, const char *dirname);

void mkdir_cmd(FILE *fp, Navigation *nav, const char *dirname);
void creat_cmd(FILE *fp, Navigation *nav, const char *filename);

int open_cmd(FILE *fp, Navigation *nav, const char *filename, const char *flags);
int close_cmd(FILE *fp, Navigation *nav, const char *filename);
void lsof_cmd(Navigation *nav);
void lseek_cmd(FILE *fp, Navigation *nav, const char *filename, int offset);
void read_cmd(FILE *fp, Navigation *nav, const char *filename, int size);

void write_cmd(FILE *fp, const char *filename, const char *data);
void mv_cmd(FILE *fp, const char *src, const char *dest);

void rm_cmd(FILE *fp, const char *filename);
void rmdir_cmd(FILE *fp, const char *dirname);

#endif // FILESYS_H

