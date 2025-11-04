#include "violet/main_window.h"
#include "violet/resource.h"
#include <commctrl.h>
#include <windowsx.h>

namespace violet {

const wchar_t* MainWindow::CLASS_NAME = L"VioletMainWindow";

MainWindow::MainWindow() 
    : hwnd_(nullptr)
    , hStatusBar_(nullptr)
    , hToolBar_(nullptr)
    , hInstance_(nullptr) {
}

MainWindow::~MainWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool MainWindow::Create(HINSTANCE hInstance) {
    hInstance_ = hInstance;
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        return false;
    }
    
    // Create the window
    hwnd_ = CreateWindowEx(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Violet - LV2 Plugin Host",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DEFAULT_WIDTH, DEFAULT_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        this  // Pass 'this' pointer to WM_CREATE
    );
    
    if (!hwnd_) {
        return false;
    }
    
    // Set minimum window size
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    return true;
}

void MainWindow::Show(int nCmdShow) {
    if (hwnd_) {
        ShowWindow(hwnd_, nCmdShow);
        UpdateWindow(hwnd_);
    }
}

void MainWindow::Hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void MainWindow::SetSize(int width, int height) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, 0, 0, width, height, 
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void MainWindow::GetSize(int& width, int& height) const {
    if (hwnd_) {
        RECT rect;
        GetClientRect(hwnd_, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hwnd_ = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        OnCreate();
        return 0;
        
    case WM_DESTROY:
        OnDestroy();
        return 0;
        
    case WM_PAINT:
        OnPaint();
        return 0;
        
    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
        
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = MIN_WIDTH;
        mmi->ptMinTrackSize.y = MIN_HEIGHT;
        return 0;
    }
    
    case WM_COMMAND:
        OnCommand(wParam, lParam);
        return 0;
        
    case WM_NOTIFY: {
        // Handle notifications from child controls
        return 0;
    }
    
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void MainWindow::OnCreate() {
    // Create child controls
    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar();
    CreateControls();
    UpdateLayout();
}

void MainWindow::OnDestroy() {
    PostQuitMessage(0);
}

void MainWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    // For now, just fill with window background color
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
    
    // Draw some placeholder text
    SetTextColor(hdc, RGB(100, 100, 100));
    SetBkMode(hdc, TRANSPARENT);
    RECT rect;
    GetClientRect(hwnd_, &rect);
    DrawText(hdc, L"Violet LV2 Plugin Host - Basic Window Created", -1, &rect, 
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    EndPaint(hwnd_, &ps);
}

void MainWindow::OnSize(int width, int height) {
    (void)width;   // Suppress unused parameter warning
    (void)height;  // Suppress unused parameter warning
    UpdateLayout();
}

void MainWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
    (void)lParam;  // Suppress unused parameter warning
    int wmId = LOWORD(wParam);
    
    switch (wmId) {
    case IDM_EXIT:
        DestroyWindow(hwnd_);
        break;
        
    default:
        break;
    }
}

void MainWindow::CreateControls() {
    // TODO: Create plugin browser panel, active plugins panel, etc.
    // For now, this is just a placeholder
}

void MainWindow::CreateMenuBar() {
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    
    AppendMenu(hFileMenu, MF_STRING, IDM_NEW, L"&New Session");
    AppendMenu(hFileMenu, MF_STRING, IDM_OPEN, L"&Open Session...");
    AppendMenu(hFileMenu, MF_STRING, IDM_SAVE, L"&Save Session");
    AppendMenu(hFileMenu, MF_STRING, IDM_SAVEAS, L"Save Session &As...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, L"E&xit");
    
    HMENU hAudioMenu = CreatePopupMenu();
    AppendMenu(hAudioMenu, MF_STRING, IDM_AUDIO_SETTINGS, L"Audio &Settings...");
    AppendMenu(hAudioMenu, MF_STRING, IDM_AUDIO_START, L"&Start Audio Engine");
    AppendMenu(hAudioMenu, MF_STRING, IDM_AUDIO_STOP, L"St&op Audio Engine");
    
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, IDM_ABOUT, L"&About Violet");
    
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hAudioMenu, L"&Audio");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
    
    SetMenu(hwnd_, hMenuBar);
}

void MainWindow::CreateStatusBar() {
    hStatusBar_ = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hwnd_,
        nullptr,
        hInstance_,
        nullptr
    );
    
    if (hStatusBar_) {
        // Set up status bar parts
        int parts[] = { 200, 400, -1 };
        SendMessage(hStatusBar_, SB_SETPARTS, 3, (LPARAM)parts);
        
        // Set initial text
        SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)L"Ready");
        SendMessage(hStatusBar_, SB_SETTEXT, 1, (LPARAM)L"Audio: Stopped");
        SendMessage(hStatusBar_, SB_SETTEXT, 2, (LPARAM)L"CPU: 0%");
    }
}

void MainWindow::CreateToolBar() {
    hToolBar_ = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        0, 0, 0, 0,
        hwnd_,
        nullptr,
        hInstance_,
        nullptr
    );
    
    if (hToolBar_) {
        SendMessage(hToolBar_, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        
        // TODO: Add toolbar buttons for play/stop, etc.
        // For now, just create an empty toolbar
    }
}

void MainWindow::UpdateLayout() {
    if (!hwnd_) return;
    
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    
    int toolbarHeight = 0;
    int statusBarHeight = 0;
    
    // Resize toolbar
    if (hToolBar_) {
        SendMessage(hToolBar_, TB_AUTOSIZE, 0, 0);
        RECT toolbarRect;
        GetWindowRect(hToolBar_, &toolbarRect);
        toolbarHeight = toolbarRect.bottom - toolbarRect.top;
    }
    
    // Resize status bar
    if (hStatusBar_) {
        SendMessage(hStatusBar_, WM_SIZE, 0, 0);
        RECT statusRect;
        GetWindowRect(hStatusBar_, &statusRect);
        statusBarHeight = statusRect.bottom - statusRect.top;
    }
    
    // TODO: Layout plugin browser and active plugins panels
    // in the remaining client area
}

} // namespace violet