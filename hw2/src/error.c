/*
 * Error handling routines
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int errors;
int warnings;
int dbflag = 1;

void fatal(char *fmt, ...)
{
        fprintf(stderr, "\nFatal error: ");
        fprintf(stderr, "%s", fmt);
        fprintf(stderr, "\n");
        exit(1);
}

void error(char *fmt, ...)
{
        fprintf(stderr, "\nError: ");
        fprintf(stderr, "%s", fmt);
        fprintf(stderr, "\n");
        errors++;
}

void warning(char *fmt, ...)
{
        fprintf(stderr, "\nWarning: ");
        fprintf(stderr, "%s", fmt);
        fprintf(stderr, "\n");
        warnings++;
}

void debug(char *fmt, ...)
{
        if(!dbflag) return;
        fprintf(stderr, "\nDebug: ");
        fprintf(stderr, "%s", fmt);
        fprintf(stderr, "\n");
}
