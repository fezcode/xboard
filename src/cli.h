#ifndef CLI_H
#define CLI_H
#include <stdbool.h>
#include <stdio.h>

/* Parsed command line. Deliberately free of SDL and application state so the
 * argument rules can be tested on their own. */
typedef struct {
    bool help;
    bool version;
    bool screenshot;
    bool compact;
    char message[128];   /* Explains the problem when cli_parse fails. */
} CliOptions;

/* Fills out and returns true when the arguments are usable. On failure the
 * options are still zeroed and message describes what was wrong. */
bool cli_parse(int argc, char* const argv[], CliOptions* out);

/* The usage text itself, so it can be checked without touching the filesystem. */
const char* cli_usage_text(void);
void cli_print_usage(FILE* out);

#endif /* CLI_H */
