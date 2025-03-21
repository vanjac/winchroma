#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")

#define CHROMA_MAIN int main(int, char**) { \
    return _tWinMain(GetModuleHandle(NULL), NULL, NULL, SW_SHOWNORMAL); \
}

namespace chroma {

/* DEBUGGING */

#ifndef CHROMA_DEBUG

#define LOG(...)
#define LOG_FORMAT(...)
#define CHECKERR(expr) (expr)

#else // CHROMA_DEBUG

#define LOG(message) _tprintf(_T("%s:%d:  %S\n"), \
    chroma::impl::fileName(_T(__FILE__)), __LINE__, (message))
#define LOG_FORMAT(format, ...) _tprintf(_T("%s:%d:  ") _T(format) _T("\n"), \
    chroma::impl::fileName(_T(__FILE__)), __LINE__, __VA_ARGS__)
#define CHECKERR(expr) chroma::impl::checkErr((expr), _T(__FILE__), __LINE__, _T(#expr))

namespace impl {

    inline const TCHAR *fileName(const TCHAR *path) {
        // avoid including shlwapi.h for PathFindFileName
        const TCHAR *name = _tcsrchr(path, _T('\\'));
        return name ? (name + 1) : path;
    }

    inline void logLastError(const TCHAR *file, int line, const TCHAR *expr) {
        DWORD error = GetLastError();
        TCHAR msgbuf[256];
        msgbuf[0] = 0;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, msgbuf, _countof(msgbuf), NULL);
        _tprintf(_T("Error %d: %s    in %s  (%s:%d)\n"), error, msgbuf, expr, fileName(file), line);
    }

    template <typename T>
    T checkErr(T result, const TCHAR *file, int line, const TCHAR *expr) {
        if (!result)
            logLastError(file, line, expr);
        return result;
    }

}

#endif // CHROMA_DEBUG

/* C++ UTILS */

// Get a pointer to the temporary result of an expression
template <typename T>
T* tempPtr(T &&x) { return &x; }

} // namespace

/* GEOMETRY UTILS */

inline bool operator==(const POINT &a, const POINT &b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const POINT &a, const POINT &b) {
    return !(a == b);
}

inline bool operator==(const SIZE &a, const SIZE &b) {
    return a.cx == b.cx && a.cy == b.cy;
}

inline bool operator!=(const SIZE &a, const SIZE &b) {
    return !(a == b);
}

