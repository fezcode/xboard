#ifndef APP_PATHS_H
#define APP_PATHS_H
#include <stdbool.h>

/* Per-user data follows the Fezcode layout so an uninstall can purge it in one
 * place: settings in %APPDATA%\fezcode\xboard. xboard keeps no logs or caches,
 * so nothing lives under %LOCALAPPDATA% today; anything like that belongs in
 * %LOCALAPPDATA%\fezcode\xboard. Must match settings_dirs in forge.toml. */
#define XBOARD_DATA_VENDOR "fezcode"
#define XBOARD_DATA_APP    "xboard"

/* Settings folder with a trailing separator, created on demand. Free with
 * SDL_free. NULL when the folder cannot be resolved. */
char* app_settings_dir(void);

/* Moves settings left by releases up to 2.0.4 in their folder
 * (%APPDATA%\xboard\xboard) into app_settings_dir. Call once at startup before
 * anything reads settings. Never overwrites (the new copy wins and the stale
 * old one is deleted), never fails startup and is a no-op once the old folder
 * is gone. */
void app_migrate_legacy_settings(void);

#ifdef _WIN32
#include <wchar.h>
/* The mechanism behind app_migrate_legacy_settings, exposed for tests. Moves
 * everything in old_dir into new_dir (whole-folder rename when new_dir does not
 * exist yet, else entry by entry, copy-then-delete across volumes). An entry
 * that already exists in new_dir is kept and the old copy deleted. Removes old_dir once empty, then
 * old_parent too when non-NULL and empty. Returns true when old_dir is gone. */
bool app_migrate_dir(const wchar_t* old_dir, const wchar_t* new_dir, const wchar_t* old_parent);
#endif

#endif /* APP_PATHS_H */
