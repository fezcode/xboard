#include "send_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

static HWND g_target_hwnd = NULL;
static HWND g_self_hwnd = NULL;

void send_input_set_target(void* hwnd, void* self_hwnd) {
    if (hwnd) g_target_hwnd = (HWND)hwnd;
    if (self_hwnd) g_self_hwnd = (HWND)self_hwnd;
}

void send_input_ensure_target(void) {
    HWND fg = GetForegroundWindow();
    if (fg == g_self_hwnd && g_target_hwnd && IsWindow(g_target_hwnd)) {
        DWORD selfThread = GetCurrentThreadId();
        DWORD targetThread = GetWindowThreadProcessId(g_target_hwnd, NULL);
        AttachThreadInput(selfThread, targetThread, TRUE);
        SetForegroundWindow(g_target_hwnd);
        SetFocus(g_target_hwnd);
        AttachThreadInput(selfThread, targetThread, FALSE);
        Sleep(10);
    }
}

void send_input_char(char c) {
    if (c == '\n' || c == '\r') {
        send_input_enter();
        return;
    }
    if (c == '\b') {
        send_input_backspace();
        return;
    }
    if (c == ' ') {
        send_input_space();
        return;
    }
    send_input_ensure_target();
    INPUT inputs[2];
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = (WCHAR)(unsigned char)c;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = (WCHAR)(unsigned char)c;
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}

void send_input_backspace(void) {
    send_input_ensure_target();
    WORD vsc = (WORD)MapVirtualKeyW(VK_BACK, MAPVK_VK_TO_VSC);
    if (!vsc) vsc = 0x0E;

    INPUT in[2];
    ZeroMemory(in, sizeof(in));

    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_BACK;
    in[0].ki.wScan = vsc;
    in[0].ki.dwFlags = 0;

    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = VK_BACK;
    in[1].ki.wScan = vsc;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;

    if (SendInput(2, in, sizeof(INPUT)) == 0) {
        keybd_event(VK_BACK, (BYTE)vsc, 0, 0);
        keybd_event(VK_BACK, (BYTE)vsc, KEYEVENTF_KEYUP, 0);
    }
}

void send_input_enter(void) {
    send_input_ensure_target();
    WORD vsc = (WORD)MapVirtualKeyW(VK_RETURN, MAPVK_VK_TO_VSC);
    if (!vsc) vsc = 0x1C;

    INPUT in[2];
    ZeroMemory(in, sizeof(in));

    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_RETURN;
    in[0].ki.wScan = vsc;
    in[0].ki.dwFlags = 0;

    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = VK_RETURN;
    in[1].ki.wScan = vsc;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;

    if (SendInput(2, in, sizeof(INPUT)) == 0) {
        keybd_event(VK_RETURN, (BYTE)vsc, 0, 0);
        keybd_event(VK_RETURN, (BYTE)vsc, KEYEVENTF_KEYUP, 0);
    }
}

void send_input_space(void) {
    send_input_ensure_target();
    WORD vsc = (WORD)MapVirtualKeyW(VK_SPACE, MAPVK_VK_TO_VSC);
    if (!vsc) vsc = 0x39;

    INPUT in[2];
    ZeroMemory(in, sizeof(in));

    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_SPACE;
    in[0].ki.wScan = vsc;
    in[0].ki.dwFlags = 0;

    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = VK_SPACE;
    in[1].ki.wScan = vsc;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;

    if (SendInput(2, in, sizeof(INPUT)) == 0) {
        keybd_event(VK_SPACE, (BYTE)vsc, 0, 0);
        keybd_event(VK_SPACE, (BYTE)vsc, KEYEVENTF_KEYUP, 0);
    }
}

static void send_input_arrow_internal(WORD vk, bool shift, bool ctrl) {
    send_input_ensure_target();
    WORD sc = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);

    /* Modifiers down */
    if (ctrl) {
        WORD ctrl_sc = (WORD)MapVirtualKeyW(VK_CONTROL, MAPVK_VK_TO_VSC);
        INPUT cin = {0};
        cin.type = INPUT_KEYBOARD;
        cin.ki.wVk = VK_CONTROL;
        cin.ki.wScan = ctrl_sc ? ctrl_sc : 0x1D;
        SendInput(1, &cin, sizeof(INPUT));
    }
    if (shift) {
        WORD shift_sc = (WORD)MapVirtualKeyW(VK_SHIFT, MAPVK_VK_TO_VSC);
        INPUT sin = {0};
        sin.type = INPUT_KEYBOARD;
        sin.ki.wVk = VK_SHIFT;
        sin.ki.wScan = shift_sc ? shift_sc : 0x2A;
        SendInput(1, &sin, sizeof(INPUT));
    }

    /* Arrow down & up (EXTENDED KEY) */
    INPUT in[2] = {0};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = vk;
    in[0].ki.wScan = sc;
    in[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = vk;
    in[1].ki.wScan = sc;
    in[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;

    if (SendInput(2, in, sizeof(INPUT)) == 0) {
        keybd_event((BYTE)vk, (BYTE)sc, KEYEVENTF_EXTENDEDKEY, 0);
        keybd_event((BYTE)vk, (BYTE)sc, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }

    /* Modifiers up */
    if (shift) {
        INPUT sin = {0};
        sin.type = INPUT_KEYBOARD;
        sin.ki.wVk = VK_SHIFT;
        sin.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &sin, sizeof(INPUT));
    }
    if (ctrl) {
        INPUT cin = {0};
        cin.type = INPUT_KEYBOARD;
        cin.ki.wVk = VK_CONTROL;
        cin.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &cin, sizeof(INPUT));
    }
}