namespace chroma {

constexpr int rectWidth(const RECT &rect) {
    return rect.right - rect.left;
}

constexpr int rectHeight(const RECT &rect) {
    return rect.bottom - rect.top;
}

constexpr SIZE rectSize(const RECT &rect) {
    return {rectWidth(rect), rectHeight(rect)};
}

constexpr RECT makeRect(POINT topLeft, SIZE size = {}) {
    return {topLeft.x, topLeft.y, topLeft.x + size.cx, topLeft.y + size.cy};
}

inline RECT inflateRect(RECT rect, SIZE amount) {
    InflateRect(&rect, amount.cx, amount.cy);
    return rect;
}

inline RECT offsetRect(RECT rect, SIZE amount) {
    OffsetRect(&rect, amount.cx, amount.cy);
    return rect;
}

/* WINDOW UTILS */

const WNDCLASSEX SCRATCH_CLASS = {
    sizeof(SCRATCH_CLASS),          // cbSize
    0,                              // style
    DefWindowProc,                  // lpfnWndProc
    0,                              // cbClsExtra
    0,                              // cbWndExtra
    GetModuleHandle(NULL),          // hInstance
    NULL,                           // hIcon
    LoadCursor(NULL, IDC_ARROW),    // hCursor
    NULL,                           // hbrBackground
    NULL,                           // lpszMenuName
    _T("Scratch"),                  // lpszClassName
    NULL,                           // hIconSm
};

// initialize the most common fields of WNDCLASSEX
inline WNDCLASSEX makeClass(const TCHAR *name, WNDPROC proc) {
    WNDCLASSEX wndClass = SCRATCH_CLASS;
    wndClass.lpszClassName = name;
    wndClass.lpfnWndProc = proc;
    return wndClass;
}

inline int simpleMessageLoop(HWND mainWindow = NULL, HACCEL accel = NULL) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (accel && TranslateAccelerator(mainWindow, accel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return int(msg.wParam);
}

inline RECT defaultWindowRect(SIZE clientSize, bool menu = false,
        DWORD style = WS_OVERLAPPEDWINDOW, WORD exStyle = 0) {
    RECT rect = {0, 0, clientSize.cx, clientSize.cy};
    AdjustWindowRectEx(&rect, style, menu, exStyle);
    return {CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT + rectWidth(rect), CW_USEDEFAULT + rectHeight(rect)};
}

inline HWND createWindow(
        const TCHAR *className,
        const TCHAR *windowName = NULL,
        const RECT &windowRect = {},
        DWORD style = WS_OVERLAPPEDWINDOW,
        DWORD exStyle = 0,
        HWND owner = NULL,
        HMENU menu = NULL,
        void *param = NULL) {
    return CreateWindowEx(exStyle, className, windowName, style,
        windowRect.left, windowRect.top, rectWidth(windowRect), rectHeight(windowRect),
        owner, menu, GetModuleHandle(NULL), param);
}

inline HWND createChildWindow(
        HWND parent,
        const TCHAR *className,
        const TCHAR *windowName = NULL,
        const RECT &windowRect = {},
        DWORD style = WS_VISIBLE,
        DWORD exStyle = 0,
        int ctrlId = 0,
        void *param = NULL) {
    return CreateWindowEx(exStyle, className, windowName, style | WS_CHILD,
        windowRect.left, windowRect.top, rectWidth(windowRect), rectHeight(windowRect),
        parent, HMENU(size_t(ctrlId)), GetModuleHandle(NULL), param);
}

inline RECT windowRect(HWND hwnd) {
    RECT rect = {};
    CHECKERR(GetWindowRect(hwnd, &rect));
    return rect;
}

inline RECT clientRect(HWND hwnd) {
    RECT rect = {};
    CHECKERR(GetClientRect(hwnd, &rect));
    return rect;
}

inline SIZE clientSize(HWND hwnd) {
    RECT rect = clientRect(hwnd);
    return {rect.right, rect.bottom};
}

inline POINT screenToClient(HWND hwnd, POINT screenPt) {
    CHECKERR(ScreenToClient(hwnd, &screenPt));
    return screenPt;
}

inline POINT clientToScreen(HWND hwnd, POINT clientPt) {
    CHECKERR(ClientToScreen(hwnd, &clientPt));
    return clientPt;
}

inline POINT cursorPos() {
    POINT pos = {};
    CHECKERR(GetCursorPos(&pos));
    return pos;
}

inline void setCursorHitTest(HWND wnd, POINT pt) {
    UINT code = UINT(SendMessage(wnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y)));
    SendMessage(wnd, WM_SETCURSOR, WPARAM(wnd), MAKELPARAM(code, WM_MOUSEMOVE));
}

struct WindowImpl {
    HWND wnd;
    virtual ~WindowImpl() = default;
    virtual const TCHAR * className() const = 0;
    virtual LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    inline HWND create(
            const TCHAR *windowName = NULL,
            const RECT &windowRect = {},
            DWORD style = WS_OVERLAPPEDWINDOW,
            DWORD exStyle = 0,
            HWND owner = NULL,
            HMENU menu = NULL) {
        return createWindow(className(), windowName, windowRect, style, exStyle, owner, menu, this);
    }
    inline HWND createChild(
            HWND parent,
            const TCHAR *windowName = NULL,
            const RECT &windowRect = {},
            DWORD style = WS_VISIBLE,
            DWORD exStyle = 0,
            int ctrlId = 0) {
        return createChildWindow(
            parent, className(), windowName, windowRect, style, exStyle, ctrlId, this);
    }
};

inline LRESULT CALLBACK windowImplProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowImpl *self;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT *>(lParam);
        self = reinterpret_cast<WindowImpl *>(create->lpCreateParams);
        self->wnd = wnd;
        SetWindowLongPtr(wnd, GWLP_USERDATA, LONG_PTR(self));
    } else {
        self = reinterpret_cast<WindowImpl *>(GetWindowLongPtr(wnd, GWLP_USERDATA));
    }
    if (self) {
        return self->handleMessage(msg, wParam, lParam);
    } else {
        return DefWindowProc(wnd, msg, wParam, lParam);
    }
}

