#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CMDBUFLEN 256

/* When set to 1, commands will be printed before execution, extern because defined in diff file */
extern int debug;

/* Formats and executes shell command */
int run_cmd(char *cmd, ...);

#endif /* UTILS_H */