#include "utils.h"

/* Define debug flag, set it off */
int debug = 0;

int run_cmd(char *cmd, ...) {
    
    va_list args;
    char buf[CMDBUFLEN];
    
    /* Initialize var arg list */
    va_start(args, cmd);

    /* Format the command into our buffer */
    vsnprintf(buf, CMDBUFLEN, cmd, args);

    /* Clean up the va_list */
    va_end(args);

    /* if debug mode on print the command */
    if (debug) {
        printf("EXEC: %s\n", buf);
    }

    return system(buf);

}