void send_input_left(bool shift, bool ctrl) {
    send_input_arrow_internal(VK_LEFT, shift, ctrl);
}

void send_input_right(bool shift, bool ctrl) {
    send_input_arrow_internal(VK_RIGHT, shift, ctrl);
}

void send_input_up(bool shift, bool ctrl) {
    send_input_arrow_internal(VK_UP, shift, ctrl);
}

void send_input_down(bool shift, bool ctrl) {
    send_input_arrow_internal(VK_DOWN, shift, ctrl);
}

void send_input_string(const char* str) {
    if (!str) return;
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
    if (!n) return;
    WCHAR* wide = (WCHAR*)malloc((size_t)n * sizeof(WCHAR));
    if (!wide) return;
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, wide, n);
    send_input_ensure_target();
    for (int i = 0; i < n - 1; ++i) {
        if (wide[i] == '\r') continue;
        if (wide[i] == '\n') { send_input_enter(); continue; }
        if (wide[i] == '\t') {
            INPUT keys[2] = {0}; keys[0].type = keys[1].type = INPUT_KEYBOARD;
            keys[0].ki.wVk = keys[1].ki.wVk = VK_TAB; keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(2, keys, sizeof(INPUT)); continue;
        }
        INPUT keys[2] = {0}; keys[0].type = keys[1].type = INPUT_KEYBOARD;
        keys[0].ki.wScan = keys[1].ki.wScan = wide[i];
        keys[0].ki.dwFlags = KEYEVENTF_UNICODE;
        keys[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        SendInput(2, keys, sizeof(INPUT));
    }
    free(wide);
}

void send_input_ctrl_combo(char c) {
    send_input_ensure_target();
    WORD vkey = 0;
    if (c == '\b') {
        vkey = VK_BACK;
    } else if (c == '\n' || c == '\r') {
        vkey = VK_RETURN;
    } else {
        SHORT vk = VkKeyScanA(c);
        vkey = (vk != -1) ? (WORD)(vk & 0xFF) : (WORD)toupper((unsigned char)c);
    }

    WORD ctrl_sc = (WORD)MapVirtualKeyW(VK_CONTROL, MAPVK_VK_TO_VSC);
    if (!ctrl_sc) ctrl_sc = 0x1D;

    WORD key_sc = (WORD)MapVirtualKeyW(vkey, MAPVK_VK_TO_VSC);

    /* 1. Ctrl Down */
    INPUT ctrl_down = {0};
    ctrl_down.type = INPUT_KEYBOARD;
    ctrl_down.ki.wVk = VK_CONTROL;
    ctrl_down.ki.wScan = ctrl_sc;
    ctrl_down.ki.dwFlags = 0;
    if (SendInput(1, &ctrl_down, sizeof(INPUT)) == 0) {
        keybd_event(VK_CONTROL, (BYTE)ctrl_sc, 0, 0);
    }

    Sleep(15);

    /* 2. Key Down & Up */
    INPUT key_events[2] = {0};
    key_events[0].type = INPUT_KEYBOARD;
    key_events[0].ki.wVk = vkey;
    key_events[0].ki.wScan = key_sc;
    key_events[0].ki.dwFlags = 0;

    key_events[1].type = INPUT_KEYBOARD;
    key_events[1].ki.wVk = vkey;
    key_events[1].ki.wScan = key_sc;
    key_events[1].ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(2, key_events, sizeof(INPUT)) == 0) {
        keybd_event((BYTE)vkey, (BYTE)key_sc, 0, 0);
        keybd_event((BYTE)vkey, (BYTE)key_sc, KEYEVENTF_KEYUP, 0);
    }

    Sleep(15);

    /* 3. Ctrl Up */
    INPUT ctrl_up = {0};
    ctrl_up.type = INPUT_KEYBOARD;
    ctrl_up.ki.wVk = VK_CONTROL;
    ctrl_up.ki.wScan = ctrl_sc;
    ctrl_up.ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(1, &ctrl_up, sizeof(INPUT)) == 0) {
        keybd_event(VK_CONTROL, (BYTE)ctrl_sc, KEYEVENTF_KEYUP, 0);
    }
}

