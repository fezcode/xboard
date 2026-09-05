# xboard Studio

<img src="assets/xboard.png" width="128" alt="xboard red and blue keycap icon">

A native Windows virtual keyboard for Xbox controllers and a mouse. Built in C11 with SDL2, SDL_ttf and XInput.

![Keyboard preview](docs/keyboard.png)

## Start here

```powershell
.\build.ps1 -Test -Run
```

Requires CMake 3.20+, Ninja, a C11 compiler, pkg-config, SDL2 and SDL2_ttf. The existing MinGW/MSYS2 environment is supported. Keep its `mingw64/bin` directory on PATH when building and running so Windows can find the runtime DLLs.

The executable is `build-studio/xboard.exe`. The build script resolves paths from its own directory, stops on configuration/build/test failures, and accepts `-Config Debug` and `-BuildDir <path>`.

## What's new

- Redesigned composer, mode navigation, focused key states, status indicators, help overlay and four coordinated dark themes.
- Local compose mode by default. Enable **Direct to apps** to type into the focused destination window.
- Real 32-step undo/redo, reversible clear, select-all replacement, word deletion at the cursor and UTF-8-aware character movement/deletion.
- Unicode clipboard copy/paste and Unicode direct text output. Paste and quick phrases are single undo operations. Oversized inserts are rejected without truncating text.
- Quick phrases, working sound toggle, and saved theme, mode and sound preferences.
- Press **L3 + R3 together** to focus quick phrases. Use either stick horizontally or D-pad left/right to choose; press **A** to insert. Press **L3 + R3 again** to return. Focus stays on phrases after insertion. Release both stick buttons between toggles.
- Corrected D-pad grid navigation, duplicated R3 toggling, conflicting L3 actions and mouse hover overriding controller focus.
- Text texture caching, separate editor/actions/interface modules and automated regression tests.

## Input modes

**Keyboard:** the left stick moves the red L selector; the right stick moves the blue R selector. Press A to activate the selector moved most recently. The D-pad moves the active selector. Fresh stick gestures take priority over repeats; exact simultaneous movements keep the previous active selector. Click keys with the mouse without moving either selector. Shift and symbols expose alternate characters.

**Dual dial:** choose one of eight groups with the left stick, then choose a character with the right stick. Flick the right stick to type once, then return it to center to re-arm. A also types the selected character. Left-stick group selection and gentle right-stick previews are silent. Both dials support clicking. In dual-dial mode, sound plays only after text is successfully inserted; navigation, deletion and other controls stay silent.

**Morse:** left stick for a dot, right stick for a dash. Press A to commit or wait 0.75 seconds. B removes an unfinished symbol first. Click either pad or an alphabet entry. Audio uses an 820 Hz tone with 75 ms dots and 225 ms dashes.

| Control | Action |
| --- | --- |
| A | Select / commit |
| B | Backspace; hold to repeat |
| X / Y | Space / Enter |
| LB / RB | Previous / next mode |
| LT / RT | Shift / Ctrl shortcuts |
| L3 + R3 | Toggle quick-phrase focus |
| L3 / R3 alone | Caps lock / symbols (on release) |
| View | Switch local/direct output |
| Menu | Copy composer |
| Guide | Windows key in direct mode |
| D-pad | Grid navigation; cursor movement in other modes |

With application keyboard focus: 1/2/3 select modes, Tab cycles modes, F1 toggles help, F2 changes theme, arrow keys move the text cursor, and Escape dismisses help or exits. The window uses Windows' no-activation behavior so clicking virtual keys preserves the destination app's focus.

## Composer behavior

The composer displays the cursor's current line and scrolls horizontally to keep it visible. Copy includes the complete text, including newlines. Capacity is 4,095 UTF-8 bytes. Character navigation follows Unicode code points, not combined grapheme clusters.

Local Ctrl+A selects all; the next insertion replaces it. Ctrl+C/V/X/Z/Y copy, paste, cut, undo and redo. Direct mode forwards controller Ctrl shortcuts to the destination app. The toolbar's Copy/Paste/Clear operate on the composer; Undo/Redo target the destination app when direct mode is enabled. The live output is a record of emitted text, not a synchronized view of the external document. External apps control their own undo history and may reject injected input, especially when elevated.

Preferences are stored in `SDL_GetPrefPath("xboard", "xboard")/preferences.ini` on normal exit. Composer text and direct-output mode are never persisted. Quick phrases are built-in presets.

## Development and verification

- `src/text_buffer.*`: platform-independent editor and bounded snapshot history.
- `src/app_actions.c`: application commands, clipboard integration and preferences.
- `src/app_shell.*`: interface rendering and shell hit testing.
- `src/mode_*.c`: mode-specific input and rendering.
- `src/font.c`, `ui_draw.c`, `audio.c`, `controller.c`, `send_input.c`: platform and rendering services.

```powershell
.\build.ps1 -Test
.\build-studio\xboard.exe --screenshot
.\build-studio\xboard.exe --screenshot --compact
```

Screenshot mode creates six BMPs in the working directory, runs hidden, does not load/save preferences and does not inject keystrokes. Compact captures use the 1100 x 860 minimum window size; default captures are 1200 x 900.

Tests cover insertion/deletion, UTF-8 boundaries, capacity, undo/redo eviction, multiline navigation, Morse decoding, toolbar actions, selection replacement, quick phrases, help, grid navigation and Morse auto-commit. Physical Xbox rumble/hotplug and cross-application SendInput require manual hardware/application testing.

The Windows executable embeds `assets/xboard.ico` with seven sizes (16–256 px). Regenerate it from the PNG with `tools/build-icon.ps1`.
