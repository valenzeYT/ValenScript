#include "../include/interpreter.h"
#include "../include/module_registry.h"
#include <array>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <richedit.h>

namespace gui_lib {
std::string open_file();
std::string save_file();
void clear_window();
int add_text(const std::string& text, int x, int y, int w, int h);
int add_section(const std::string& title, int x, int y, int w, int h);
int add_button(const std::string& label, int x, int y, int w, int h);
bool button_clicked(int id);
int add_input(const std::string& placeholder, int x, int y, int w, int h);
int add_editor(const std::string& text, int x, int y, int w, int h);
int add_panel(const std::string& text, int x, int y, int w, int h);
std::string input_text(int id);
void set_input(int id, const std::string& text);
int add_link(const std::string& label, const std::string& url, int x, int y, int w, int h);
void set_icon(const std::string& path);
void open_link(const std::string& url);
void set_opacity(double alpha);
void set_background_gradient(double r1, double g1, double b1, double r2, double g2, double b2);
void set_panel_color(double r, double g, double b);
void set_input_color(double r, double g, double b);
void set_editor_color(double r, double g, double b);
void set_button_color(double r, double g, double b);
void set_text_color(double r, double g, double b);
void set_muted_color(double r, double g, double b);
void set_section_color(double r, double g, double b);
void set_fonts(const std::string& uiName, double uiSize, double uiWeight,
               const std::string& codeName, double codeSize, double codeWeight);
}

namespace {
std::string runGuiPowerShell(const std::string& script) {
    std::string command = "powershell -NoProfile -Command \"" + script + "\" 2>&1";
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("gui: failed to start PowerShell");
    }

    std::string output;
    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    int exitCode = _pclose(pipe);
    if (exitCode != 0) {
        throw std::runtime_error(output.empty() ? "gui: PowerShell failed" : output);
    }
    return output;
}

constexpr const char* kValenWindowClass = "ValenScriptNativeWindowClass";
constexpr const char* kValenMediaMenuClass = "ValenScriptMediaMenuClass";
std::mutex g_windowMutex;
HWND g_windowHwnd = nullptr;
std::atomic<bool> g_windowRunning{false};
std::unordered_map<int, HWND> g_controlHwndById;
std::unordered_map<int, std::string> g_controlTypeById;
std::unordered_map<HWND, std::string> g_controlTypeByHwnd;
std::unordered_map<int, std::string> g_linkUrlById;
std::unordered_map<int, bool> g_controlClicked;
std::unordered_map<HWND, HWND> g_editorToGutter;
std::unordered_map<HWND, bool> g_gutterIsRich;
std::unordered_map<HWND, WNDPROC> g_editorPrevProc;
std::unordered_map<HWND, WNDPROC> g_panelPrevProc;
std::unordered_map<int, HWND> g_editorGutterById;
std::unordered_map<HWND, bool> g_editorHighlighting;
std::unordered_map<HWND, bool> g_editorIsRich;
std::unordered_set<HWND> g_pendingEditorRefresh;
int g_nextControlId = 5000;
HICON g_windowIcon = nullptr;
HFONT g_uiFont = nullptr;
HFONT g_codeFont = nullptr;
HBRUSH g_brushPanel = nullptr;
HBRUSH g_brushInput = nullptr;
HBRUSH g_brushEditor = nullptr;
HBRUSH g_brushGutter = nullptr;
HBRUSH g_brushButton = nullptr;
COLORREF g_bgTop = RGB(12, 18, 34);
COLORREF g_bgBottom = RGB(6, 10, 22);
COLORREF g_colorPanel = RGB(10, 16, 30);
COLORREF g_colorInput = RGB(18, 26, 44);
COLORREF g_colorEditor = RGB(12, 18, 34);
COLORREF g_colorGutter = RGB(22, 32, 54);
COLORREF g_colorButton = RGB(26, 40, 68);
COLORREF g_colorText = RGB(214, 233, 255);
COLORREF g_colorMuted = RGB(150, 176, 210);
COLORREF g_colorSection = RGB(205, 224, 255);
int g_windowAlpha = 230;
std::string g_uiFontName = "Segoe UI Semibold";
std::string g_codeFontName = "Cascadia Mono";
int g_uiFontSize = -20;
int g_codeFontSize = -19;
int g_uiFontWeight = FW_SEMIBOLD;
int g_codeFontWeight = FW_NORMAL;
constexpr UINT WM_VALEN_CTRL = WM_APP + 100;
constexpr UINT WM_VALEN_CLEAR = WM_APP + 101;
constexpr UINT_PTR kGradientTimerId = 77;
constexpr UINT_PTR kEditorRefreshTimerId = 78;
constexpr int kEditorGutterWidth = 56;
constexpr COLORREF kCodeDefault = RGB(224, 228, 237);
constexpr COLORREF kCodeKeyword = RGB(125, 191, 255);
constexpr COLORREF kCodeString = RGB(241, 203, 133);
constexpr COLORREF kCodeNumber = RGB(157, 219, 171);
constexpr COLORREF kCodeComment = RGB(131, 149, 172);
constexpr COLORREF kCodeBuiltin = RGB(196, 166, 255);

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (size <= 0) return L"";
    std::wstring out(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &out[0], size);
    return out;
}

std::string wideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &out[0], size, nullptr, nullptr);
    return out;
}

std::string getControlText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return "";
    std::wstring wout(static_cast<size_t>(len) + 1, L'\0');
    GetWindowTextW(hwnd, &wout[0], len + 1);
    wout.resize(static_cast<size_t>(len));
    return wideToUtf8(wout);
}

void setSelectionColor(HWND hwnd, long start, long end, COLORREF color) {
    CHARRANGE range{};
    range.cpMin = start;
    range.cpMax = end;
    SendMessageA(hwnd, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));

    CHARFORMAT2A format{};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = color;
    SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
}

bool isEditorGutterHwnd(HWND hwnd) {
    for (const auto& kv : g_editorToGutter) {
        if (kv.second == hwnd) return true;
    }
    return false;
}

void disableControlTheme(HWND hwnd) {
    using SetWindowThemeFn = HRESULT(WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    static SetWindowThemeFn setWindowTheme = []() -> SetWindowThemeFn {
        HMODULE h = LoadLibraryA("uxtheme.dll");
        if (!h) return nullptr;
        return reinterpret_cast<SetWindowThemeFn>(GetProcAddress(h, "SetWindowTheme"));
    }();
    if (setWindowTheme) {
        setWindowTheme(hwnd, L"", L"");
    }
}

void paintSolidBackground(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    if (rc.bottom <= rc.top || rc.right <= rc.left) return;
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    if (g_brushPanel) {
        const int steps = 28;
        const int height = rc.bottom - rc.top;
        for (int i = 0; i < steps; ++i) {
            int y0 = rc.top + (height * i) / steps;
            int y1 = rc.top + (height * (i + 1)) / steps;
            int r = static_cast<int>(GetRValue(g_bgTop)) + (i * (static_cast<int>(GetRValue(g_bgBottom)) - static_cast<int>(GetRValue(g_bgTop)))) / steps;
            int g = static_cast<int>(GetGValue(g_bgTop)) + (i * (static_cast<int>(GetGValue(g_bgBottom)) - static_cast<int>(GetGValue(g_bgTop)))) / steps;
            int b = static_cast<int>(GetBValue(g_bgTop)) + (i * (static_cast<int>(GetBValue(g_bgBottom)) - static_cast<int>(GetBValue(g_bgTop)))) / steps;
            HBRUSH band = CreateSolidBrush(RGB(r, g, b));
            RECT bandRc{rc.left, y0, rc.right, y1};
            FillRect(hdc, &bandRc, band);
            DeleteObject(band);
        }
    } else {
        FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
    }
    EndPaint(hwnd, &ps);
}

void syncEditorGutter(HWND editor) {
    auto it = g_editorToGutter.find(editor);
    if (it == g_editorToGutter.end()) return;
    HWND gutter = it->second;
    if (!gutter) return;
    auto richIt = g_editorIsRich.find(editor);
    bool isRich = (richIt != g_editorIsRich.end() && richIt->second);
    auto gutterRichIt = g_gutterIsRich.find(gutter);
    bool gutterIsRich = (gutterRichIt != g_gutterIsRich.end() && gutterRichIt->second);
    if (isRich && gutterIsRich) {
        POINT pt{};
        SendMessageA(editor, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&pt));
        SendMessageA(gutter, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&pt));
        return;
    }
    int firstEditor = static_cast<int>(SendMessageA(editor, EM_GETFIRSTVISIBLELINE, 0, 0));
    int firstGutter = static_cast<int>(SendMessageA(gutter, EM_GETFIRSTVISIBLELINE, 0, 0));
    int delta = firstEditor - firstGutter;
    if (delta != 0) {
        SendMessageA(gutter, EM_LINESCROLL, 0, delta);
    }
}

bool isWordChar(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || c == '_';
}

