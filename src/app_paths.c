#include "app_paths.h"
#include <SDL.h>

char* app_settings_dir(void) {
    /* On Windows SDL resolves this to %APPDATA%\<org>\<app>\ and creates it. */
    return SDL_GetPrefPath(XBOARD_DATA_VENDOR, XBOARD_DATA_APP);
}

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>

#define PATH_CAP 1024

static bool join(wchar_t* out, const wchar_t* a, const wchar_t* b) {
    int n = _snwprintf(out, PATH_CAP, L"%ls\\%ls", a, b);
    return n > 0 && n < PATH_CAP;
}

static bool exists(const wchar_t* path) {
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

static bool is_dir(const wchar_t* path) {
    DWORD a = GetFileAttributesW(path);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

static void delete_file(const wchar_t* path) {
    SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);   /* A read-only file refuses deletion. */
    DeleteFileW(path);
}

/* Removes a stale old folder whose name is already taken in the new location.
 * Reparse points are removed as links, never followed. */
static void delete_tree(const wchar_t* dir, int depth) {
    if (depth > 16) return;
    wchar_t pattern[PATH_CAP];
    if (!join(pattern, dir, L"*")) return;
    WIN32_FIND_DATAW fd;
    HANDLE find = FindFirstFileW(pattern, &fd);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
            wchar_t child[PATH_CAP];
            if (!join(child, dir, fd.cFileName)) continue;
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) delete_file(child);
            else if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) RemoveDirectoryW(child);
            else delete_tree(child, depth + 1);
        } while (FindNextFileW(find, &fd));
        FindClose(find);
    }
    RemoveDirectoryW(dir);
}

/* Moves each entry of src into dst. Anything already in dst wins and the stale
 * src copy is deleted instead; dst is never overwritten. Entries that cannot be
 * moved or deleted stay behind, so src then survives non-empty. */
static void merge_into(const wchar_t* src, const wchar_t* dst, int depth) {
    if (depth > 16) return;
    if (!is_dir(dst) && !CreateDirectoryW(dst, NULL)) return;
    wchar_t pattern[PATH_CAP];
    if (!join(pattern, src, L"*")) return;
    WIN32_FIND_DATAW fd;
    HANDLE find = FindFirstFileW(pattern, &fd);
    if (find == INVALID_HANDLE_VALUE) return;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        wchar_t from[PATH_CAP], to[PATH_CAP];
        if (!join(from, src, fd.cFileName) || !join(to, dst, fd.cFileName)) continue;
        bool folder = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                      !(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
        if (folder) {
            /* A rename moves the whole subtree at once when the name is free. */
            if (!exists(to) && MoveFileExW(from, to, 0)) continue;
            if (exists(to) && !is_dir(to)) { delete_tree(from, depth + 1); continue; }
            merge_into(from, to, depth + 1);
            RemoveDirectoryW(from);   /* Only succeeds once emptied. */
        } else if (exists(to)) {
            /* The new location already has it: that copy wins, drop the stale one. */
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RemoveDirectoryW(from);
            else delete_file(from);
        } else {
            /* Without MOVEFILE_REPLACE_EXISTING this refuses to overwrite;
             * COPY_ALLOWED falls back to copy-then-delete across volumes. */
            MoveFileExW(from, to, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
        }
    } while (FindNextFileW(find, &fd));
    FindClose(find);
}

bool app_migrate_dir(const wchar_t* old_dir, const wchar_t* new_dir, const wchar_t* old_parent) {
    if (!is_dir(old_dir)) return true;
    if (!exists(new_dir) && MoveFileExW(old_dir, new_dir, 0)) {
        /* Renamed in one step. */
    } else {
        merge_into(old_dir, new_dir, 0);
        RemoveDirectoryW(old_dir);
    }
    if (old_parent) RemoveDirectoryW(old_parent);   /* Only if nothing else is there. */
    return !exists(old_dir);
}

void app_migrate_legacy_settings(void) {
    wchar_t appdata[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdata))) return;
    wchar_t old_parent[PATH_CAP], old_dir[PATH_CAP], vendor[PATH_CAP], new_dir[PATH_CAP];
    if (!join(old_parent, appdata, L"xboard") || !join(old_dir, old_parent, L"xboard")) return;
    if (!is_dir(old_dir)) return;
    if (!join(vendor, appdata, L"" XBOARD_DATA_VENDOR) || !join(new_dir, vendor, L"" XBOARD_DATA_APP)) return;
    if (!is_dir(vendor) && !CreateDirectoryW(vendor, NULL)) return;
    if (!app_migrate_dir(old_dir, new_dir, old_parent))
        SDL_Log("Some settings in %%APPDATA%%\\xboard\\xboard could not be moved and were left in place");
}
#else
void app_migrate_legacy_settings(void) {}
#endif
