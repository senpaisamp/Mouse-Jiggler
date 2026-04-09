#include <windows.h>
#include <stdlib.h>
#include <time.h>

#define TIMER_JIGGLE  3001
#define TIMER_ANIM    3002
#define WM_TRAYICON   (WM_USER + 1)

// Colori
#define COLOR_NAVY_DARK    RGB(10, 25, 47)
#define COLOR_SUCCESS      RGB(0, 200, 150)
#define COLOR_OFF          RGB(70, 85, 100)
#define COLOR_WHITE        RGB(255, 255, 255)
#define COLOR_TEXT         RGB(240, 245, 255)
#define COLOR_SIGNATURE    RGB(100, 120, 140)

static BOOL g_isJiggling = FALSE;
static float g_anim = 0.0f;
static BOOL g_hover = FALSE;
static HWND g_hWnd = NULL;
static HICON g_hIcon = NULL;
static HICON g_hIconSmall = NULL;

// Doppio buffer
static HDC g_hdcMem = NULL;
static HBITMAP g_hbmMem = NULL;

void DoMouseJiggle() {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    int dx = (rand() % 3) - 1;
    int dy = (rand() % 3) - 1;
    input.mi.dx = dx;
    input.mi.dy = dy;
    SendInput(1, &input, sizeof(INPUT));
}

void CALLBACK JiggleProc(HWND hWnd, UINT, UINT_PTR, DWORD) {
    if (g_isJiggling) DoMouseJiggle();
}

void CALLBACK AnimProc(HWND hWnd, UINT, UINT_PTR, DWORD) {
    float oldAnim = g_anim;
    
    if (g_isJiggling && g_anim < 1.0f)
        g_anim += 0.1f;
    else if (!g_isJiggling && g_anim > 0.0f)
        g_anim -= 0.1f;
    
    if (oldAnim != g_anim) {
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void Toggle() {
    g_isJiggling = !g_isJiggling;
    if (g_isJiggling)
        SetTimer(g_hWnd, TIMER_JIGGLE, 2000, JiggleProc);
    else
        KillTimer(g_hWnd, TIMER_JIGGLE);
    
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void AddTrayIcon(HWND hWnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    
    nid.hIcon = g_hIconSmall ? g_hIconSmall : LoadIcon(NULL, IDI_APPLICATION);
    
    wcscpy_s(nid.szTip, 128, L"Mouse Jiggler Pro - Click to restore");
    
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hWnd) {
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    
    AppendMenuW(hMenu, MF_STRING, 1, L"✧ Ripristina");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 2, L"✖ Esci");
    
    POINT pt;
    GetCursorPos(&pt);
    
    // Mostra il menu
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0);
    
    DestroyMenu(hMenu);
}

void HideToTray(HWND hWnd) {
    ShowWindow(hWnd, SW_HIDE);
    AddTrayIcon(hWnd);
}

void RestoreFromTray(HWND hWnd) {
    ShowWindow(hWnd, SW_SHOW);
    ShowWindow(hWnd, SW_RESTORE);
    SetForegroundWindow(hWnd);
    RemoveTrayIcon(hWnd);
}

void ExitApplication(HWND hWnd) {
    RemoveTrayIcon(hWnd);
    DestroyWindow(hWnd);
}

void DrawRoundRect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF color) {
    HRGN rgn = CreateRoundRectRgn(x, y, x + w, y + h, radius * 2, radius * 2);
    HBRUSH brush = CreateSolidBrush(color);
    FillRgn(hdc, rgn, brush);
    DeleteObject(brush);
    DeleteObject(rgn);
}

void DrawCircle(HDC hdc, int x, int y, int size, COLORREF color) {
    HRGN rgn = CreateEllipticRgn(x, y, x + size, y + size);
    HBRUSH brush = CreateSolidBrush(color);
    FillRgn(hdc, rgn, brush);
    DeleteObject(brush);
    DeleteObject(rgn);
}

void DrawSwitch(HDC hdc, int x, int y, int w, int h, float anim, BOOL hover) {
    COLORREF trackColor = (anim > 0.5f) ? COLOR_SUCCESS : COLOR_OFF;
    
    if (hover && anim <= 0.5f) {
        trackColor = RGB(
            min(GetRValue(trackColor) + 25, 255),
            min(GetGValue(trackColor) + 25, 255),
            min(GetBValue(trackColor) + 25, 255)
        );
    }
    
    DrawRoundRect(hdc, x, y, w, h, h / 2, trackColor);
    
    int knobSize = h - 8;
    int knobX = x + 4 + (int)(anim * (w - knobSize - 8));
    int knobY = y + 4;
    
    DrawCircle(hdc, knobX, knobY, knobSize, COLOR_WHITE);
}