/* RESOURCE UTILS */

// unicode only. must use /n flag with rc.exe!
inline const WCHAR * getString(HINSTANCE inst, UINT id) {
    WCHAR *str = NULL;
    LoadStringW(inst, id, reinterpret_cast<WCHAR *>(&str), 0);
    return str;
}

inline const void * getResource(const TCHAR *name, const TCHAR *type = RT_RCDATA,
        HINSTANCE inst = NULL, DWORD *sizeOut = NULL) {
    HRSRC res = FindResource(inst, name, type);
    if (sizeOut)
        *sizeOut = SizeofResource(inst, res);
    HGLOBAL mem = LoadResource(inst, res);
    return LockResource(mem);
}

/* GDI UTILS */

inline SIZE bitmapSize(HBITMAP hbitmap) {
    BITMAP bitmap = {};
    GetObject(hbitmap, sizeof(bitmap), &bitmap);
    return {bitmap.bmWidth, bitmap.bmHeight};
}

// general-purpose RAII wrapper
template <typename T, typename CloseFnType, CloseFnType CloseFn>
struct CResource {
    CResource() : _obj(NULL) {}
    CResource(T obj) : _obj(obj) {}
    ~CResource() {
        reset();
    }

    T & operator=(T &h) {
        reset();
        _obj = h;
    }
    T * operator&() {
        reset();
        return &_obj;
    }
    operator T() { return _obj; }

    void reset() {
        if (_obj != NULL)
            CHECKERR(CloseFn(_obj));
        _obj = NULL;
    }

    T release() {
        T val = _obj;
        _obj = NULL;
        return val;
    }

    T _obj;
};

template <typename T>
using CGDIObj = CResource<T, BOOL (__stdcall *)(HGDIOBJ), DeleteObject>;

struct CDC : CResource<HDC, BOOL (__stdcall *)(HDC), DeleteDC> {
    CDC() : CResource() {}
    CDC(HDC dc) : CResource(dc) {}
    // Create memory device context
    CDC(HDC existing, HGDIOBJ obj) {
        _obj = CHECKERR(CreateCompatibleDC(existing));
        SelectObject(_obj, obj);
    }
};

/* COMMON CONTROL UTILS */

namespace impl {
    const char ignore = (InitCommonControls(), 0);
}

inline void updateToolbarStates(HWND toolbar, HMENU menu) {
    int numButtons = int(SendMessage(toolbar, TB_BUTTONCOUNT, 0, 0));
    for (int i = 0; i < numButtons; i++) {
        TBBUTTONINFO btnInfo = {sizeof(btnInfo), TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE};
        SendMessage(toolbar, TB_GETBUTTONINFO, i, LPARAM(&btnInfo));
        if (btnInfo.fsStyle & BTNS_SEP) continue;
        UINT menuState = GetMenuState(menu, btnInfo.idCommand, MF_BYCOMMAND);
        UINT btnState = ((menuState & MF_GRAYED) ? TBSTATE_INDETERMINATE : TBSTATE_ENABLED)
            | ((menuState & MF_CHECKED) ? TBSTATE_CHECKED : 0);
        SendMessage(toolbar, TB_SETSTATE, btnInfo.idCommand, btnState);
    }
}

inline void handleToolbarTip(NMTTDISPINFO *info, HMENU menu) {
    GetMenuString(menu, UINT(info->hdr.idFrom), info->szText, _countof(info->szText), 0);
    if (TCHAR *tab = _tcschr(info->szText, L'\t'))
        *tab = '\n';
}

/* COMMON DIALOG UTILS */

// initialize the most common fields of OPENFILENAME
inline OPENFILENAME makeOpenFileName(TCHAR *fileBuf, HWND owner, const TCHAR *filters,
        const TCHAR *defExt = NULL) {
    OPENFILENAME open = {sizeof(open)};
    open.hwndOwner = owner;
    open.lpstrFilter = filters;
    open.lpstrFile = fileBuf;
    open.nMaxFile = MAX_PATH;
    open.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    open.lpstrDefExt = defExt;
    return open;
}

} // namespace
