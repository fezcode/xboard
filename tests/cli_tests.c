/* Command line parsing is independent of SDL, so this suite links cli.c alone. */
#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

static CliOptions parse(bool expect_ok, int argc, char* argv[]) {
    CliOptions o;
    bool ok = cli_parse(argc, argv, &o);
    if (ok != expect_ok) {
        fprintf(stderr, "cli_parse returned %d for '%s', expected %d (%s)\n",
                ok, argc > 1 ? argv[1] : "", expect_ok, o.message);
        exit(1);
    }
    return o;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    /* A bare invocation runs the application normally. */
    char* none[] = {"xboard"};
    CliOptions o = parse(true, 1, none);
    CHECK(!o.help && !o.version && !o.screenshot && !o.compact);

    char* help_long[] = {"xboard", "--help"};
    CHECK(parse(true, 2, help_long).help);
    char* help_short[] = {"xboard", "-h"};
    CHECK(parse(true, 2, help_short).help);

    char* version_long[] = {"xboard", "--version"};
    CHECK(parse(true, 2, version_long).version);
    char* version_short[] = {"xboard", "-v"};
    CHECK(parse(true, 2, version_short).version);

    char* shot[] = {"xboard", "--screenshot"};
    o = parse(true, 2, shot);
    CHECK(o.screenshot && !o.compact);

    /* Flags are order independent; --compact used to be recognized only as argv[2]. */
    char* shot_compact[] = {"xboard", "--screenshot", "--compact"};
    o = parse(true, 3, shot_compact);
    CHECK(o.screenshot && o.compact);
    char* compact_shot[] = {"xboard", "--compact", "--screenshot"};
    o = parse(true, 3, compact_shot);
    CHECK(o.screenshot && o.compact);

    /* --compact only means something alongside --screenshot. */
    char* compact_only[] = {"xboard", "--compact"};
    o = parse(false, 2, compact_only);
    CHECK(strstr(o.message, "--compact") != NULL);

    /* Help and version win over the --compact requirement so usage stays reachable. */
    char* help_compact[] = {"xboard", "--compact", "--help"};
    CHECK(parse(true, 3, help_compact).help);

    /* Unknown arguments name the offender instead of starting a hidden window. */
    char* bogus[] = {"xboard", "--bogus"};
    o = parse(false, 2, bogus);
    CHECK(strstr(o.message, "--bogus") != NULL);
    char* stray[] = {"xboard", "screenshot"};
    o = parse(false, 2, stray);
    CHECK(strstr(o.message, "screenshot") != NULL);
    char* after_valid[] = {"xboard", "--screenshot", "-x"};
    o = parse(false, 3, after_valid);
    CHECK(strstr(o.message, "-x") != NULL);

    /* An overlong argument must not overflow the fixed message buffer. */
    char big[512];
    memset(big, 'q', sizeof(big) - 1);
    big[0] = '-'; big[1] = '-'; big[sizeof(big) - 1] = '\0';
    char* huge[] = {"xboard", big};
    o = parse(false, 2, huge);
    CHECK(strlen(o.message) < sizeof(o.message));

    /* Usage text has to mention every accepted flag. */
    const char* usage = cli_usage_text();
    CHECK(strstr(usage, "--help") && strstr(usage, "--version"));
    CHECK(strstr(usage, "-h") && strstr(usage, "-v"));
    CHECK(strstr(usage, "--screenshot") && strstr(usage, "--compact"));
    /* It is printed verbatim, so a stray conversion specifier would be a bug. */
    CHECK(strchr(usage, '%') == strstr(usage, "%APPDATA%"));

    puts("Command line flags parse in any order, reject unknown arguments and document themselves.");
    return 0;
}