const std::vector<std::string>& autocompleteTerms() {
    static const std::vector<std::string> terms = {
        "IMPORT", "FUNC", "RETURN", "IF", "ELSE", "WHILE", "FOR", "REPEAT", "WAIT",
        "TRUE", "FALSE", "PRINT", "BREAK", "CONTINUE", "EVENT", "AND", "OR", "NOT",
        "LEN", "TYPE", "NUM", "BOOL", "STRINGIFIED", "NUMIFIED", "BOOLIFIED",
        "STDIN", "ASSERT", "CHANNEL_CREATE", "CHANNEL_SEND", "CHANNEL_RECV", "SET_TIMEOUT"
    };
    return terms;
}

bool startsWithIgnoreCase(const std::string& value, const std::string& prefix) {
    if (prefix.size() > value.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        char a = static_cast<char>(std::toupper(static_cast<unsigned char>(value[i])));
        char b = static_cast<char>(std::toupper(static_cast<unsigned char>(prefix[i])));
        if (a != b) return false;
    }
    return true;
}

bool tryAutocompleteWord(HWND editor) {
    std::string text = getControlText(editor);
    auto richIt = g_editorIsRich.find(editor);
    bool isRich = (richIt != g_editorIsRich.end() && richIt->second);

    long caretStart = 0;
    long caretEnd = 0;
    if (isRich) {
        CHARRANGE sel{};
        SendMessageA(editor, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&sel));
        caretStart = sel.cpMin;
        caretEnd = sel.cpMax;
    } else {
        DWORD s = 0;
        DWORD e = 0;
        SendMessageA(editor, EM_GETSEL, reinterpret_cast<WPARAM>(&s), reinterpret_cast<LPARAM>(&e));
        caretStart = static_cast<long>(s);
        caretEnd = static_cast<long>(e);
    }
    if (caretStart != caretEnd) return false;
    if (caretStart <= 0 || static_cast<size_t>(caretStart) > text.size()) return false;

    long wordStart = caretStart;
    while (wordStart > 0 && isWordChar(text[static_cast<size_t>(wordStart - 1)])) {
        --wordStart;
    }
    if (wordStart == caretStart) return false;
    std::string prefix = text.substr(static_cast<size_t>(wordStart), static_cast<size_t>(caretStart - wordStart));

    std::string best = "";
    for (const auto& term : autocompleteTerms()) {
        if (!startsWithIgnoreCase(term, prefix)) continue;
        if (term.size() <= prefix.size()) continue;
        if (best.empty() || term.size() < best.size()) best = term;
    }
    if (best.empty()) return false;

    if (isRich) {
        CHARRANGE replaceSel{};
        replaceSel.cpMin = wordStart;
        replaceSel.cpMax = caretStart;
        SendMessageA(editor, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&replaceSel));
        SendMessageA(editor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(best.c_str()));
    } else {
        SendMessageA(editor, EM_SETSEL, wordStart, caretStart);
        SendMessageA(editor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(best.c_str()));
    }
    return true;
}

void updateEditorLineNumbers(HWND editor) {
    auto it = g_editorToGutter.find(editor);
    if (it == g_editorToGutter.end()) return;
    HWND gutter = it->second;
    if (!gutter) return;
    std::string text = getControlText(editor);
    int lineCount = 1;
    for (char ch : text) {
        if (ch == '\n') ++lineCount;
    }
    int digitCount = static_cast<int>(std::to_string(lineCount).size());
    if (digitCount < 2) digitCount = 2;
    std::string lines;
    lines.reserve(static_cast<size_t>(lineCount) * static_cast<size_t>(digitCount + 3));
    for (int i = 1; i <= lineCount; ++i) {
        std::string n = std::to_string(i);
        int pad = digitCount - static_cast<int>(n.size());
        for (int p = 0; p < pad; ++p) lines += " ";
        lines += n;
        lines += " ";
        if (i != lineCount) lines += "\r\n";
    }
    SetWindowTextA(gutter, lines.c_str());
    syncEditorGutter(editor);
}

void applyEditorHighlight(HWND editor) {
    auto richIt = g_editorIsRich.find(editor);
    if (richIt == g_editorIsRich.end() || !richIt->second) return;
    if (g_editorHighlighting[editor]) return;
    g_editorHighlighting[editor] = true;

    std::string text = getControlText(editor);
    CHARRANGE oldSel{};
    SendMessageA(editor, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&oldSel));

    SendMessageA(editor, WM_SETREDRAW, FALSE, 0);
    setSelectionColor(editor, 0, -1, kCodeDefault);

    static const std::unordered_set<std::string> keywords = {
        "IMPORT", "FUNC", "RETURN", "IF", "ELSE", "WHILE", "FOR", "REPEAT", "WAIT",
        "TRUE", "FALSE", "PRINT", "BREAK", "CONTINUE", "EVENT", "AND", "OR", "NOT"
    };
    static const std::unordered_set<std::string> builtins = {
        "LEN", "TYPE", "NUM", "BOOL", "STRINGIFIED", "NUMIFIED", "BOOLIFIED",
        "STDIN", "ASSERT", "CHANNEL_CREATE", "CHANNEL_SEND", "CHANNEL_RECV", "SET_TIMEOUT"
    };

    const size_t n = text.size();
    size_t i = 0;
    while (i < n) {
        if ((text[i] == '/' && i + 1 < n && text[i + 1] == '/') || text[i] == '#') {
            size_t start = i;
            if (text[i] == '/' && i + 1 < n && text[i + 1] == '/') i += 2;
            else i += 1;
            while (i < n && text[i] != '\n') ++i;
            setSelectionColor(editor, static_cast<long>(start), static_cast<long>(i), kCodeComment);
            continue;
        }
        if (text[i] == '"') {
            size_t start = i++;
            while (i < n) {
                if (text[i] == '\\' && i + 1 < n) {
                    i += 2;
                    continue;
                }
                if (text[i] == '"') {
                    ++i;
                    break;
                }
                ++i;
            }
            setSelectionColor(editor, static_cast<long>(start), static_cast<long>(i), kCodeString);
            continue;
        }
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (std::isdigit(c)) {
            size_t start = i++;
            while (i < n) {
                unsigned char d = static_cast<unsigned char>(text[i]);
                if (!std::isdigit(d) && text[i] != '.') break;
                ++i;
            }
            setSelectionColor(editor, static_cast<long>(start), static_cast<long>(i), kCodeNumber);
            continue;
        }
        if (std::isalpha(c) || text[i] == '_') {
            size_t start = i++;
            while (i < n) {
                unsigned char d = static_cast<unsigned char>(text[i]);
                if (!std::isalnum(d) && text[i] != '_') break;
                ++i;
            }
            std::string token = text.substr(start, i - start);
            for (char& ch : token) {
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            if (keywords.find(token) != keywords.end()) {
                setSelectionColor(editor, static_cast<long>(start), static_cast<long>(i), kCodeKeyword);
            } else if (builtins.find(token) != builtins.end()) {
                setSelectionColor(editor, static_cast<long>(start), static_cast<long>(i), kCodeBuiltin);
            }
            continue;
        }
        ++i;
    }

    SendMessageA(editor, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&oldSel));
    SendMessageA(editor, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(editor, nullptr, FALSE);
    g_editorHighlighting[editor] = false;
}

LRESULT CALLBACK valenEditorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto prevIt = g_editorPrevProc.find(hwnd);
    if (prevIt == g_editorPrevProc.end()) {
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    WNDPROC prev = prevIt->second;

    if (msg == WM_KEYDOWN && wParam == VK_TAB) {
        if (tryAutocompleteWord(hwnd)) {
            syncEditorGutter(hwnd);
            return 0;
        }
    }

    if (msg == WM_VSCROLL || msg == WM_MOUSEWHEEL || msg == WM_KEYUP || msg == WM_LBUTTONUP) {
        LRESULT out = CallWindowProcA(prev, hwnd, msg, wParam, lParam);
        syncEditorGutter(hwnd);
        return out;
    }

    if (msg == WM_DESTROY) {
        g_editorToGutter.erase(hwnd);
        g_editorHighlighting.erase(hwnd);
        g_editorPrevProc.erase(hwnd);
        g_panelPrevProc.erase(hwnd);
    }

    return CallWindowProcA(prev, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK valenPanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto prevIt = g_panelPrevProc.find(hwnd);
    if (prevIt == g_panelPrevProc.end()) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    WNDPROC prev = prevIt->second;

    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MOUSEMOVE:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_KEYDOWN:
        case WM_CHAR:
        case WM_SETFOCUS:
            return 0;
        default:
            break;
    }

    return CallWindowProcW(prev, hwnd, msg, wParam, lParam);
}

enum class ControlOp : int {
    AddText = 1,
    AddSection = 2,
    AddButton = 3,
    AddInput = 4,
    AddLink = 5,
    AddEditor = 6,
    AddPanel = 7
};

struct ControlRequest {
    ControlOp op;
    int id;
    int x;
    int y;
    int w;
    int h;
    std::wstring text;
    std::string url;
    HWND out = nullptr;
};

constexpr int kBtnImport = 1001;
constexpr int kBtnExport = 1002;
constexpr int kBtnMeta = 1003;
HWND g_metaLabel = nullptr;
std::string g_mediaPath;

