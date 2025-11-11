#include "violet/about_dialog.h"
#include <commctrl.h>
#include <shellapi.h>

namespace violet {

AboutDialog::AboutDialog()
    : hwnd_(nullptr)
    , parentWindow_(nullptr) {
}

AboutDialog::~AboutDialog() {
}

void AboutDialog::Show(HWND parentWindow) {
    parentWindow_ = parentWindow;
    
    // Register window class for dialog
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = StaticDialogProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"VioletAboutDialog";
    
    static bool classRegistered = false;
    if (!classRegistered) {
        if (!RegisterClassEx(&wc)) {
            MessageBox(parentWindow, L"Failed to register about dialog class", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        classRegistered = true;
    }
    
    // Create the dialog window
    hwnd_ = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
        L"VioletAboutDialog",
        L"About Violet",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 320,
        parentWindow,
        nullptr,
        GetModuleHandle(nullptr),
        this  // Pass this pointer via CreateParams
    );
    
    if (!hwnd_) {
        DWORD error = GetLastError();
        wchar_t buf[256];
        wsprintf(buf, L"Failed to create about dialog. Error: %d", error);
        MessageBox(parentWindow, buf, L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Initialize dialog
    OnInitDialog(hwnd_);
    
    // Center dialog
    RECT rcParent, rcDialog;
    GetWindowRect(parentWindow, &rcParent);
    GetWindowRect(hwnd_, &rcDialog);
    int x = rcParent.left + (rcParent.right - rcParent.left - (rcDialog.right - rcDialog.left)) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDialog.bottom - rcDialog.top)) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    // Show the window
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    
    // Run modal message loop
    MSG msg;
    EnableWindow(parentWindow, FALSE);
    
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsWindow(hwnd_)) {
            break;
        }
        
        if (!IsDialogMessage(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(parentWindow, TRUE);
    SetForegroundWindow(parentWindow);
}

void AboutDialog::OnInitDialog(HWND hwnd) {
    hwnd_ = hwnd;
    
    // Create logo area (static control for custom painting)
    CreateWindow(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        20, 20, 360, 80,
        hwnd_, (HMENU)IDC_LOGO_STATIC, GetModuleHandle(nullptr), nullptr);
    
    int yPos = 110;
    int spacing = 25;
    
    // App name and version
    CreateWindow(L"STATIC", L"Violet v0.78",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        20, yPos, 360, 20,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    yPos += spacing;
    
    // Description
    CreateWindow(L"STATIC", APP_DESCRIPTION,
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        20, yPos, 360, 20,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    yPos += spacing;
    
    // Copyright
    CreateWindow(L"STATIC", APP_COPYRIGHT,
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        20, yPos, 360, 20,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    yPos += spacing;
    
    // License
    CreateWindow(L"STATIC", APP_LICENSE,
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        20, yPos, 360, 20,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    yPos += spacing;
    
    // Website link (clickable)
    HWND hLink = CreateWindow(L"STATIC", APP_WEBSITE,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
        20, yPos, 360, 20,
        hwnd_, (HMENU)1002, GetModuleHandle(nullptr), nullptr);
    
    // Make it blue and underlined
    HFONT hFont = CreateFont(
        -14, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessage(hLink, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Change cursor to hand when hovering
    SetClassLongPtr(hLink, GCLP_HCURSOR, (LONG_PTR)LoadCursor(nullptr, IDC_HAND));
    
    // OK button
    CreateWindow(L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        160, 250, 80, 30,
        hwnd_, (HMENU)IDC_OK_BUTTON, GetModuleHandle(nullptr), nullptr);
    
    // Set fonts for better appearance
    HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    EnumChildWindows(hwnd_, [](HWND hwndChild, LPARAM lParam) -> BOOL {
        SendMessage(hwndChild, WM_SETFONT, lParam, TRUE);
        return TRUE;
    }, (LPARAM)hDefaultFont);
}

LRESULT CALLBACK AboutDialog::StaticDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AboutDialog* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        // Get the this pointer from CreateParams
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<AboutDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<AboutDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT AboutDialog::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        OnCommand(wParam);
        return 0;
        
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
        if (pDIS->CtlID == IDC_LOGO_STATIC) {
            DrawLogo(pDIS->hDC, pDIS->rcItem);
            return TRUE;
        }
        break;
    }
    
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        
        // Make website link blue
        if (GetDlgCtrlID(hwndStatic) == 1002) {
            SetTextColor(hdcStatic, RGB(0, 102, 204));
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        break;
    }
    
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
        
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void AboutDialog::OnCommand(WPARAM wParam) {
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);
    
    switch (wmId) {
    case IDC_OK_BUTTON:
        DestroyWindow(hwnd_);
        break;
        
    case 1002: // Website link
        if (wmEvent == STN_CLICKED) {
            ShellExecute(nullptr, L"open", APP_WEBSITE, nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    }
}

void AboutDialog::DrawLogo(HDC hdc, const RECT& rect) {
    // Set background
    HBRUSH hBrush = CreateSolidBrush(RGB(98, 52, 136)); // Purple gradient start
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
    
    // Create gradient effect
    for (int y = rect.top; y < rect.bottom; y++) {
        int progress = ((y - rect.top) * 255) / (rect.bottom - rect.top);
        HBRUSH hGradBrush = CreateSolidBrush(RGB(98 - progress/6, 52 - progress/6, 136 - progress/4));
        RECT lineRect = { rect.left, y, rect.right, y + 1 };
        FillRect(hdc, &lineRect, hGradBrush);
        DeleteObject(hGradBrush);
    }
    
    // Draw app name in logo area
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    
    HFONT hFont = CreateFont(
        -36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    DrawTextW(hdc, APP_NAME, -1, (LPRECT)&rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    
    // Draw border
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(70, 35, 100));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
}

} // namespace violet
