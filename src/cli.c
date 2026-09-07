#include "cli.h"
#include <stddef.h>
#include <string.h>

bool cli_parse(int argc, char* const argv[], CliOptions* out) {
    memset(out, 0, sizeof(*out));
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) out->help = true;
        else if (!strcmp(arg, "-v") || !strcmp(arg, "--version")) out->version = true;
        else if (!strcmp(arg, "--screenshot")) out->screenshot = true;
        else if (!strcmp(arg, "--compact")) out->compact = true;
        else {
            /* Truncate rather than overflow; the reader only needs the offender. */
            snprintf(out->message, sizeof(out->message), "Unknown option: %.80s", arg);
            memset(out, 0, offsetof(CliOptions, message));
            return false;
        }
    }
    /* Usage and version stay reachable no matter what else was typed. */
    if (out->help || out->version) return true;
    if (out->compact && !out->screenshot) {
        snprintf(out->message, sizeof(out->message), "--compact only applies with --screenshot");
        memset(out, 0, offsetof(CliOptions, message));
        return false;
    }
    return true;
}

const char* cli_usage_text(void) {
    return
        "xboard - a controller keyboard for Windows\n"
        "\n"
        "Usage: xboard [options]\n"
        "\n"
        "  -h, --help      Show this help and exit\n"
        "  -v, --version   Show the version and exit\n"
        "      --screenshot  Render one capture per mode and theme, then exit.\n"
        "                    Runs hidden, keeps preferences untouched and sends no keystrokes.\n"
        "      --compact     Capture at the 1100x860 minimum size (needs --screenshot)\n"
        "\n"
        "With no options xboard opens the keyboard window.\n"
        "\n"
        "Controller:\n"
        "  A  Select        B  Backspace (hold to repeat)   X  Space      Y  Enter\n"
        "  LB / RB  Previous / next mode                    LT  Shift     RT  Ctrl\n"
        "  L3 + R3  Toggle quick phrases                    L3  Caps      R3  Symbols\n"
        "  View  Local / direct output   Menu  Copy composer   Guide  Windows key\n"
        "  D-pad  Grid navigation, cursor movement elsewhere\n"
        "\n"
        "Keyboard:\n"
        "  1 / 2 / 3  Choose mode      Tab  Cycle modes     F1  Help      F2  Theme\n"
        "  Arrows  Move the cursor     Esc  Close help, or quit\n"
        "\n"
        "Preferences, including your quick phrases and window placement, live in\n"
        "the xboard folder under %APPDATA%.\n";
}

void cli_print_usage(FILE* out) {
    fputs(cli_usage_text(), out);
}
