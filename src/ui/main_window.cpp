#include "violet/main_window.h"
#include "violet/plugin_browser.h"
#include "violet/active_plugins_panel.h"
#include "violet/plugin_parameters_window.h"
#include "violet/plugin_manager.h"
#include "violet/audio_engine.h"
#include "violet/audio_processing_chain.h"
#include "violet/utils.h"
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
        
    case WM_USER + 101: {
        // Custom message: open plugin parameters
        uint32_t nodeId = (uint32_t)wParam;
        ShowPluginParameters(nodeId);
        return 0;
    }
        
    case WM_NOTIFY: {
        NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
        
        // Handle notifications from child controls
        // Check if it's a double-click notification
        if (pnmhdr->code == NM_DBLCLK) {
            // Check if it came from plugin browser's tree view
            if (pluginBrowser_) {
                std::string pluginUri = pluginBrowser_->GetSelectedPluginUri();
                if (!pluginUri.empty()) {
                    LoadPlugin(pluginUri);
                }
            }
        }
        return 0;
    }
    
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void MainWindow::OnCreate() {
    // Initialize backend components
    pluginManager_ = std::make_unique<PluginManager>();
    if (pluginManager_) {
        pluginManager_->Initialize();
    }
    
    audioEngine_ = std::make_unique<AudioEngine>();
    if (audioEngine_) {
        audioEngine_->Initialize();
    }
    
    processingChain_ = std::make_unique<AudioProcessingChain>(audioEngine_.get());
    if (processingChain_) {
        // Set default format
        processingChain_->SetFormat(44100, 2, 256);
    }
    
    // Create UI components
    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar();
    CreateControls();
    UpdateLayout();
    
    // Update status bar with initial state
    if (hStatusBar_) {
        std::wstring pluginCount = L"Plugins: " + std::to_wstring(
            pluginManager_ ? pluginManager_->GetAvailablePlugins().size() : 0);
        SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)pluginCount.c_str());
    }
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
    // Create plugin browser on the left side
    pluginBrowser_ = std::make_unique<PluginBrowser>();
    if (pluginBrowser_) {
        RECT clientRect;
        GetClientRect(hwnd_, &clientRect);
        
        pluginBrowser_->Create(hwnd_, hInstance_, 0, 0, PLUGIN_BROWSER_WIDTH, 
                              clientRect.bottom - clientRect.top);
        
        if (pluginManager_) {
            pluginBrowser_->SetPluginManager(pluginManager_.get());
        }
    }
    
    // Create active plugins panel on the right side
    activePluginsPanel_ = std::make_unique<ActivePluginsPanel>();
    if (activePluginsPanel_) {
        RECT clientRect;
        GetClientRect(hwnd_, &clientRect);
        
        activePluginsPanel_->Create(hwnd_, hInstance_, PLUGIN_BROWSER_WIDTH, 0,
                                   clientRect.right - PLUGIN_BROWSER_WIDTH,
                                   clientRect.bottom - clientRect.top);
        
        if (processingChain_) {
            activePluginsPanel_->SetProcessingChain(processingChain_.get());
        }
    }
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

void MainWindow::LoadPlugin(const std::string& pluginUri) {
    if (!processingChain_ || !pluginManager_ || !activePluginsPanel_) {
        return;
    }
    
    // Get plugin info
    PluginInfo info = pluginManager_->GetPluginInfo(pluginUri);
    if (info.uri.empty()) {
        return;
    }
    
    // Add plugin to processing chain
    uint32_t nodeId = processingChain_->AddPlugin(pluginUri);
    if (nodeId == 0) {
        MessageBox(hwnd_, L"Failed to load plugin", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Add plugin to visual display
    activePluginsPanel_->AddPlugin(nodeId, info.name, pluginUri);
    
    // Update status bar
    if (hStatusBar_) {
        std::wstring status = L"Loaded: " + utils::StringToWString(info.name);
        SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)status.c_str());
    }
}

void MainWindow::ShowPluginParameters(uint32_t nodeId) {
    if (!processingChain_ || nodeId == 0) {
        return;
    }
    
    // Create parameters window if it doesn't exist
    if (!parametersWindow_) {
        parametersWindow_ = std::make_unique<PluginParametersWindow>();
        if (!parametersWindow_->Create(hInstance_, hwnd_)) {
            parametersWindow_.reset();
            return;
        }
    }
    
    // Set the plugin to display
    parametersWindow_->SetPlugin(processingChain_.get(), nodeId);
    
    // Show the window
    parametersWindow_->Show();
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
    
    // Calculate available client area
    int availableTop = toolbarHeight;
    int availableHeight = clientRect.bottom - toolbarHeight - statusBarHeight;
    int availableWidth = clientRect.right;
    
    // Layout plugin browser on the left
    if (pluginBrowser_) {
        pluginBrowser_->Resize(0, availableTop, PLUGIN_BROWSER_WIDTH, availableHeight);
    }
    
    // Layout active plugins panel on the right
    if (activePluginsPanel_) {
        activePluginsPanel_->Resize(PLUGIN_BROWSER_WIDTH, availableTop,
                                   availableWidth - PLUGIN_BROWSER_WIDTH, availableHeight);
    }
}

} // namespace violet