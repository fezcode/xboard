#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "app_state.h"
#include "audio.h"
#include "cli.h"
#include "controller.h"
#include "font.h"
#include "morse.h"
#include "send_input.h"
#include "ui_draw.h"
#include "app_shell.h"

#ifdef _WIN32
#include <SDL_syswm.h>
#include <windows.h>

static WNDPROC g_old_wndproc = NULL;
static HWND g_self_hwnd = NULL;

static LRESULT CALLBACK OskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_MOUSEACTIVATE) {
        return MA_NOACTIVATE;
    }
    return CallWindowProc(g_old_wndproc, hwnd, uMsg, wParam, lParam);
}

/* SDL2main links the application as a GUI-subsystem binary, so it starts with no
 * console of its own and --help would write into the void. Borrow the console of
 * whichever shell launched us, leaving any stream the user already redirected to
 * a pipe or file alone. */
static void attach_parent_console(void) {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) return;
    const DWORD handles[] = { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };
    FILE* streams[] = { stdout, stderr };
    for (int i = 0; i < 2; ++i) {
        HANDLE h = GetStdHandle(handles[i]);
        if (h && h != INVALID_HANDLE_VALUE && GetFileType(h) != FILE_TYPE_UNKNOWN) continue;
        freopen("CONOUT$", "w", streams[i]);
    }
}

static void setup_win32_osk(SDL_Window* window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        HWND hwnd = wmInfo.info.win.window;
        g_self_hwnd = hwnd;
        HINSTANCE instance = GetModuleHandleW(NULL);
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32, LR_SHARED));
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_SHARED));

        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_NOACTIVATE | WS_EX_TOPMOST;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

        g_old_wndproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)OskWndProc);

        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        send_input_set_target(NULL, hwnd);
    }
}
#endif

/* Forward declarations of mode renderers/updaters */
void mode_dial_update(AppState* state, float dt);
void mode_dial_render(AppState* state);

void mode_grid_update(AppState* state, float dt);
void mode_grid_render(AppState* state);

void mode_morse_update(AppState* state, float dt);
void mode_morse_render(AppState* state);

int main(int argc, char* argv[]) {
    /* Resolve the command line before touching SDL so --help and --version stay
     * cheap, and so an unusable argument reports itself instead of opening a
     * window nobody can see. */
    CliOptions opts;
    bool parsed = cli_parse(argc, argv, &opts);
#ifdef _WIN32
    if (!parsed || opts.help || opts.version || opts.screenshot) attach_parent_console();
#endif
    if (!parsed) {
        fprintf(stderr, "xboard: %s\n\n", opts.message);
        cli_print_usage(stderr);
        return 2;
    }
    if (opts.help) { cli_print_usage(stdout); return 0; }
    if (opts.version) { printf("xboard %s\n", XBOARD_VERSION); return 0; }

#ifdef _WIN32
    /* Set Windows Per-Monitor V2 DPI Awareness before any windows are created
     * so Windows DWM does not stretch/blur our UI on High-DPI displays (125%/150%) */
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
        SetProcessDpiAwarenessContextFunc setDpiContext =
            (SetProcessDpiAwarenessContextFunc)(void*)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (setDpiContext) {
            setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            typedef BOOL (WINAPI *SetProcessDPIAwareFunc)(void);
            SetProcessDPIAwareFunc setDPIAware =
                (SetProcessDPIAwareFunc)(void*)GetProcAddress(user32, "SetProcessDPIAware");
            if (setDPIAware) setDPIAware();
        }
    }
#endif

    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3");

    theme_init();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    controller_init();

    AudioEngine* audio = audio_init();
    FontManager* fonts = font_init();
    if (!fonts) {
        fprintf(stderr, "Failed to initialize fonts!\n");
    }

    SDL_Window* window = SDL_CreateWindow(
        "xboard " XBOARD_VERSION " - Controller Keyboard",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1200, 900,
        (opts.screenshot ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN) | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        font_shutdown(fonts); audio_shutdown(audio); controller_shutdown(); SDL_Quit();
        return 1;
    }

#ifdef _WIN32
    if (!opts.screenshot) setup_win32_osk(window);
