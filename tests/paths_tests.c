/* Settings migration from the old folder, exercised in a scratch temp folder. */
#include "app_paths.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

#ifdef _WIN32
#include <windows.h>

static wchar_t base[MAX_PATH];
static wchar_t root[MAX_PATH];
static int case_no;

static void path(wchar_t* out, const wchar_t* rel) { _snwprintf(out, MAX_PATH, L"%ls\\%ls", root, rel); }
static void mkdir_rel(const wchar_t* rel) { wchar_t p[MAX_PATH]; path(p, rel); CreateDirectoryW(p, NULL); }
static void write_rel(const wchar_t* rel, const char* text) {
    wchar_t p[MAX_PATH]; path(p, rel);
    FILE* f = _wfopen(p, L"wb"); CHECK(f); fputs(text, f); fclose(f);
}
static bool exists_rel(const wchar_t* rel) { wchar_t p[MAX_PATH]; path(p, rel); return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }
static bool content_is(const wchar_t* rel, const char* text) {
    wchar_t p[MAX_PATH]; path(p, rel);
    char buf[64] = {0}; FILE* f = _wfopen(p, L"rb"); if (!f) return false;
    fread(buf, 1, sizeof(buf) - 1, f); fclose(f); return !strcmp(buf, text);
}
static bool migrate(void) {
    wchar_t o[MAX_PATH], n[MAX_PATH], parent[MAX_PATH];
    path(o, L"xboard\\xboard"); path(n, L"fezcode\\xboard"); path(parent, L"xboard");
    return app_migrate_dir(o, n, parent);
}
/* Each case gets its own fresh folder: deleting the previous one can lag
 * behind (antivirus, indexer) and must never leak files into the next case. */
static void reset(void) {
    _snwprintf(root, MAX_PATH, L"%ls\\case%d", base, ++case_no);
    CreateDirectoryW(root, NULL);
    mkdir_rel(L"fezcode");
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    wchar_t tmp[MAX_PATH]; GetTempPathW(MAX_PATH, tmp);
    _snwprintf(base, MAX_PATH, L"%lsxboard_paths_test_%lu", tmp, GetCurrentProcessId());
    CreateDirectoryW(base, NULL);

    /* Nothing to migrate is a success and a no-op. */
    reset();
    CHECK(migrate());
    CHECK(!exists_rel(L"fezcode\\xboard"));

    /* Fresh target: the folder is renamed whole and the old parent removed. */
    reset();
    mkdir_rel(L"xboard"); mkdir_rel(L"xboard\\xboard"); mkdir_rel(L"xboard\\xboard\\sub");
    write_rel(L"xboard\\xboard\\preferences.ini", "old");
    write_rel(L"xboard\\xboard\\sub\\extra.txt", "x");
    CHECK(migrate());
    CHECK(content_is(L"fezcode\\xboard\\preferences.ini", "old"));
    CHECK(content_is(L"fezcode\\xboard\\sub\\extra.txt", "x"));
    CHECK(!exists_rel(L"xboard"));
    CHECK(migrate());   /* Idempotent. */
    CHECK(content_is(L"fezcode\\xboard\\preferences.ini", "old"));

    /* Existing target: merge without overwriting. The new copy wins every
     * conflict and the stale old copy is deleted, so the old folders go away. */
    reset();
    mkdir_rel(L"xboard"); mkdir_rel(L"xboard\\xboard"); mkdir_rel(L"xboard\\xboard\\sub");
    mkdir_rel(L"xboard\\xboard\\clash");
    write_rel(L"xboard\\xboard\\preferences.ini", "old");
    write_rel(L"xboard\\xboard\\other.txt", "moved");
    write_rel(L"xboard\\xboard\\sub\\extra.txt", "x");
    write_rel(L"xboard\\xboard\\sub\\dup.txt", "old");
    write_rel(L"xboard\\xboard\\clash\\inside.txt", "old");   /* Folder vs. new file. */
    {   /* A read-only stale copy must still be removed. */
        wchar_t p[MAX_PATH]; path(p, L"xboard\\xboard\\preferences.ini");
        SetFileAttributesW(p, FILE_ATTRIBUTE_READONLY);
    }
    mkdir_rel(L"fezcode\\xboard"); mkdir_rel(L"fezcode\\xboard\\sub");
    write_rel(L"fezcode\\xboard\\preferences.ini", "new");
    write_rel(L"fezcode\\xboard\\sub\\dup.txt", "new");
    write_rel(L"fezcode\\xboard\\clash", "new");
    CHECK(migrate());
    CHECK(content_is(L"fezcode\\xboard\\preferences.ini", "new"));
    CHECK(content_is(L"fezcode\\xboard\\sub\\dup.txt", "new"));
    CHECK(content_is(L"fezcode\\xboard\\clash", "new"));
    CHECK(content_is(L"fezcode\\xboard\\other.txt", "moved"));
    CHECK(content_is(L"fezcode\\xboard\\sub\\extra.txt", "x"));
    CHECK(!exists_rel(L"xboard\\xboard"));
    CHECK(!exists_rel(L"xboard"));
    CHECK(migrate());   /* Idempotent. */
    CHECK(content_is(L"fezcode\\xboard\\preferences.ini", "new"));

    /* An unrelated file in the old parent keeps the parent alive. */
    reset();
    mkdir_rel(L"xboard"); mkdir_rel(L"xboard\\xboard");
    write_rel(L"xboard\\xboard\\preferences.ini", "old");
    write_rel(L"xboard\\keep.txt", "k");
    CHECK(migrate());
    CHECK(content_is(L"xboard\\keep.txt", "k"));
    CHECK(content_is(L"fezcode\\xboard\\preferences.ini", "old"));

    wchar_t cmd[MAX_PATH * 2];
    _snwprintf(cmd, MAX_PATH * 2, L"cmd /c rd /s /q \"%ls\" 2>nul", base);
    _wsystem(cmd);
    puts("Legacy settings move into the fezcode folder without overwriting anything.");
    return 0;
}
#else
int main(int argc, char** argv) { (void)argc; (void)argv; puts("Settings migration is Windows-only."); return 0; }
#endif
