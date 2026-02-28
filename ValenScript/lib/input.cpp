#include <algorithm>
#include <string>
#include <windows.h>

namespace {
void sendKeyEvent(WORD key, DWORD flags) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    input.ki.dwFlags = flags;
    SendInput(1, &input, sizeof(INPUT));
}

void sendMouseEvent(DWORD flags, LONG dx = 0, LONG dy = 0, DWORD data = 0) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.mouseData = data;
    SendInput(1, &input, sizeof(INPUT));
}
} // namespace

namespace input_lib {

void key_down(WORD key) { sendKeyEvent(key, 0); }
void key_up(WORD key) { sendKeyEvent(key, KEYEVENTF_KEYUP); }
void key_press(WORD key) {
    key_down(key);
    key_up(key);
}

void key_code_down(int key) { key_down(static_cast<WORD>(key)); }
void key_code_up(int key) { key_up(static_cast<WORD>(key)); }
void key_code_press(int key) { key_press(static_cast<WORD>(key)); }

void type(const std::string& text) {
    for (char c : text) {
        SHORT keyInfo = VkKeyScanA(c);
        if (keyInfo == -1) {
            continue;
        }
        BYTE vk = LOBYTE(keyInfo);
        BYTE modifiers = HIBYTE(keyInfo);
        if (modifiers & 1) key_down(VK_SHIFT);
        if (modifiers & 2) key_down(VK_CONTROL);
        if (modifiers & 4) key_down(VK_MENU);
        key_press(static_cast<WORD>(vk));
        if (modifiers & 4) key_up(VK_MENU);
        if (modifiers & 2) key_up(VK_CONTROL);
        if (modifiers & 1) key_up(VK_SHIFT);
    }
}

void left_down() { sendMouseEvent(MOUSEEVENTF_LEFTDOWN); }
void left_up() { sendMouseEvent(MOUSEEVENTF_LEFTUP); }
void left_click() {
    left_down();
    left_up();
}

void right_down() { sendMouseEvent(MOUSEEVENTF_RIGHTDOWN); }
void right_up() { sendMouseEvent(MOUSEEVENTF_RIGHTUP); }
void right_click() {
    right_down();
    right_up();
}

void middle_down() { sendMouseEvent(MOUSEEVENTF_MIDDLEDOWN); }
void middle_up() { sendMouseEvent(MOUSEEVENTF_MIDDLEUP); }
void middle_click() {
    middle_down();
    middle_up();
}

void move(int x, int y) {
    int screenX = std::max(GetSystemMetrics(SM_CXSCREEN) - 1, 1);
    int screenY = std::max(GetSystemMetrics(SM_CYSCREEN) - 1, 1);
    LONG absX = static_cast<LONG>((x * 65535LL) / screenX);
    LONG absY = static_cast<LONG>((y * 65535LL) / screenY);
    sendMouseEvent(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, absX, absY);
}

void move_rel(int dx, int dy) { sendMouseEvent(MOUSEEVENTF_MOVE, dx, dy); }
void scroll(int amount) { sendMouseEvent(MOUSEEVENTF_WHEEL, 0, 0, static_cast<DWORD>(amount)); }
void hscroll(int amount) { sendMouseEvent(MOUSEEVENTF_HWHEEL, 0, 0, static_cast<DWORD>(amount)); }

int cursor_x() {
    POINT p = {};
    GetCursorPos(&p);
    return p.x;
}

int cursor_y() {
    POINT p = {};
    GetCursorPos(&p);
    return p.y;
}

#define DEFINE_LETTER_FUNCS(letter, vk) \
void letter##_down() { key_down(vk); } \
void letter##_up() { key_up(vk); } \
void letter##_press() { key_press(vk); }

DEFINE_LETTER_FUNCS(a, 'A')
DEFINE_LETTER_FUNCS(b, 'B')
DEFINE_LETTER_FUNCS(c, 'C')
DEFINE_LETTER_FUNCS(d, 'D')
DEFINE_LETTER_FUNCS(e, 'E')
DEFINE_LETTER_FUNCS(f, 'F')
DEFINE_LETTER_FUNCS(g, 'G')
DEFINE_LETTER_FUNCS(h, 'H')
DEFINE_LETTER_FUNCS(i, 'I')
DEFINE_LETTER_FUNCS(j, 'J')
DEFINE_LETTER_FUNCS(k, 'K')
DEFINE_LETTER_FUNCS(l, 'L')
DEFINE_LETTER_FUNCS(m, 'M')
DEFINE_LETTER_FUNCS(n, 'N')
DEFINE_LETTER_FUNCS(o, 'O')
DEFINE_LETTER_FUNCS(p, 'P')
DEFINE_LETTER_FUNCS(q, 'Q')
DEFINE_LETTER_FUNCS(r, 'R')
DEFINE_LETTER_FUNCS(s, 'S')
DEFINE_LETTER_FUNCS(t, 'T')
DEFINE_LETTER_FUNCS(u, 'U')
DEFINE_LETTER_FUNCS(v, 'V')
DEFINE_LETTER_FUNCS(w, 'W')
DEFINE_LETTER_FUNCS(x, 'X')
DEFINE_LETTER_FUNCS(y, 'Y')
DEFINE_LETTER_FUNCS(z, 'Z')

#undef DEFINE_LETTER_FUNCS

#define DEFINE_VK_FUNCS(name, vk) \
void name##_down() { key_down(vk); } \
void name##_up() { key_up(vk); } \
void name##_press() { key_press(vk); }

DEFINE_VK_FUNCS(enter, VK_RETURN)
DEFINE_VK_FUNCS(space, VK_SPACE)
DEFINE_VK_FUNCS(tab, VK_TAB)
DEFINE_VK_FUNCS(esc, VK_ESCAPE)
DEFINE_VK_FUNCS(shift, VK_SHIFT)
DEFINE_VK_FUNCS(ctrl, VK_CONTROL)
DEFINE_VK_FUNCS(alt, VK_MENU)
DEFINE_VK_FUNCS(backspace, VK_BACK)
DEFINE_VK_FUNCS(del, VK_DELETE)
DEFINE_VK_FUNCS(arrow_up, VK_UP)
DEFINE_VK_FUNCS(arrow_down, VK_DOWN)
DEFINE_VK_FUNCS(arrow_left, VK_LEFT)
DEFINE_VK_FUNCS(arrow_right, VK_RIGHT)
DEFINE_VK_FUNCS(home, VK_HOME)
DEFINE_VK_FUNCS(end, VK_END)
DEFINE_VK_FUNCS(page_up, VK_PRIOR)
DEFINE_VK_FUNCS(page_down, VK_NEXT)

#undef DEFINE_VK_FUNCS

} // namespace input_lib