void send_input_win_key(void) {
    send_input_ensure_target();
    WORD vsc = (WORD)MapVirtualKeyW(VK_LWIN, MAPVK_VK_TO_VSC);
    if (!vsc) vsc = 0x5B;

    INPUT in[2] = {0};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_LWIN;
    in[0].ki.wScan = vsc;
    in[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = VK_LWIN;
    in[1].ki.wScan = vsc;
    in[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;

    if (SendInput(2, in, sizeof(INPUT)) == 0) {
        keybd_event(VK_LWIN, (BYTE)vsc, KEYEVENTF_EXTENDEDKEY, 0);
        keybd_event(VK_LWIN, (BYTE)vsc, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
}

void send_input_win_down(void) {
    send_input_ensure_target();
    WORD vsc = (WORD)MapVirtualKeyW(VK_LWIN, MAPVK_VK_TO_VSC);
    if (!vsc) vsc = 0x5B;

    INPUT in = {0};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_LWIN;
    in.ki.wScan = vsc;
    in.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

    if (SendInput(1, &in, sizeof(INPUT)) == 0) {
        keybd_event(VK_LWIN, (BYTE)vsc, KEYEVENTF_EXTENDEDKEY, 0);
    }
}

void send_input_win_up(void) {
    WORD vsc = (WORD)MapVirtualKeyW(VK_LWIN, MAPVK_VK_TO_VSC);
    if (!vsc) vsc = 0x5B;

    INPUT in = {0};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_LWIN;
    in.ki.wScan = vsc;
    in.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;

    if (SendInput(1, &in, sizeof(INPUT)) == 0) {
        keybd_event(VK_LWIN, (BYTE)vsc, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
}

void send_input_win_combo(char c) {
    send_input_ensure_target();
    WORD vkey = 0;
    SHORT vk = VkKeyScanA(c);
    vkey = (vk != -1) ? (WORD)(vk & 0xFF) : (WORD)toupper((unsigned char)c);
    WORD key_sc = (WORD)MapVirtualKeyW(vkey, MAPVK_VK_TO_VSC);
    WORD win_sc = (WORD)MapVirtualKeyW(VK_LWIN, MAPVK_VK_TO_VSC);
    if (!win_sc) win_sc = 0x5B;

    /* 1. Win Down */
    INPUT win_down = {0};
    win_down.type = INPUT_KEYBOARD;
    win_down.ki.wVk = VK_LWIN;
    win_down.ki.wScan = win_sc;
    win_down.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    if (SendInput(1, &win_down, sizeof(INPUT)) == 0) {
        keybd_event(VK_LWIN, (BYTE)win_sc, KEYEVENTF_EXTENDEDKEY, 0);
    }

    Sleep(15);

    /* 2. Key Down & Up */
    INPUT key_events[2] = {0};
    key_events[0].type = INPUT_KEYBOARD;
    key_events[0].ki.wVk = vkey;
    key_events[0].ki.wScan = key_sc;
    key_events[0].ki.dwFlags = 0;

    key_events[1].type = INPUT_KEYBOARD;
    key_events[1].ki.wVk = vkey;
    key_events[1].ki.wScan = key_sc;
    key_events[1].ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(2, key_events, sizeof(INPUT)) == 0) {
        keybd_event((BYTE)vkey, (BYTE)key_sc, 0, 0);
        keybd_event((BYTE)vkey, (BYTE)key_sc, KEYEVENTF_KEYUP, 0);
    }

    Sleep(15);

    /* 3. Win Up */
    INPUT win_up = {0};
    win_up.type = INPUT_KEYBOARD;
    win_up.ki.wVk = VK_LWIN;
    win_up.ki.wScan = win_sc;
    win_up.ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
    if (SendInput(1, &win_up, sizeof(INPUT)) == 0) {
        keybd_event(VK_LWIN, (BYTE)win_sc, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
}

bool copy_to_clipboard(const char* text) {
    return text && SDL_SetClipboardText(text) == 0;
}
bool paste_from_clipboard(char* out_buf, size_t max_len) {
    if (!out_buf || !max_len || !SDL_HasClipboardText()) return false;
    char* text = SDL_GetClipboardText(); if (!text) return false;
    size_t n = strlen(text);
    if (n >= max_len) { SDL_free(text); return false; }
    memcpy(out_buf, text, n + 1); SDL_free(text); return true;
}

#else

void send_input_set_target(void* hwnd, void* self_hwnd) { (void)hwnd; (void)self_hwnd; }
void send_input_ensure_target(void) {}
void send_input_char(char c) { (void)c; }
void send_input_ctrl_combo(char c) { (void)c; }
void send_input_win_key(void) {}
void send_input_win_combo(char c) { (void)c; }
void send_input_win_down(void) {}
void send_input_win_up(void) {}
void send_input_backspace(void) {}
void send_input_enter(void) {}
void send_input_space(void) {}
void send_input_left(bool shift, bool ctrl) { (void)shift; (void)ctrl; }
void send_input_right(bool shift, bool ctrl) { (void)shift; (void)ctrl; }
void send_input_up(bool shift, bool ctrl) { (void)shift; (void)ctrl; }
void send_input_down(bool shift, bool ctrl) { (void)shift; (void)ctrl; }
void send_input_string(const char* str) { (void)str; }
bool copy_to_clipboard(const char* text) { (void)text; return false; }
bool paste_from_clipboard(char* out_buf, size_t max_len) { (void)out_buf; (void)max_len; return false; }

#endif