#endif

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer || !fonts) {
        fprintf(stderr, "Unable to initialize renderer or fonts: %s\n", SDL_GetError());
        font_shutdown(fonts); audio_shutdown(audio); controller_shutdown();
        if (renderer) SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }
    SDL_SetWindowMinimumSize(window, XBOARD_MIN_WIN_W, XBOARD_MIN_WIN_H);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    AppState state;
    memset(&state, 0, sizeof(AppState));
    state.window = window;
    state.renderer = renderer;
    state.win_w = 1200;
    state.win_h = 900;
    state.running = true;
    state.mode = MODE_GRID;
    state.direct_send_input = false; /* Each session starts in local compose mode. */
    state.sound_enabled = true;
    state.audio = audio;
    state.fonts = fonts;

    /* Initialize dial */
    state.dial.active_sector = 0;
    state.dial.active_pick = -1;

    if (!opts.screenshot) app_load_preferences(&state);
    audio_set_enabled(audio, state.sound_enabled);

    if (opts.screenshot) {
        if (opts.compact) {
            state.win_w = XBOARD_MIN_WIN_W; state.win_h = XBOARD_MIN_WIN_H;
            SDL_SetWindowSize(window, state.win_w, state.win_h);
        }
        const char* filenames[3] = { "screenshot_dial.bmp", "screenshot_grid.bmp", "screenshot_morse.bmp" };
        for (int m = 0; m < 3; ++m) {
            state.mode = (AppMode)m;
            theme_set(0);
            state.theme_index = 0;
            SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
            SDL_RenderClear(renderer);
            shell_draw_header(&state);
            shell_draw_composer(&state);
            switch (state.mode) {
                case MODE_DIAL:  mode_dial_render(&state); break;
                case MODE_GRID:  mode_grid_render(&state); break;
                case MODE_MORSE: mode_morse_render(&state); break;
                default: break;
            }
            shell_draw_footer(&state);
            shell_draw_overlay(&state);

            SDL_Surface* sshot = SDL_CreateRGBSurfaceWithFormat(0, state.win_w, state.win_h, 32, SDL_PIXELFORMAT_ARGB8888);
            if (sshot) {
                SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
                SDL_SaveBMP(sshot, filenames[m]);
                SDL_FreeSurface(sshot);
                printf("Saved %s\n", filenames[m]);
            }
            SDL_RenderPresent(renderer);
        }

        /* Also capture screenshots for each theme */
        struct { int theme; AppMode mode; const char* fn; } extra[] = {
            { 1, MODE_DIAL, "screenshot_theme1_cyber.bmp" },
            { 2, MODE_GRID, "screenshot_theme2_nordic.bmp" },
            { 3, MODE_DIAL, "screenshot_theme3_titanium.bmp" }
        };
        for (int i = 0; i < 3; ++i) {
            state.mode = extra[i].mode;
            theme_set(extra[i].theme);
            state.theme_index = extra[i].theme;
            SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
            SDL_RenderClear(renderer);
            shell_draw_header(&state);
            shell_draw_composer(&state);
            switch (state.mode) {
                case MODE_DIAL:  mode_dial_render(&state); break;
                case MODE_GRID:  mode_grid_render(&state); break;
                case MODE_MORSE: mode_morse_render(&state); break;
                default: break;
            }
            shell_draw_footer(&state);
            shell_draw_overlay(&state);

            SDL_Surface* sshot = SDL_CreateRGBSurfaceWithFormat(0, state.win_w, state.win_h, 32, SDL_PIXELFORMAT_ARGB8888);
            if (sshot) {
                SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
                SDL_SaveBMP(sshot, extra[i].fn);
                SDL_FreeSurface(sshot);
                printf("Saved %s\n", extra[i].fn);
            }
            SDL_RenderPresent(renderer);
        }

        font_shutdown(fonts);
        audio_shutdown(audio);
        controller_shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    AppMode previous_mode = state.mode;
    Uint32 prev_time = SDL_GetTicks();

    while (state.running) {
        Uint32 cur_time = SDL_GetTicks();
        float dt = (cur_time - prev_time) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        prev_time = cur_time;

#ifdef _WIN32
        HWND cur_fg = GetForegroundWindow();
        if (cur_fg && cur_fg != g_self_hwnd) {
            send_input_set_target((void*)cur_fg, (void*)g_self_hwnd);
        }
#endif

        if (state.toast_timer > 0.0f) {
            state.toast_timer -= dt;
        }

        /* Reset per-frame mouse click triggers */
        state.mouse_moved = false;
        state.mouse.left_clicked = false;
        state.mouse.right_clicked = false;
        state.mouse.wheel_y = 0;

        /* Update window size */
        SDL_GetWindowSize(window, &state.win_w, &state.win_h);

        audio_set_insertion_only(audio, state.mode == MODE_DIAL);

        /* Handle SDL Events */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                state.running = false;
            } else if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    state.win_w = ev.window.data1;
                    state.win_h = ev.window.data2;
                }
            } else if (ev.type == SDL_MOUSEMOTION) {
                state.mouse_moved = true;
                state.mouse.x = ev.motion.x;
                state.mouse.y = ev.motion.y;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                state.mouse.x = ev.button.x;
                state.mouse.y = ev.button.y;
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    state.mouse.left_down = true;
                    state.mouse.left_clicked = true;
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    state.mouse.right_down = true;
                    state.mouse.right_clicked = true;
                }
            } else if (ev.type == SDL_MOUSEBUTTONUP) {
                state.mouse.x = ev.button.x;
                state.mouse.y = ev.button.y;
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    state.mouse.left_down = false;
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    state.mouse.right_down = false;
                }
            } else if (ev.type == SDL_MOUSEWHEEL) {
                state.mouse.wheel_y = ev.wheel.y;
            } else if (ev.type == SDL_KEYDOWN) {
                /* PC Keyboard fallback for convenience */
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    if (state.help_open) state.help_open = false;
                    else state.running = false;
                } else if (ev.key.keysym.sym == SDLK_1) {
                    state.mode = MODE_DIAL;
                    app_set_toast(&state, "Mode: Dual-Stick Dial", 2.0f);
                } else if (ev.key.keysym.sym == SDLK_2) {
                    state.mode = MODE_GRID;
                    app_set_toast(&state, "Mode: Virtual Grid", 2.0f);
                } else if (ev.key.keysym.sym == SDLK_3) {
                    state.mode = MODE_MORSE;
                    app_set_toast(&state, "Mode: Morse Code", 2.0f);
                } else if (ev.key.keysym.sym == SDLK_TAB) {
                    state.mode = (state.mode + 1) % MODE_COUNT;
                } else if (ev.key.keysym.sym == SDLK_BACKSPACE && !ev.key.repeat) {
                    if (ev.key.keysym.mod & KMOD_CTRL) {
                        app_delete_word(&state);
                    } else {
                        app_backspace(&state);
                    }
                } else if (ev.key.keysym.sym == SDLK_SPACE && !ev.key.repeat) {
                    app_space(&state);
                } else if (ev.key.keysym.sym == SDLK_RETURN && !ev.key.repeat) {
                    app_newline(&state);
                } else if (ev.key.keysym.sym == SDLK_LEFT) {
                    app_cursor_left(&state);
                } else if (ev.key.keysym.sym == SDLK_RIGHT) {
                    app_cursor_right(&state);
                } else if (ev.key.keysym.sym == SDLK_UP) {
                    app_cursor_up(&state);
                } else if (ev.key.keysym.sym == SDLK_DOWN) {
                    app_cursor_down(&state);
                } else if (ev.key.keysym.sym == SDLK_F1) {
                    state.help_open = !state.help_open;
                } else if (ev.key.keysym.sym == SDLK_F2) {
                    app_cycle_theme(&state);
                } else if (ev.key.keysym.sym == SDLK_LGUI || ev.key.keysym.sym == SDLK_RGUI) {
                    app_win_key(&state);
                }
            }
            controller_handle_event(&state, &ev);
        }

        /* Update Controller */
        controller_update(&state, dt);

        shell_update(&state);
        audio_set_insertion_only(audio, state.mode == MODE_DIAL);
        bool mode_input_blocked = shell_controller_update(&state, dt);
        if (state.help_open) {
            state.ctrl.buttons_pressed = state.ctrl.buttons_held = 0;
            state.ctrl.lt = state.ctrl.rt = 0;
        }

        shell_global_shortcuts(&state, dt, mode_input_blocked);

        if (state.mode != previous_mode) {
            memset(&state.morse, 0, sizeof(state.morse));
            state.dial.was_picked = false;
            state.dial.active_pick = -1;
            previous_mode = state.mode;
        }

        /* Mode-specific updates */
        if (!state.help_open && !mode_input_blocked) switch (state.mode) {
            case MODE_DIAL:  mode_dial_update(&state, dt); break;
            case MODE_GRID:  mode_grid_update(&state, dt); break;
            case MODE_MORSE: mode_morse_update(&state, dt); break;
            default: break;
        }

        /* --- Rendering --- */
        SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, 255);
        SDL_RenderClear(renderer);

        shell_draw_header(&state);
        shell_draw_composer(&state);

        /* Render Active Mode */
        switch (state.mode) {
            case MODE_DIAL:  mode_dial_render(&state); break;
            case MODE_GRID:  mode_grid_render(&state); break;
            case MODE_MORSE: mode_morse_render(&state); break;
            default: break;
        }

        shell_draw_footer(&state);
        shell_draw_overlay(&state);

        SDL_RenderPresent(renderer);
    }

    app_save_preferences(&state);
    font_shutdown(fonts);
    audio_shutdown(audio);
    controller_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