std::string buildMediaMetaText() {
    if (g_mediaPath.empty()) {
        return "No media imported yet.";
    }
    std::error_code ec;
    auto bytes = std::filesystem::file_size(g_mediaPath, ec);
    if (ec) {
        return "Imported: " + g_mediaPath + "\r\nMetadata unavailable.";
    }
    double mb = static_cast<double>(bytes) / 1048576.0;
    std::ostringstream out;
    out << "Imported: " << g_mediaPath << "\r\n";
    out << "File size: " << std::fixed << std::setprecision(2) << mb << " MB (" << bytes << " bytes)\r\n";
    out << "DPI: 72 (assumed)\r\n";
    out << "Pixel size: unavailable (image backend stub)";
    return out.str();
}

void refreshMediaLabel() {
    if (g_metaLabel) {
        std::string text = buildMediaMetaText();
        SetWindowTextA(g_metaLabel, text.c_str());
    }
}

LRESULT CALLBACK valenMediaMenuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
        case WM_CREATE: {
            CreateWindowExA(0, "STATIC", "Valen Mini Media Menu",
                WS_CHILD | WS_VISIBLE, 20, 15, 420, 24, hwnd, nullptr, GetModuleHandleA(nullptr), nullptr);
            CreateWindowExA(0, "BUTTON", "Import Media",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 50, 130, 34, hwnd, reinterpret_cast<HMENU>(kBtnImport), GetModuleHandleA(nullptr), nullptr);
            CreateWindowExA(0, "BUTTON", "Export Media",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 165, 50, 130, 34, hwnd, reinterpret_cast<HMENU>(kBtnExport), GetModuleHandleA(nullptr), nullptr);
            CreateWindowExA(0, "BUTTON", "Metadata",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 310, 50, 130, 34, hwnd, reinterpret_cast<HMENU>(kBtnMeta), GetModuleHandleA(nullptr), nullptr);
            g_metaLabel = CreateWindowExA(0, "STATIC", "No media imported yet.",
                WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 100, 560, 170, hwnd, nullptr, GetModuleHandleA(nullptr), nullptr);
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == kBtnImport) {
                try {
                    std::string picked = gui_lib::open_file();
                    if (!picked.empty()) {
                        g_mediaPath = picked;
                        refreshMediaLabel();
                    }
                } catch (const std::exception& e) {
                    MessageBoxA(hwnd, e.what(), "Import error", MB_OK | MB_ICONERROR);
                }
                return 0;
            }
            if (id == kBtnExport) {
                if (g_mediaPath.empty()) {
                    MessageBoxA(hwnd, "Import media first.", "Export", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                try {
                    std::string outPath = gui_lib::save_file();
                    if (!outPath.empty()) {
                        std::error_code ec;
                        std::filesystem::copy_file(g_mediaPath, outPath, std::filesystem::copy_options::overwrite_existing, ec);
                        if (ec) {
                            MessageBoxA(hwnd, ("Export failed: " + ec.message()).c_str(), "Export", MB_OK | MB_ICONERROR);
                        } else {
                            MessageBoxA(hwnd, "Export complete.", "Export", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                } catch (const std::exception& e) {
                    MessageBoxA(hwnd, e.what(), "Export error", MB_OK | MB_ICONERROR);
                }
                return 0;
            }
            if (id == kBtnMeta) {
                std::string meta = buildMediaMetaText();
                MessageBoxA(hwnd, meta.c_str(), "Metadata", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            return 0;
        }
        case WM_DESTROY:
            g_metaLabel = nullptr;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK valenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_VALEN_CLEAR) {
        for (auto& kv : g_editorPrevProc) {
            SetWindowLongPtrA(kv.first, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(kv.second));
        }
        for (auto& kv : g_controlHwndById) {
            if (kv.second) DestroyWindow(kv.second);
        }
        for (auto& kv : g_editorGutterById) {
            if (kv.second) DestroyWindow(kv.second);
        }
        g_controlHwndById.clear();
        g_controlTypeById.clear();
        g_controlTypeByHwnd.clear();
        g_linkUrlById.clear();
        g_controlClicked.clear();
        g_editorToGutter.clear();
        g_gutterIsRich.clear();
        g_editorPrevProc.clear();
        g_panelPrevProc.clear();
        g_editorGutterById.clear();
        g_editorHighlighting.clear();
        g_editorIsRich.clear();
        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);
        return 0;
    }
    if (msg == WM_TIMER && wParam == kEditorRefreshTimerId) {
        KillTimer(hwnd, kEditorRefreshTimerId);
        for (HWND editor : g_pendingEditorRefresh) {
            updateEditorLineNumbers(editor);
            applyEditorHighlight(editor);
        }
        g_pendingEditorRefresh.clear();
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    if (msg == WM_PAINT) {
        paintSolidBackground(hwnd);
        return 0;
    }
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND ctrl = reinterpret_cast<HWND>(lParam);
        std::string type = "";
        auto itType = g_controlTypeByHwnd.find(ctrl);
        if (itType != g_controlTypeByHwnd.end()) {
            type = itType->second;
        } else if (isEditorGutterHwnd(ctrl)) {
            type = "gutter";
        }

        HBRUSH transparent = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
        if (type == "section") {
            SetTextColor(hdc, g_colorSection);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        if (type == "text") {
            SetTextColor(hdc, g_colorText);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        if (type == "button" || type == "link") {
            SetTextColor(hdc, g_colorText);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        if (type == "input") {
            SetTextColor(hdc, g_colorText);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        if (type == "gutter") {
            SetTextColor(hdc, g_colorMuted);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        if (type == "editor") {
            SetTextColor(hdc, g_colorText);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        if (type == "panel") {
            SetTextColor(hdc, g_colorText);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(transparent);
        }
        SetTextColor(hdc, g_colorText);
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(transparent);
    }
    if (msg == WM_VALEN_CTRL) {
        auto* req = reinterpret_cast<ControlRequest*>(lParam);
        if (!req) return 0;

        HWND child = nullptr;
        if (req->op == ControlOp::AddText) {
            child = CreateWindowExW(WS_EX_TRANSPARENT, L"STATIC", req->text.c_str(),
                WS_CHILD | WS_VISIBLE | SS_LEFT, req->x, req->y, req->w, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            if (child) {
                disableControlTheme(child);
                g_controlTypeById[req->id] = "text";
                g_controlTypeByHwnd[child] = "text";
                if (g_uiFont) SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            }
        } else if (req->op == ControlOp::AddSection) {
            child = CreateWindowExW(WS_EX_TRANSPARENT, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE, req->x, req->y, req->w, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            if (child) {
                disableControlTheme(child);
                g_controlTypeById[req->id] = "section";
                g_controlTypeByHwnd[child] = "section";
                if (g_uiFont) SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
                if (!req->text.empty()) {
                    HWND label = CreateWindowExW(WS_EX_TRANSPARENT, L"STATIC", req->text.c_str(),
                        WS_CHILD | WS_VISIBLE | SS_LEFT,
                        req->x + 12, req->y + 2, req->w - 24, 20,
                        hwnd, nullptr, GetModuleHandleA(nullptr), nullptr);
                    if (label) {
                        disableControlTheme(label);
                        g_controlTypeByHwnd[label] = "text";
                        if (g_uiFont) SendMessageA(label, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
                    }
                }
            }
        } else if (req->op == ControlOp::AddButton) {
            child = CreateWindowExW(WS_EX_TRANSPARENT, L"BUTTON", req->text.c_str(),
                WS_CHILD | WS_VISIBLE | BS_FLAT, req->x, req->y, req->w, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            if (child) {
                disableControlTheme(child);
                g_controlTypeById[req->id] = "button";
                g_controlTypeByHwnd[child] = "button";
                g_controlClicked[req->id] = false;
                if (g_uiFont) SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            }
        } else if (req->op == ControlOp::AddInput) {
            child = CreateWindowExW(WS_EX_TRANSPARENT, L"EDIT", req->text.c_str(),
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_READONLY, req->x, req->y, req->w, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            if (child) {
                disableControlTheme(child);
                g_controlTypeById[req->id] = "input";
                g_controlTypeByHwnd[child] = "input";
                if (g_uiFont) SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            }
        } else if (req->op == ControlOp::AddPanel) {
            LoadLibraryA("Msftedit.dll");
            child = CreateWindowExW(0, L"RICHEDIT50W", req->text.c_str(),
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                req->x, req->y, req->w, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            if (!child) {
                child = CreateWindowExW(0, L"EDIT", req->text.c_str(),
                    WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                    req->x, req->y, req->w, req->h,
                    hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            }
            if (child) {
                disableControlTheme(child);
                g_controlTypeById[req->id] = "panel";
                g_controlTypeByHwnd[child] = "panel";
                if (g_codeFont) SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_codeFont), TRUE);
                SendMessageA(child, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(8, 8));
                SendMessageA(child, EM_SETREADONLY, TRUE, 0);
                auto prev = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(child, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(valenPanelProc)));
                g_panelPrevProc[child] = prev;
            }
        } else if (req->op == ControlOp::AddEditor) {
            LoadLibraryA("Msftedit.dll");
            HWND gutter = CreateWindowExW(0, L"RICHEDIT50W", L"1",
                WS_CHILD | WS_VISIBLE |
                WS_BORDER | ES_RIGHT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                req->x, req->y, kEditorGutterWidth, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id + 100000), GetModuleHandleA(nullptr), nullptr);
            bool gutterRich = gutter != nullptr;
            if (!gutter) {
                gutter = CreateWindowExW(0, L"RICHEDIT50A", L"1",
                    WS_CHILD | WS_VISIBLE |
                    WS_BORDER | ES_RIGHT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                    req->x, req->y, kEditorGutterWidth, req->h,
                    hwnd, reinterpret_cast<HMENU>(req->id + 100000), GetModuleHandleA(nullptr), nullptr);
                gutterRich = gutter != nullptr;
            }
            if (!gutter) {
                gutter = CreateWindowExW(0, L"EDIT", L"1",
                    WS_CHILD | WS_VISIBLE |
                    WS_BORDER | ES_RIGHT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                    req->x, req->y, kEditorGutterWidth, req->h,
                    hwnd, reinterpret_cast<HMENU>(req->id + 100000), GetModuleHandleA(nullptr), nullptr);
                gutterRich = false;
            }
            child = CreateWindowExW(WS_EX_TRANSPARENT, L"RICHEDIT50W", req->text.c_str(),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
                req->x + kEditorGutterWidth + 4, req->y, req->w - (kEditorGutterWidth + 4), req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            bool rich = child != nullptr;
            if (!child) {
                child = CreateWindowExW(WS_EX_TRANSPARENT, L"RICHEDIT50A", req->text.c_str(),
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                    ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
                    req->x + kEditorGutterWidth + 4, req->y, req->w - (kEditorGutterWidth + 4), req->h,
                    hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
                rich = child != nullptr;
            }
            if (!child) {
                child = CreateWindowExW(WS_EX_TRANSPARENT, L"EDIT", req->text.c_str(),
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                    ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
                    req->x + kEditorGutterWidth + 4, req->y, req->w - (kEditorGutterWidth + 4), req->h,
                    hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
                rich = false;
            }
            if (child && gutter) {
                disableControlTheme(child);
                disableControlTheme(gutter);
                g_controlTypeById[req->id] = "editor";
                g_controlTypeByHwnd[child] = "editor";
                g_controlTypeByHwnd[gutter] = "gutter";
                g_editorToGutter[child] = gutter;
                g_gutterIsRich[gutter] = gutterRich;
                g_editorGutterById[req->id] = gutter;
                g_editorIsRich[child] = rich;
                if (g_codeFont) {
                    SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_codeFont), TRUE);
                    SendMessageA(gutter, WM_SETFONT, reinterpret_cast<WPARAM>(g_codeFont), TRUE);
                }
                SendMessageA(child, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(6, 6));
                SendMessageA(gutter, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(6, 6));
                if (rich) SendMessageA(child, EM_SETBKGNDCOLOR, 0, RGB(26, 33, 47));
                SendMessageA(gutter, EM_SETBKGNDCOLOR, 0, RGB(39, 47, 63));
                setSelectionColor(gutter, 0, -1, RGB(142, 161, 189));
                SendMessageA(gutter, EM_SETREADONLY, TRUE, 0);
                auto prev = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(child, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(valenEditorProc)));
                g_editorPrevProc[child] = prev;
                updateEditorLineNumbers(child);
                applyEditorHighlight(child);
            } else {
                if (gutter) DestroyWindow(gutter);
                if (child) DestroyWindow(child);
                child = nullptr;
            }
        } else if (req->op == ControlOp::AddLink) {
            child = CreateWindowExW(0, L"BUTTON", req->text.c_str(),
                WS_CHILD | WS_VISIBLE | BS_FLAT, req->x, req->y, req->w, req->h,
                hwnd, reinterpret_cast<HMENU>(req->id), GetModuleHandleA(nullptr), nullptr);
            if (child) {
                disableControlTheme(child);
                g_controlTypeById[req->id] = "link";
                g_controlTypeByHwnd[child] = "link";
                g_controlClicked[req->id] = false;
                if (!req->url.empty()) g_linkUrlById[req->id] = req->url;
                if (g_uiFont) SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            }
        }

        if (child) {
            g_controlHwndById[req->id] = child;
            InvalidateRect(hwnd, nullptr, TRUE);
            UpdateWindow(hwnd);
        }
        req->out = child;
        return 0;
    }
    if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        auto itType = g_controlTypeById.find(id);
        if (itType != g_controlTypeById.end() && itType->second == "editor" && code == EN_CHANGE) {
            auto editorIt = g_controlHwndById.find(id);
            if (editorIt != g_controlHwndById.end() && editorIt->second) {
                g_pendingEditorRefresh.insert(editorIt->second);
                SetTimer(hwnd, kEditorRefreshTimerId, 70, nullptr);
            }
        }
        if (itType != g_controlTypeById.end() && code == BN_CLICKED) {
            g_controlClicked[id] = true;
            if (itType->second == "link") {
                auto itUrl = g_linkUrlById.find(id);
                if (itUrl != g_linkUrlById.end()) {
                    ShellExecuteA(nullptr, "open", itUrl->second.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
            }
        }
    }
    if (msg == WM_DESTROY) {
        {
            std::lock_guard<std::mutex> lock(g_windowMutex);
            if (g_windowHwnd == hwnd) g_windowHwnd = nullptr;
        }
        g_controlHwndById.clear();
        g_controlTypeById.clear();
        g_controlTypeByHwnd.clear();
        g_linkUrlById.clear();
        g_controlClicked.clear();
        g_editorToGutter.clear();
        g_gutterIsRich.clear();
        g_editorPrevProc.clear();
        g_editorGutterById.clear();
        g_editorHighlighting.clear();
        g_editorIsRich.clear();
        g_pendingEditorRefresh.clear();
        if (g_windowIcon) {
            DestroyIcon(g_windowIcon);
            g_windowIcon = nullptr;
        }
        if (g_uiFont) {
            DeleteObject(g_uiFont);
            g_uiFont = nullptr;
        }
        if (g_codeFont) {
            DeleteObject(g_codeFont);
            g_codeFont = nullptr;
        }
        if (g_brushPanel) {
            DeleteObject(g_brushPanel);
            g_brushPanel = nullptr;
        }
        if (g_brushInput) {
            DeleteObject(g_brushInput);
            g_brushInput = nullptr;
        }
        if (g_brushEditor) {
            DeleteObject(g_brushEditor);
            g_brushEditor = nullptr;
        }
        if (g_brushGutter) {
            DeleteObject(g_brushGutter);
            g_brushGutter = nullptr;
        }
        if (g_brushButton) {
            DeleteObject(g_brushButton);
            g_brushButton = nullptr;
        }
        KillTimer(hwnd, kGradientTimerId);
        KillTimer(hwnd, kEditorRefreshTimerId);
        g_windowRunning = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

HWND currentWindow() {
    std::lock_guard<std::mutex> lock(g_windowMutex);
    return g_windowHwnd;
}

HWND waitForWindow(int timeoutMs = 1500) {
    const int sleepMs = 20;
    int waited = 0;
    while (waited <= timeoutMs) {
        HWND hwnd = currentWindow();
        if (hwnd) return hwnd;
        Sleep(sleepMs);
        waited += sleepMs;
    }
    return nullptr;
}

int nextControlId() {
    std::lock_guard<std::mutex> lock(g_windowMutex);
    return g_nextControlId++;
}

HWND createControl(ControlOp op, const std::string& text, const std::string& url, int x, int y, int w, int h, int id) {
    HWND hwnd = waitForWindow();
    if (!hwnd) return nullptr;
    ControlRequest req{op, id, x, y, w, h, utf8ToWide(text), url, nullptr};
    SendMessageA(hwnd, WM_VALEN_CTRL, 0, reinterpret_cast<LPARAM>(&req));
    return req.out;
}

void windowThreadMain(std::string title, int width, int height) {
    auto enableDpi = []() {
        using SetDpiAwarenessContextFn = BOOL (WINAPI*)(DPI_AWARENESS_CONTEXT);
        HMODULE user32 = LoadLibraryA("user32.dll");
        if (user32) {
            auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
            if (fn) {
                fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
                return;
            }
        }
        auto setDpiAware = reinterpret_cast<BOOL (WINAPI*)()>(GetProcAddress(GetModuleHandleA("user32.dll"), "SetProcessDPIAware"));
        if (setDpiAware) {
            setDpiAware();
        }
    };
    enableDpi();
    HINSTANCE hInst = GetModuleHandleA(nullptr);
    std::wstring wClass = utf8ToWide(kValenWindowClass);
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = valenWindowProc;
    wc.hInstance = hInst;
    wc.lpszClassName = wClass.c_str();
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassExW(&wc);

    std::wstring wTitle = utf8ToWide(title);
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED,
        wClass.c_str(),
        wTitle.c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );
    if (!hwnd) {
        g_windowRunning = false;
        return;
    }
    SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(g_windowAlpha), LWA_ALPHA);

    if (!g_uiFont) {
        g_uiFont = CreateFontA(
            g_uiFontSize, 0, 0, 0, g_uiFontWeight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, g_uiFontName.c_str());
    }
    if (!g_codeFont) {
        g_codeFont = CreateFontA(
            g_codeFontSize, 0, 0, 0, g_codeFontWeight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, g_codeFontName.c_str());
    }
    if (!g_brushPanel) g_brushPanel = CreateSolidBrush(g_colorPanel);
    if (!g_brushInput) g_brushInput = CreateSolidBrush(g_colorInput);
    if (!g_brushEditor) g_brushEditor = CreateSolidBrush(g_colorEditor);
    if (!g_brushGutter) g_brushGutter = CreateSolidBrush(g_colorGutter);
    if (!g_brushButton) g_brushButton = CreateSolidBrush(g_colorButton);

    {
        std::lock_guard<std::mutex> lock(g_windowMutex);
        g_windowHwnd = hwnd;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    // No animated gradient by default.

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    {
        std::lock_guard<std::mutex> lock(g_windowMutex);
        if (g_windowHwnd == hwnd) g_windowHwnd = nullptr;
    }
    g_windowRunning = false;
}
} // namespace

namespace gui_lib {

void msgbox(const std::string& text) {
    MessageBoxA(nullptr, text.c_str(), "ValenScript", MB_OK | MB_ICONINFORMATION);
}

bool confirm(const std::string& text) {
    int result = MessageBoxA(nullptr, text.c_str(), "ValenScript", MB_YESNO | MB_ICONQUESTION);
    return result == IDYES;
}

std::string prompt(const std::string& text) {
    _putenv_s("VALEN_GUI_PROMPT", text.c_str());
    const std::string script =
        "Add-Type -AssemblyName Microsoft.VisualBasic; "
        "$r=[Microsoft.VisualBasic.Interaction]::InputBox($env:VALEN_GUI_PROMPT,'ValenScript',''); "
        "[Console]::Out.Write($r)";
    return runGuiPowerShell(script);
}

void beep() { MessageBeep(MB_OK); }

void info(const std::string& text) { MessageBoxA(nullptr, text.c_str(), "ValenScript", MB_OK | MB_ICONINFORMATION); }
void warning(const std::string& text) { MessageBoxA(nullptr, text.c_str(), "ValenScript", MB_OK | MB_ICONWARNING); }
void error(const std::string& text) { MessageBoxA(nullptr, text.c_str(), "ValenScript", MB_OK | MB_ICONERROR); }

int yesnocancel(const std::string& text) {
    int result = MessageBoxA(nullptr, text.c_str(), "ValenScript", MB_YESNOCANCEL | MB_ICONQUESTION);
    if (result == IDYES) return 1;
    if (result == IDNO) return 0;
    return -1;
}

std::string prompt_default(const std::string& text, const std::string& defaultValue) {
    _putenv_s("VALEN_GUI_PROMPT", text.c_str());
    _putenv_s("VALEN_GUI_DEFAULT", defaultValue.c_str());
    const std::string script =
        "Add-Type -AssemblyName Microsoft.VisualBasic; "
        "$r=[Microsoft.VisualBasic.Interaction]::InputBox($env:VALEN_GUI_PROMPT,'ValenScript',$env:VALEN_GUI_DEFAULT); "
        "[Console]::Out.Write($r)";
    return runGuiPowerShell(script);
}

std::string open_file() {
    const std::string script =
        "Add-Type -AssemblyName System.Windows.Forms; "
        "$d=New-Object System.Windows.Forms.OpenFileDialog; "
        "$d.Filter='All Files (*.*)|*.*'; "
        "$ok=$d.ShowDialog(); "
        "if($ok -eq [System.Windows.Forms.DialogResult]::OK){[Console]::Out.Write($d.FileName)}";
    return runGuiPowerShell(script);
}

std::string save_file() {
    const std::string script =
        "Add-Type -AssemblyName System.Windows.Forms; "
        "$d=New-Object System.Windows.Forms.SaveFileDialog; "
        "$d.Filter='All Files (*.*)|*.*'; "
        "$ok=$d.ShowDialog(); "
        "if($ok -eq [System.Windows.Forms.DialogResult]::OK){[Console]::Out.Write($d.FileName)}";
    return runGuiPowerShell(script);
}

std::string pick_folder() {
    const std::string script =
        "Add-Type -AssemblyName System.Windows.Forms; "
        "$d=New-Object System.Windows.Forms.FolderBrowserDialog; "
        "$ok=$d.ShowDialog(); "
        "if($ok -eq [System.Windows.Forms.DialogResult]::OK){[Console]::Out.Write($d.SelectedPath)}";
    return runGuiPowerShell(script);
}

void notify(const std::string& title, const std::string& text) {
    _putenv_s("VALEN_GUI_TITLE", title.c_str());
    _putenv_s("VALEN_GUI_TEXT", text.c_str());
    const std::string script =
        "$ws=New-Object -ComObject WScript.Shell; "
        "$null=$ws.Popup($env:VALEN_GUI_TEXT,3,$env:VALEN_GUI_TITLE,64)";
    runGuiPowerShell(script);
}

void set_title(const std::string& title) { SetConsoleTitleA(title.c_str()); }

std::string get_title() {
    char title[1024] = {0};
    GetConsoleTitleA(title, 1024);
    return std::string(title);
}

void show_console() { ShowWindow(GetConsoleWindow(), SW_SHOW); }
void hide_console() { ShowWindow(GetConsoleWindow(), SW_HIDE); }
void minimize_console() { ShowWindow(GetConsoleWindow(), SW_MINIMIZE); }
void maximize_console() { ShowWindow(GetConsoleWindow(), SW_MAXIMIZE); }
void restore_console() { ShowWindow(GetConsoleWindow(), SW_RESTORE); }

void set_console_pos(int x, int y) {
    HWND hwnd = GetConsoleWindow();
    RECT r = {};
    GetWindowRect(hwnd, &r);
    MoveWindow(hwnd, x, y, r.right - r.left, r.bottom - r.top, TRUE);
}

void set_console_size(int w, int h) {
    HWND hwnd = GetConsoleWindow();
    RECT r = {};
    GetWindowRect(hwnd, &r);
    MoveWindow(hwnd, r.left, r.top, w, h, TRUE);
}

int screen_width() { return GetSystemMetrics(SM_CXSCREEN); }
int screen_height() { return GetSystemMetrics(SM_CYSCREEN); }

void topmost_console(bool enabled) {
    HWND hwnd = GetConsoleWindow();
    SetWindowPos(
        hwnd,
        enabled ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
}

void create_window(const std::string& title, int width, int height) {
    HWND hwnd = currentWindow();
    if (hwnd) {
        SetWindowTextA(hwnd, title.c_str());
        RECT r = {};
        GetWindowRect(hwnd, &r);
        MoveWindow(hwnd, r.left, r.top, width, height, TRUE);
        ShowWindow(hwnd, SW_SHOW);
        return;
    }

    if (g_windowRunning.load()) {
        return;
    }

    g_windowRunning = true;
    std::thread(windowThreadMain, title, width, height).detach();
    waitForWindow(2500);
}

void clear_window() {
    HWND hwnd = waitForWindow();
    if (!hwnd) return;
    SendMessageA(hwnd, WM_VALEN_CLEAR, 0, 0);
}

int add_text(const std::string& text, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddText, text, "", x, y, w, h, id);
    return child ? id : 0;
}

int add_section(const std::string& title, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddSection, title, "", x, y, w, h, id);
    return child ? id : 0;
}

int add_button(const std::string& label, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddButton, label, "", x, y, w, h, id);
    return child ? id : 0;
}

int add_input(const std::string& placeholder, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddInput, placeholder, "", x, y, w, h, id);
    return child ? id : 0;
}

int add_editor(const std::string& text, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddEditor, text, "", x, y, w, h, id);
    return child ? id : 0;
}

int add_panel(const std::string& text, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddPanel, text, "", x, y, w, h, id);
    return child ? id : 0;
}

std::string input_text(int id) {
    auto it = g_controlHwndById.find(id);
    if (it == g_controlHwndById.end() || !it->second) return "";
    int len = GetWindowTextLengthW(it->second);
    if (len <= 0) return "";
    std::wstring wout(static_cast<size_t>(len) + 1, L'\0');
    GetWindowTextW(it->second, &wout[0], len + 1);
    wout.resize(static_cast<size_t>(len));
    return wideToUtf8(wout);
}

void set_input(int id, const std::string& text) {
    auto it = g_controlHwndById.find(id);
    if (it == g_controlHwndById.end() || !it->second) return;
    std::wstring wtext = utf8ToWide(text);
    SetWindowTextW(it->second, wtext.c_str());
    auto itType = g_controlTypeById.find(id);
    if (itType != g_controlTypeById.end() && itType->second == "editor") {
        updateEditorLineNumbers(it->second);
        applyEditorHighlight(it->second);
    }
}

int add_link(const std::string& label, const std::string& url, int x, int y, int w, int h) {
    int id = nextControlId();
    HWND child = createControl(ControlOp::AddLink, label, url, x, y, w, h, id);
    return child ? id : 0;
}

void rebuildTheme() {
    if (g_brushPanel) { DeleteObject(g_brushPanel); g_brushPanel = nullptr; }
    if (g_brushInput) { DeleteObject(g_brushInput); g_brushInput = nullptr; }
    if (g_brushEditor) { DeleteObject(g_brushEditor); g_brushEditor = nullptr; }
    if (g_brushGutter) { DeleteObject(g_brushGutter); g_brushGutter = nullptr; }
    if (g_brushButton) { DeleteObject(g_brushButton); g_brushButton = nullptr; }
    if (g_uiFont) { DeleteObject(g_uiFont); g_uiFont = nullptr; }
    if (g_codeFont) { DeleteObject(g_codeFont); g_codeFont = nullptr; }

    g_brushPanel = CreateSolidBrush(g_colorPanel);
    g_brushInput = CreateSolidBrush(g_colorInput);
    g_brushEditor = CreateSolidBrush(g_colorEditor);
    g_brushGutter = CreateSolidBrush(g_colorGutter);
    g_brushButton = CreateSolidBrush(g_colorButton);
    g_uiFont = CreateFontA(
        g_uiFontSize, 0, 0, 0, g_uiFontWeight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, g_uiFontName.c_str());
    g_codeFont = CreateFontA(
        g_codeFontSize, 0, 0, 0, g_codeFontWeight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN, g_codeFontName.c_str());

    for (const auto& kv : g_controlHwndById) {
        if (!kv.second) continue;
        const auto itType = g_controlTypeById.find(kv.first);
        if (itType != g_controlTypeById.end() && (itType->second == "editor" || itType->second == "panel")) {
            SendMessageA(kv.second, WM_SETFONT, reinterpret_cast<WPARAM>(g_codeFont), TRUE);
            SendMessageA(kv.second, EM_SETBKGNDCOLOR, 0, RGB(8, 8, 8));
        } else {
            SendMessageA(kv.second, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
            SendMessageA(kv.second, EM_SETBKGNDCOLOR, 0, RGB(8, 8, 8));
        }
    }

    HWND hwnd = currentWindow();
    if (hwnd) {
        SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(g_windowAlpha), LWA_ALPHA);
        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);
    }
}

void set_opacity(double alpha) {
    int a = static_cast<int>(alpha);
    if (a < 30) a = 30;
    if (a > 255) a = 255;
    g_windowAlpha = a;
    HWND hwnd = currentWindow();
    if (hwnd) SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(g_windowAlpha), LWA_ALPHA);
}

void set_background_gradient(double r1, double g1, double b1, double r2, double g2, double b2) {
    g_bgTop = RGB(static_cast<int>(r1), static_cast<int>(g1), static_cast<int>(b1));
    g_bgBottom = RGB(static_cast<int>(r2), static_cast<int>(g2), static_cast<int>(b2));
    HWND hwnd = currentWindow();
    if (hwnd) InvalidateRect(hwnd, nullptr, TRUE);
}

void set_panel_color(double r, double g, double b) { g_colorPanel = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }
void set_input_color(double r, double g, double b) { g_colorInput = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }
void set_editor_color(double r, double g, double b) { g_colorEditor = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }
void set_button_color(double r, double g, double b) { g_colorButton = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }
void set_text_color(double r, double g, double b) { g_colorText = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }
void set_muted_color(double r, double g, double b) { g_colorMuted = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }
void set_section_color(double r, double g, double b) { g_colorSection = RGB(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)); rebuildTheme(); }

void set_fonts(const std::string& uiName, double uiSize, double uiWeight,
               const std::string& codeName, double codeSize, double codeWeight) {
    if (!uiName.empty()) g_uiFontName = uiName;
    if (!codeName.empty()) g_codeFontName = codeName;
    g_uiFontSize = static_cast<int>(uiSize);
    g_codeFontSize = static_cast<int>(codeSize);
    g_uiFontWeight = static_cast<int>(uiWeight);
    g_codeFontWeight = static_cast<int>(codeWeight);
    rebuildTheme();
}

bool button_clicked(int id) {
    auto it = g_controlClicked.find(id);
    if (it == g_controlClicked.end()) return false;
    bool clicked = it->second;
    it->second = false;
    return clicked;
}

void set_icon(const std::string& path) {
    HWND hwnd = waitForWindow();
    if (!hwnd) return;
    HICON icon = reinterpret_cast<HICON>(
        LoadImageA(nullptr, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE)
    );
    if (!icon) return;
    if (g_windowIcon) DestroyIcon(g_windowIcon);
    g_windowIcon = icon;
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
    SendMessageA(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
}

void open_link(const std::string& url) {
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void close_window() {
    HWND hwnd = currentWindow();
    if (hwnd) {
        PostMessageA(hwnd, WM_CLOSE, 0, 0);
    }
}

bool window_open() { return currentWindow() != nullptr; }

void show_window() {
    HWND hwnd = currentWindow();
    if (hwnd) ShowWindow(hwnd, SW_SHOW);
}

void hide_window() {
    HWND hwnd = currentWindow();
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
}

void focus_window() {
    HWND hwnd = currentWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
    }
}

void set_window_title(const std::string& title) {
    HWND hwnd = currentWindow();
    if (hwnd) SetWindowTextA(hwnd, title.c_str());
}

std::string get_window_title() {
    HWND hwnd = currentWindow();
    if (!hwnd) return "";
    char title[1024] = {0};
    GetWindowTextA(hwnd, title, 1024);
    return std::string(title);
}

void set_window_pos(int x, int y) {
    HWND hwnd = currentWindow();
    if (!hwnd) return;
    RECT r = {};
    GetWindowRect(hwnd, &r);
    MoveWindow(hwnd, x, y, r.right - r.left, r.bottom - r.top, TRUE);
}

void set_window_size(int width, int height) {
    HWND hwnd = currentWindow();
    if (!hwnd) return;
    RECT r = {};
    GetWindowRect(hwnd, &r);
    MoveWindow(hwnd, r.left, r.top, width, height, TRUE);
}

int window_width() {
    HWND hwnd = currentWindow();
    if (!hwnd) return 0;
    RECT r = {};
    GetWindowRect(hwnd, &r);
    return r.right - r.left;
}

int window_height() {
    HWND hwnd = currentWindow();
    if (!hwnd) return 0;
    RECT r = {};
    GetWindowRect(hwnd, &r);
    return r.bottom - r.top;
}

void topmost_window(bool enabled) {
    HWND hwnd = currentWindow();
    if (!hwnd) return;
    SetWindowPos(
        hwnd,
        enabled ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
}

void media_menu() {
    const std::string script =
        "Add-Type -AssemblyName System.Windows.Forms; "
        "Add-Type -AssemblyName System.Drawing; "
        "$script:path=''; "
        "$script:image=$null; "
        "$script:zoom=1.0; "
        "$script:dragging=$false; "
        "$script:lastPoint=New-Object System.Drawing.Point 0,0; "
        "$form=New-Object System.Windows.Forms.Form; "
        "$form.Text='Valen Image Editor'; "
        "$form.StartPosition='CenterScreen'; "
        "$form.Size=New-Object System.Drawing.Size(1100,720); "
        "$form.MinimumSize=New-Object System.Drawing.Size(900,580); "
        "$form.AllowDrop=$true; "
        "$title=New-Object System.Windows.Forms.Label; "
        "$title.Text='Image Preview Workspace'; "
        "$title.Font=New-Object System.Drawing.Font('Segoe UI Semibold',15); "
        "$title.AutoSize=$true; "
        "$title.Location=New-Object System.Drawing.Point(20,14); "
        "$form.Controls.Add($title); "
        "$importBtn=New-Object System.Windows.Forms.Button; "
        "$importBtn.Text='Import'; "
        "$importBtn.Size=New-Object System.Drawing.Size(120,36); "
        "$importBtn.Location=New-Object System.Drawing.Point(20,52); "
        "$form.Controls.Add($importBtn); "
        "$exportBtn=New-Object System.Windows.Forms.Button; "
        "$exportBtn.Text='Export'; "
        "$exportBtn.Size=New-Object System.Drawing.Size(120,36); "
        "$exportBtn.Location=New-Object System.Drawing.Point(152,52); "
        "$form.Controls.Add($exportBtn); "
        "$metaBtn=New-Object System.Windows.Forms.Button; "
        "$metaBtn.Text='Metadata'; "
        "$metaBtn.Size=New-Object System.Drawing.Size(120,36); "
        "$metaBtn.Location=New-Object System.Drawing.Point(284,52); "
        "$form.Controls.Add($metaBtn); "
        "$status=New-Object System.Windows.Forms.Label; "
        "$status.Text='Drop an image or click Import.'; "
        "$status.AutoSize=$true; "
        "$status.Location=New-Object System.Drawing.Point(420,61); "
        "$form.Controls.Add($status); "
        "$meta=New-Object System.Windows.Forms.Label; "
        "$meta.Location=New-Object System.Drawing.Point(20,98); "
        "$meta.Size=New-Object System.Drawing.Size(360,560); "
        "$meta.BorderStyle='FixedSingle'; "
        "$meta.Text='Metadata`r`n`r`nNo image loaded.'; "
        "$meta.Padding=New-Object System.Windows.Forms.Padding(12); "
        "$form.Controls.Add($meta); "
        "$panel=New-Object System.Windows.Forms.Panel; "
        "$panel.Location=New-Object System.Drawing.Point(400,98); "
        "$panel.Size=New-Object System.Drawing.Size(670,510); "
        "$panel.BorderStyle='FixedSingle'; "
        "$panel.AutoScroll=$true; "
        "$form.Controls.Add($panel); "
        "$pic=New-Object System.Windows.Forms.PictureBox; "
        "$pic.Location=New-Object System.Drawing.Point(0,0); "
        "$pic.SizeMode='StretchImage'; "
        "$panel.Controls.Add($pic); "
        "$zoomLabel=New-Object System.Windows.Forms.Label; "
        "$zoomLabel.Text='Zoom 100%'; "
        "$zoomLabel.AutoSize=$true; "
        "$zoomLabel.Location=New-Object System.Drawing.Point(400,620); "
        "$form.Controls.Add($zoomLabel); "
        "$zoom=New-Object System.Windows.Forms.TrackBar; "
        "$zoom.Minimum=10; "
        "$zoom.Maximum=400; "
        "$zoom.Value=100; "
        "$zoom.TickFrequency=10; "
        "$zoom.SmallChange=5; "
        "$zoom.LargeChange=20; "
        "$zoom.Size=New-Object System.Drawing.Size(670,45); "
        "$zoom.Location=New-Object System.Drawing.Point(400,640); "
        "$form.Controls.Add($zoom); "
        "$loadImage={ "
        "param([string]$p) "
        "if([string]::IsNullOrWhiteSpace($p) -or -not (Test-Path $p)){ return } "
        "try { "
        "$tmp=[System.Drawing.Image]::FromFile($p); "
        "$bmp=New-Object System.Drawing.Bitmap $tmp; "
        "$tmp.Dispose(); "
        "if($script:image -ne $null){ $script:image.Dispose() } "
        "$script:image=$bmp; "
        "$script:path=$p; "
        "$status.Text='Loaded: ' + $p; "
        "$dpiX=[Math]::Round($bmp.HorizontalResolution,2); "
        "$dpiY=[Math]::Round($bmp.VerticalResolution,2); "
        "if($dpiX -le 0){ $dpiX=72 } "
        "if($dpiY -le 0){ $dpiY=72 } "
        "$inW=[Math]::Round($bmp.Width / $dpiX,2); "
        "$inH=[Math]::Round($bmp.Height / $dpiY,2); "
        "$f=Get-Item $p; "
        "$mb=[Math]::Round($f.Length / 1MB, 2); "
        "$meta.Text='Metadata`r`n`r`nPath: ' + $p + '`r`n' + "
        "'File Size: ' + $mb + ' MB (' + $f.Length + ' bytes)`r`n' + "
        "'Pixel Size: ' + $bmp.Width + ' x ' + $bmp.Height + '`r`n' + "
        "'DPI: ' + $dpiX + ' x ' + $dpiY + '`r`n' + "
        "'Actual Size: ' + $inW + ' x ' + $inH + ' inches'; "
        "$script:zoom=$zoom.Value / 100.0; "
        "$pic.Image=$script:image; "
        "$pic.Width=[Math]::Max(1,[int]($script:image.Width * $script:zoom)); "
        "$pic.Height=[Math]::Max(1,[int]($script:image.Height * $script:zoom)); "
        "$panel.AutoScrollPosition=New-Object System.Drawing.Point(0,0); "
        "} catch { [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Import error','OK','Error') | Out-Null } "
        "}; "
        "$importBtn.Add_Click({ "
        "$d=New-Object System.Windows.Forms.OpenFileDialog; "
        "$d.Filter='Images|*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp;*.tif;*.tiff|All Files|*.*'; "
        "$d.Title='Import Image'; "
        "if($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){ & $loadImage $d.FileName } "
        "}); "
        "$exportBtn.Add_Click({ "
        "if([string]::IsNullOrWhiteSpace($script:path)){ "
        "[System.Windows.Forms.MessageBox]::Show('Import an image first.','Export','OK','Warning') | Out-Null; return } "
        "$d=New-Object System.Windows.Forms.SaveFileDialog; "
        "$d.Filter='PNG|*.png|JPEG|*.jpg|Bitmap|*.bmp|All Files|*.*'; "
        "$d.Title='Export Image'; "
        "if($d.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK){ "
        "try { [System.IO.File]::Copy($script:path,$d.FileName,$true); "
        "$status.Text='Exported: ' + $d.FileName } "
        "catch { [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Export error','OK','Error') | Out-Null } "
        "} "
        "}); "
        "$metaBtn.Add_Click({ [System.Windows.Forms.MessageBox]::Show($meta.Text,'Metadata') | Out-Null }); "
        "$zoom.Add_Scroll({ "
        "$zoomLabel.Text='Zoom ' + $zoom.Value + '%'; "
        "if($script:image -ne $null){ "
        "$script:zoom=$zoom.Value / 100.0; "
        "$pic.Width=[Math]::Max(1,[int]($script:image.Width * $script:zoom)); "
        "$pic.Height=[Math]::Max(1,[int]($script:image.Height * $script:zoom)); "
        "} "
        "}); "
        "$pic.Add_MouseDown({ $script:dragging=$true; $script:lastPoint=$_.Location; $pic.Cursor='SizeAll' }); "
        "$pic.Add_MouseUp({ $script:dragging=$false; $pic.Cursor='Default' }); "
        "$pic.Add_MouseMove({ "
        "if($script:dragging){ "
        "$dx=$_.X - $script:lastPoint.X; "
        "$dy=$_.Y - $script:lastPoint.Y; "
        "$newX = -$panel.AutoScrollPosition.X - $dx; "
        "$newY = -$panel.AutoScrollPosition.Y - $dy; "
        "$panel.AutoScrollPosition = New-Object System.Drawing.Point($newX,$newY); "
        "} "
        "}); "
        "$form.Add_DragEnter({ if($_.Data.GetDataPresent([System.Windows.Forms.DataFormats]::FileDrop)){ $_.Effect='Copy' } }); "
        "$form.Add_DragDrop({ "
        "$files=$_.Data.GetData([System.Windows.Forms.DataFormats]::FileDrop); "
        "if($files -and $files.Length -gt 0){ & $loadImage $files[0] } "
        "}); "
        "$form.Add_FormClosed({ if($script:image -ne $null){ $script:image.Dispose() } }); "
        "[void]$form.ShowDialog();";
    runGuiPowerShell(script);
}

} // namespace gui_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("gui", [](Interpreter& interp) {
                    interp.registerModuleFunction("gui", "msgbox", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.msgbox");
                        gui_lib::msgbox(interp.expectString(args[0], "gui.msgbox expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "confirm", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.confirm");
                        bool ok = gui_lib::confirm(interp.expectString(args[0], "gui.confirm expects string"));
                        return Value::fromBool(ok);
                    });
                    interp.registerModuleFunction("gui", "prompt", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.prompt");
                        return Value::fromString(gui_lib::prompt(interp.expectString(args[0], "gui.prompt expects string")));
                    });
                    interp.registerModuleFunction("gui", "beep", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.beep");
                        gui_lib::beep();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "info", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.info");
                        gui_lib::info(interp.expectString(args[0], "gui.info expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "warning", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.warning");
                        gui_lib::warning(interp.expectString(args[0], "gui.warning expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "error", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.error");
                        gui_lib::error(interp.expectString(args[0], "gui.error expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "yesnocancel", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.yesnocancel");
                        return Value::fromNumber(static_cast<double>(gui_lib::yesnocancel(interp.expectString(args[0], "gui.yesnocancel expects string"))));
                    });
                    interp.registerModuleFunction("gui", "prompt_default", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.prompt_default");
                        return Value::fromString(gui_lib::prompt_default(
                            interp.expectString(args[0], "gui.prompt_default expects prompt string"),
                            interp.expectString(args[1], "gui.prompt_default expects default string")));
                    });
                    interp.registerModuleFunction("gui", "open_file", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.open_file");
                        return Value::fromString(gui_lib::open_file());
                    });
                    interp.registerModuleFunction("gui", "save_file", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.save_file");
                        return Value::fromString(gui_lib::save_file());
                    });
                    interp.registerModuleFunction("gui", "pick_folder", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.pick_folder");
                        return Value::fromString(gui_lib::pick_folder());
                    });
                    interp.registerModuleFunction("gui", "notify", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.notify");
                        gui_lib::notify(
                            interp.expectString(args[0], "gui.notify expects title string"),
                            interp.expectString(args[1], "gui.notify expects text string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_title", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.set_title");
                        gui_lib::set_title(interp.expectString(args[0], "gui.set_title expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "get_title", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.get_title");
                        return Value::fromString(gui_lib::get_title());
                    });
                    interp.registerModuleFunction("gui", "show_console", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.show_console");
                        gui_lib::show_console();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "hide_console", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.hide_console");
                        gui_lib::hide_console();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "minimize_console", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.minimize_console");
                        gui_lib::minimize_console();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "maximize_console", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.maximize_console");
                        gui_lib::maximize_console();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "restore_console", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.restore_console");
                        gui_lib::restore_console();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_console_pos", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.set_console_pos");
                        gui_lib::set_console_pos(
                            static_cast<int>(interp.expectNumber(args[0], "gui.set_console_pos expects x number")),
                            static_cast<int>(interp.expectNumber(args[1], "gui.set_console_pos expects y number")));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_console_size", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.set_console_size");
                        gui_lib::set_console_size(
                            static_cast<int>(interp.expectNumber(args[0], "gui.set_console_size expects width number")),
                            static_cast<int>(interp.expectNumber(args[1], "gui.set_console_size expects height number")));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "screen_width", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.screen_width");
                        return Value::fromNumber(static_cast<double>(gui_lib::screen_width()));
                    });
                    interp.registerModuleFunction("gui", "screen_height", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.screen_height");
                        return Value::fromNumber(static_cast<double>(gui_lib::screen_height()));
                    });
                    interp.registerModuleFunction("gui", "topmost_console", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.topmost_console");
                        gui_lib::topmost_console(interp.isTruthyPublic(args[0]));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "create_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.create_window");
                        gui_lib::create_window(
                            interp.expectString(args[0], "gui.create_window expects title string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.create_window expects width number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.create_window expects height number")));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "clear_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.clear_window");
                        gui_lib::clear_window();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "add_text", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 5, "gui.add_text");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_text(
                            interp.expectString(args[0], "gui.add_text expects text string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.add_text expects x number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_text expects y number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_text expects width number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_text expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "add_section", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 5, "gui.add_section");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_section(
                            interp.expectString(args[0], "gui.add_section expects title string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.add_section expects x number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_section expects y number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_section expects width number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_section expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "add_button", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 5, "gui.add_button");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_button(
                            interp.expectString(args[0], "gui.add_button expects label string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.add_button expects x number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_button expects y number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_button expects width number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_button expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "button_clicked", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.button_clicked");
                        return Value::fromBool(gui_lib::button_clicked(
                            static_cast<int>(interp.expectNumber(args[0], "gui.button_clicked expects id number"))));
                    });
                    interp.registerModuleFunction("gui", "add_input", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 5, "gui.add_input");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_input(
                            interp.expectString(args[0], "gui.add_input expects placeholder string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.add_input expects x number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_input expects y number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_input expects width number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_input expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "add_editor", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 5, "gui.add_editor");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_editor(
                            interp.expectString(args[0], "gui.add_editor expects text string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.add_editor expects x number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_editor expects y number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_editor expects width number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_editor expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "add_panel", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 5, "gui.add_panel");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_panel(
                            interp.expectString(args[0], "gui.add_panel expects text string"),
                            static_cast<int>(interp.expectNumber(args[1], "gui.add_panel expects x number")),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_panel expects y number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_panel expects width number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_panel expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "input_text", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.input_text");
                        return Value::fromString(gui_lib::input_text(
                            static_cast<int>(interp.expectNumber(args[0], "gui.input_text expects id number"))));
                    });
                    interp.registerModuleFunction("gui", "set_input", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.set_input");
                        gui_lib::set_input(
                            static_cast<int>(interp.expectNumber(args[0], "gui.set_input expects id number")),
                            interp.expectString(args[1], "gui.set_input expects text string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "add_link", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 6, "gui.add_link");
                        return Value::fromNumber(static_cast<double>(gui_lib::add_link(
                            interp.expectString(args[0], "gui.add_link expects label string"),
                            interp.expectString(args[1], "gui.add_link expects url string"),
                            static_cast<int>(interp.expectNumber(args[2], "gui.add_link expects x number")),
                            static_cast<int>(interp.expectNumber(args[3], "gui.add_link expects y number")),
                            static_cast<int>(interp.expectNumber(args[4], "gui.add_link expects width number")),
                            static_cast<int>(interp.expectNumber(args[5], "gui.add_link expects height number")))));
                    });
                    interp.registerModuleFunction("gui", "set_icon", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.set_icon");
                        gui_lib::set_icon(interp.expectString(args[0], "gui.set_icon expects path string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "open_link", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.open_link");
                        gui_lib::open_link(interp.expectString(args[0], "gui.open_link expects url string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_opacity", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.set_opacity");
                        gui_lib::set_opacity(interp.expectNumber(args[0], "gui.set_opacity expects number"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_background_gradient", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 6, "gui.set_background_gradient");
                        gui_lib::set_background_gradient(
                            interp.expectNumber(args[0], "gui.set_background_gradient expects r1"),
                            interp.expectNumber(args[1], "gui.set_background_gradient expects g1"),
                            interp.expectNumber(args[2], "gui.set_background_gradient expects b1"),
                            interp.expectNumber(args[3], "gui.set_background_gradient expects r2"),
                            interp.expectNumber(args[4], "gui.set_background_gradient expects g2"),
                            interp.expectNumber(args[5], "gui.set_background_gradient expects b2"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_panel_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_panel_color");
                        gui_lib::set_panel_color(
                            interp.expectNumber(args[0], "gui.set_panel_color expects r"),
                            interp.expectNumber(args[1], "gui.set_panel_color expects g"),
                            interp.expectNumber(args[2], "gui.set_panel_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_input_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_input_color");
                        gui_lib::set_input_color(
                            interp.expectNumber(args[0], "gui.set_input_color expects r"),
                            interp.expectNumber(args[1], "gui.set_input_color expects g"),
                            interp.expectNumber(args[2], "gui.set_input_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_editor_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_editor_color");
                        gui_lib::set_editor_color(
                            interp.expectNumber(args[0], "gui.set_editor_color expects r"),
                            interp.expectNumber(args[1], "gui.set_editor_color expects g"),
                            interp.expectNumber(args[2], "gui.set_editor_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_button_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_button_color");
                        gui_lib::set_button_color(
                            interp.expectNumber(args[0], "gui.set_button_color expects r"),
                            interp.expectNumber(args[1], "gui.set_button_color expects g"),
                            interp.expectNumber(args[2], "gui.set_button_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_text_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_text_color");
                        gui_lib::set_text_color(
                            interp.expectNumber(args[0], "gui.set_text_color expects r"),
                            interp.expectNumber(args[1], "gui.set_text_color expects g"),
                            interp.expectNumber(args[2], "gui.set_text_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_muted_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_muted_color");
                        gui_lib::set_muted_color(
                            interp.expectNumber(args[0], "gui.set_muted_color expects r"),
                            interp.expectNumber(args[1], "gui.set_muted_color expects g"),
                            interp.expectNumber(args[2], "gui.set_muted_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_section_color", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 3, "gui.set_section_color");
                        gui_lib::set_section_color(
                            interp.expectNumber(args[0], "gui.set_section_color expects r"),
                            interp.expectNumber(args[1], "gui.set_section_color expects g"),
                            interp.expectNumber(args[2], "gui.set_section_color expects b"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_fonts", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 6, "gui.set_fonts");
                        gui_lib::set_fonts(
                            interp.expectString(args[0], "gui.set_fonts expects ui name"),
                            interp.expectNumber(args[1], "gui.set_fonts expects ui size"),
                            interp.expectNumber(args[2], "gui.set_fonts expects ui weight"),
                            interp.expectString(args[3], "gui.set_fonts expects code name"),
                            interp.expectNumber(args[4], "gui.set_fonts expects code size"),
                            interp.expectNumber(args[5], "gui.set_fonts expects code weight"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "close_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.close_window");
                        gui_lib::close_window();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "window_open", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.window_open");
                        return Value::fromBool(gui_lib::window_open());
                    });
                    interp.registerModuleFunction("gui", "show_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.show_window");
                        gui_lib::show_window();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "hide_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.hide_window");
                        gui_lib::hide_window();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "focus_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.focus_window");
                        gui_lib::focus_window();
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_window_title", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.set_window_title");
                        gui_lib::set_window_title(interp.expectString(args[0], "gui.set_window_title expects string"));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "get_window_title", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.get_window_title");
                        return Value::fromString(gui_lib::get_window_title());
                    });
                    interp.registerModuleFunction("gui", "set_window_pos", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.set_window_pos");
                        gui_lib::set_window_pos(
                            static_cast<int>(interp.expectNumber(args[0], "gui.set_window_pos expects x number")),
                            static_cast<int>(interp.expectNumber(args[1], "gui.set_window_pos expects y number")));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "set_window_size", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 2, "gui.set_window_size");
                        gui_lib::set_window_size(
                            static_cast<int>(interp.expectNumber(args[0], "gui.set_window_size expects width number")),
                            static_cast<int>(interp.expectNumber(args[1], "gui.set_window_size expects height number")));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "window_width", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.window_width");
                        return Value::fromNumber(static_cast<double>(gui_lib::window_width()));
                    });
                    interp.registerModuleFunction("gui", "window_height", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.window_height");
                        return Value::fromNumber(static_cast<double>(gui_lib::window_height()));
                    });
                    interp.registerModuleFunction("gui", "topmost_window", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 1, "gui.topmost_window");
                        gui_lib::topmost_window(interp.isTruthyPublic(args[0]));
                        return Value::fromNumber(0.0);
                    });
                    interp.registerModuleFunction("gui", "media_menu", [&interp](const std::vector<Value>& args) -> Value {
                        interp.expectArity(args, 0, "gui.media_menu");
                        gui_lib::media_menu();
                        return Value::fromNumber(0.0);
                    });

    });
}