void DrawUI(HWND hWnd, HDC hdc) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    
    HBRUSH bgBrush = CreateSolidBrush(COLOR_NAVY_DARK);
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);
    
    HPEN linePen = CreatePen(PS_SOLID, 2, COLOR_SUCCESS);
    HPEN oldPen = SelectObject(hdc, linePen);
    MoveToEx(hdc, 0, 60, NULL);
    LineTo(hdc, rc.right, 60);
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
    
    HFONT titleFont = CreateFontW(-28, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT subFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT statusFont = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT signatureFont = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 1, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    
    SetBkMode(hdc, TRANSPARENT);
    
    SetTextColor(hdc, COLOR_TEXT);
    SelectObject(hdc, titleFont);
    RECT titleRect = {0, 80, rc.right, 120};
    DrawTextW(hdc, L"Mouse Jiggler", -1, &titleRect, DT_CENTER);
    
    SetTextColor(hdc, RGB(150, 170, 200));
    SelectObject(hdc, subFont);
    RECT subRect = {0, 125, rc.right, 150};
    DrawTextW(hdc, L"Keep your computer awake", -1, &subRect, DT_CENTER);
    
    int sw = 150, sh = 56;
    int x = (rc.right / 2) - (sw / 2);
    int y = 185;
    DrawSwitch(hdc, x, y, sw, sh, g_anim, g_hover);
    
    SelectObject(hdc, statusFont);
    RECT statusRect = {0, 265, rc.right, 295};
    
    if (g_isJiggling) {
        SetTextColor(hdc, COLOR_SUCCESS);
        DrawTextW(hdc, L"● ACTIVE", -1, &statusRect, DT_CENTER);
    } else {
        SetTextColor(hdc, RGB(130, 150, 180));
        DrawTextW(hdc, L"○ INACTIVE", -1, &statusRect, DT_CENTER);
    }
    
    SelectObject(hdc, signatureFont);
    SetTextColor(hdc, COLOR_SIGNATURE);
    RECT signatureRect = {10, rc.bottom - 30, 200, rc.bottom - 10};
    DrawTextW(hdc, L"by @senpaisamp", -1, &signatureRect, DT_LEFT);
    
    DeleteObject(titleFont);
    DeleteObject(subFont);
    DeleteObject(statusFont);
    DeleteObject(signatureFont);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    
    case WM_CREATE: {
        srand((unsigned int)time(NULL));
        g_hWnd = hWnd;
        SetTimer(hWnd, TIMER_ANIM, 16, AnimProc);
        
        HINSTANCE hInst = GetModuleHandle(NULL);
        
        g_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(1));
        g_hIconSmall = (HICON)LoadImage(hInst, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, 0);
        
        if (!g_hIcon) {
            g_hIcon = LoadIcon(NULL, IDI_APPLICATION);
        }
        if (!g_hIconSmall) {
            g_hIconSmall = LoadIcon(NULL, IDI_APPLICATION);
        }
        
        if (g_hIcon) {
            SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);
        }
        break;
    }
        
    case WM_SIZE:
        if (g_hdcMem) DeleteDC(g_hdcMem);
        if (g_hbmMem) DeleteObject(g_hbmMem);
        g_hdcMem = NULL;
        g_hbmMem = NULL;
        break;
    
    case WM_MOUSEMOVE: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        int sw = 150, sh = 56;
        int x = (rc.right / 2) - (sw / 2);
        int y = 185;
        RECT switchRect = {x, y, x + sw, y + sh};
        
        BOOL wasHover = g_hover;
        g_hover = PtInRect(&switchRect, pt);
        
        if (wasHover != g_hover) {
            InvalidateRect(hWnd, &switchRect, FALSE);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        int sw = 150, sh = 56;
        int x = (rc.right / 2) - (sw / 2);
        int y = 185;
        RECT switchRect = {x, y, x + sw, y + sh};
        
        if (PtInRect(&switchRect, pt)) {
            Toggle();
        }
        break;
    }
    
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONDBLCLK) {
            RestoreFromTray(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hWnd);
        }
        break;
    
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
            case 1:  // Ripristina
                RestoreFromTray(hWnd);
                break;
            case 2:  // Esci
                ExitApplication(hWnd);
                break;
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        if (!g_hdcMem || !g_hbmMem) {
            g_hdcMem = CreateCompatibleDC(hdc);
            g_hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(g_hdcMem, g_hbmMem);
        }
        
        DrawUI(hWnd, g_hdcMem);
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, g_hdcMem, 0, 0, SRCCOPY);
        
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        HideToTray(hWnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ANIM);
        KillTimer(hWnd, TIMER_JIGGLE);
        RemoveTrayIcon(hWnd);
        if (g_hIcon) DestroyIcon(g_hIcon);
        if (g_hIconSmall) DestroyIcon(g_hIconSmall);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = CreateSolidBrush(COLOR_NAVY_DARK);
    wc.lpszClassName = L"JigglerPro";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(1));
    
    if (!wc.hIcon) {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    RegisterClassW(&wc);
    
    HWND hWnd = CreateWindowW(
        L"JigglerPro",
        L"Mouse Jiggler Pro",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        480, 380,
        NULL, NULL, hInst, NULL
    );
    
    ShowWindow(hWnd, nShow);
    
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}