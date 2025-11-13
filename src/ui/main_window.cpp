#include "violet/main_window.h"
#include "violet/plugin_browser.h"
#include "violet/active_plugins_panel.h"
#include "violet/plugin_parameters_window.h"
#include "violet/plugin_manager.h"
#include "violet/audio_engine.h"
#include "violet/audio_processing_chain.h"
#include "violet/theme_manager.h"
#include "violet/session_manager.h"
#include "violet/audio_settings_dialog.h"
#include "violet/about_dialog.h"
#include "violet/utils.h"
#include "violet/resource.h"
#include <commctrl.h>
#include <windowsx.h>
#include <iostream>

namespace violet {

const wchar_t* MainWindow::CLASS_NAME = L"VioletMainWindow";

MainWindow::MainWindow() 
    : hwnd_(nullptr)
    , hStatusBar_(nullptr)
    , hToolBar_(nullptr)
    , hInstance_(nullptr)
    , titleFont_(nullptr)
    , normalFont_(nullptr)
    , borderless_(false)  // Disabled for now to show menu bar
    , titleBarHeight_(0)
    , borderWidth_(0)
    , isDragging_(false)
    , dragOffset_{0, 0} {
}

MainWindow::~MainWindow() {
    if (titleFont_) DeleteObject(titleFont_);
    if (normalFont_) DeleteObject(normalFont_);
    
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool MainWindow::Create(HINSTANCE hInstance) {
    hInstance_ = hInstance;
    
    // Initialize DPI scaling
    DpiScaling::Instance().Initialize();
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = Theme::Instance().GetBackgroundBrush();
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        return false;
    }
    
    // Create the window with modern borderless style
    DWORD style = borderless_ ? (WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX) 
                               : WS_OVERLAPPEDWINDOW;
    DWORD exStyle = WS_EX_APPWINDOW;
    
    int scaledWidth = DPI_SCALE(DEFAULT_WIDTH);
    int scaledHeight = DPI_SCALE(DEFAULT_HEIGHT);
    
    hwnd_ = CreateWindowEx(
        exStyle,
        CLASS_NAME,
        L"Violet - LV2 Plugin Host",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        scaledWidth, scaledHeight,
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
        
    case WM_DPICHANGED: {
        UINT dpi = HIWORD(wParam);
        RECT* rect = reinterpret_cast<RECT*>(lParam);
        OnDpiChanged(dpi, rect);
        return 0;
    }
    
    case WM_NCCALCSIZE: {
        if (borderless_ && wParam == TRUE) {
            LRESULT result = 0;
            OnNcCalcSize(wParam, lParam, result);
            return result;
        }
        break;
    }
    
    case WM_NCHITTEST: {
        LRESULT result = 0;
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        OnNcHitTest(x, y, result);
        if (result != 0) return result;
        break;
    }
    
    case WM_NCPAINT:
        if (borderless_) {
            OnNcPaint();
            return 0;
        }
        break;
        
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        mmi->ptMinTrackSize.x = DPI_SCALE(MIN_WIDTH);
        mmi->ptMinTrackSize.y = DPI_SCALE(MIN_HEIGHT);
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
    
    case WM_USER + 200: {
        // Custom message: drag-drop plugin URI from browser
        if (lParam != 0 && activePluginsPanel_) {
            const char* uri = reinterpret_cast<const char*>(lParam);
            activePluginsPanel_->LoadPluginFromUri(uri);
        }
        return 0;
    }
    
    case WM_TIMER:
        // Update status bar with audio stats
        if (wParam == 1 && audioEngine_ && processingChain_) {
            if (hStatusBar_) {
                // Update CPU usage
                double cpu = processingChain_->GetCpuUsage();
                std::wstring cpuText = L"CPU: " + std::to_wstring(static_cast<int>(cpu)) + L"%";
                SendMessage(hStatusBar_, SB_SETTEXT, 2, (LPARAM)cpuText.c_str());
                
                // Update audio status
                if (audioEngine_->IsRunning()) {
                    double latency = audioEngine_->GetLatency();
                    std::wstring statusText = L"Audio: Running (" + 
                        std::to_wstring(static_cast<int>(latency)) + L"ms)";
                    SendMessage(hStatusBar_, SB_SETTEXT, 1, (LPARAM)statusText.c_str());
                } else {
                    SendMessage(hStatusBar_, SB_SETTEXT, 1, (LPARAM)L"Audio: Stopped");
                }
            }
        }
        return 0;
        
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
    
    return DefWindowProc(hwnd_, uMsg, wParam, lParam);
}

void MainWindow::OnCreate() {
    // Initialize DPI-aware fonts
    titleBarHeight_ = DPI_SCALE(32);
    borderWidth_ = DPI_SCALE(1);
    
    titleFont_ = Theme::Instance().CreateScaledFont(DPI_SCALE(14), FW_SEMIBOLD);
    normalFont_ = Theme::Instance().CreateScaledFont(DPI_SCALE(11), FW_NORMAL);
    
    // Load theme preferences
    ThemeManager::GetInstance().LoadFromConfig();
    
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
            // Set format to match audio engine - will be updated when audio starts
        processingChain_->SetFormat(44100, 2, 256);
    }
    
    // Initialize session manager
    sessionManager_ = std::make_unique<SessionManager>();
    
    // Set up audio callback to process through chain
    if (audioEngine_ && processingChain_) {
        // Pre-allocate audio buffers to avoid reallocation during processing
        audioBufferLeft_.resize(1024);  // Max expected buffer size
        audioBufferRight_.resize(1024);
        audioBufferLeftOut_.resize(1024);
        audioBufferRightOut_.resize(1024);
        
        audioEngine_->SetAudioCallback(
            [](float* input, float* output, uint32_t frames, void* userData) {
                auto* mainWindow = static_cast<MainWindow*>(userData);
                if (!mainWindow || !mainWindow->processingChain_) {
                    // Bypass: copy input to output
                    if (input && output) {
                        memcpy(output, input, frames * 2 * sizeof(float));
                    }
                    return;
                }
                
                auto* chain = mainWindow->processingChain_.get();
                
                // De-interleave input: WASAPI provides interleaved stereo (L,R,L,R,...)
                // Chain expects planar buffers (LLLL..., RRRR...)
                std::vector<float>& leftIn = mainWindow->audioBufferLeft_;
                std::vector<float>& rightIn = mainWindow->audioBufferRight_;
                std::vector<float>& leftOut = mainWindow->audioBufferLeftOut_;
                std::vector<float>& rightOut = mainWindow->audioBufferRightOut_;
                
                // Ensure buffers are large enough (but don't shrink to avoid reallocation)
                if (leftIn.size() < frames) {
                    leftIn.resize(frames);
                    rightIn.resize(frames);
                    leftOut.resize(frames);
                    rightOut.resize(frames);
                }
                
                // Validate input/output pointers
                if (!input || !output) {
                    std::cerr << "Error: NULL audio buffer in callback" << std::endl;
                    return;
                }
                
                // De-interleave input
                for (uint32_t i = 0; i < frames; ++i) {
                    leftIn[i] = input[i * 2];
                    rightIn[i] = input[i * 2 + 1];
                }
                
                // Create planar buffer pointers
                float* inputBuffers[2] = { leftIn.data(), rightIn.data() };
                float* outputBuffers[2] = { leftOut.data(), rightOut.data() };
                
                // Validate buffer pointers
                if (!inputBuffers[0] || !inputBuffers[1] || !outputBuffers[0] || !outputBuffers[1]) {
                    std::cerr << "Error: NULL buffer pointer after data()" << std::endl;
                    memcpy(output, input, frames * 2 * sizeof(float));
                    return;
                }
                
                // Process through the plugin chain
                // Note: Process all frames at once - the chain will handle chunking internally
                chain->Process(inputBuffers, outputBuffers, 2, frames);
                
                // Interleave output back to WASAPI format
                for (uint32_t i = 0; i < frames; ++i) {
                    output[i * 2] = leftOut[i];
                    output[i * 2 + 1] = rightOut[i];
                }
            },
            this  // Pass MainWindow as user data
        );
        
        // Set audio format in engine to match chain
        AudioFormat format;
        format.sampleRate = 44100;
        format.channels = 2;
        format.bufferSize = 256;
        format.bitsPerSample = 32;
        audioEngine_->SetFormat(format);
        
        // Start audio engine automatically
        if (audioEngine_->Start()) {
            // Update processing chain with actual audio format
            AudioFormat actualFormat = audioEngine_->GetFormat();
            processingChain_->SetFormat(actualFormat.sampleRate, actualFormat.channels, actualFormat.bufferSize);
            std::cout << "Updated processing chain to use " << actualFormat.sampleRate << " Hz" << std::endl;
            
            if (hStatusBar_) {
                SendMessage(hStatusBar_, SB_SETTEXT, 1, (LPARAM)L"Audio: Running");
            }
        }
    }
    
    // Create UI components
    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar();
    CreateControls();
    UpdateLayout();
    
    // Apply theme to main window
    ThemeManager::GetInstance().ApplyToWindow(hwnd_);
    
    // Set up timer for status bar updates (every 500ms)
    SetTimer(hwnd_, 1, 500, nullptr);
    
    // Update status bar with initial state
    if (hStatusBar_) {
        std::wstring pluginCount = L"Plugins: " + std::to_wstring(
            pluginManager_ ? pluginManager_->GetAvailablePlugins().size() : 0);
        SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)pluginCount.c_str());
    }
}

void MainWindow::OnDestroy() {
    // Kill timer
    KillTimer(hwnd_, 1);
    
    // Stop audio engine before destroying
    if (audioEngine_ && audioEngine_->IsRunning()) {
        audioEngine_->Stop();
    }
    
    PostQuitMessage(0);
}

void MainWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    const auto& colors = Theme::Instance().GetColors();
    
    // Fill with themed background color
    HBRUSH bgBrush = Theme::Instance().GetBackgroundBrush();
    FillRect(hdc, &ps.rcPaint, bgBrush);
    
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
    case IDM_NEW:
        OnNewSession();
        break;
    
    case IDM_OPEN:
        OnOpenSession();
        break;
    
    case IDM_SAVE:
        OnSaveSession();
        break;
    
    case IDM_SAVEAS:
        OnSaveSessionAs();
        break;
    
    case IDM_EXIT:
        DestroyWindow(hwnd_);
        break;
    
    case IDM_VIEW_THEME_LIGHT:
        ThemeManager::GetInstance().SetTheme(ThemeType::Light);
        break;
    
    case IDM_VIEW_THEME_DARK:
        ThemeManager::GetInstance().SetTheme(ThemeType::Dark);
        break;
    
    case IDM_VIEW_THEME_SYSTEM:
        ThemeManager::GetInstance().SetTheme(ThemeType::System);
        break;
    
    case IDM_AUDIO_SETTINGS:
        OnAudioSettings();
        break;
    
    case IDM_AUDIO_START:
        if (audioEngine_ && !audioEngine_->IsRunning()) {
            if (audioEngine_->Start()) {
                if (hStatusBar_) {
                    SendMessage(hStatusBar_, SB_SETTEXT, 1, (LPARAM)L"Audio: Running");
                }
            } else {
                std::wstring errorMsg = L"Failed to start audio engine.\n\nPossible reasons:\n";
                errorMsg += L"• No audio output device available\n";
                errorMsg += L"• Audio device is in use by another application\n";
                errorMsg += L"• Invalid audio format settings\n\n";
                errorMsg += L"Please check Audio > Audio Settings to configure devices.";
                MessageBox(hwnd_, errorMsg.c_str(), L"Audio Engine Error", MB_OK | MB_ICONERROR);
            }
        }
        break;
    
    case IDM_AUDIO_STOP:
        if (audioEngine_ && audioEngine_->IsRunning()) {
            audioEngine_->Stop();
            if (hStatusBar_) {
                SendMessage(hStatusBar_, SB_SETTEXT, 1, (LPARAM)L"Audio: Stopped");
            }
        }
        break;
    
    case IDM_ABOUT:
        OnAbout();
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
    
    HMENU hViewMenu = CreatePopupMenu();
    HMENU hThemeMenu = CreatePopupMenu();
    AppendMenu(hThemeMenu, MF_STRING, IDM_VIEW_THEME_LIGHT, L"&Light");
    AppendMenu(hThemeMenu, MF_STRING, IDM_VIEW_THEME_DARK, L"&Dark");
    AppendMenu(hThemeMenu, MF_STRING, IDM_VIEW_THEME_SYSTEM, L"&System Default");
    AppendMenu(hViewMenu, MF_POPUP, (UINT_PTR)hThemeMenu, L"&Theme");
    
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, IDM_ABOUT, L"&About Violet");
    
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hAudioMenu, L"&Audio");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
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

void MainWindow::OnNewSession() {
    if (!sessionManager_) return;
    
    // TODO: Check for unsaved changes and prompt user
    
    // Clear current session
    if (processingChain_) {
        processingChain_->ClearChain();
    }
    
    if (activePluginsPanel_) {
        activePluginsPanel_->ClearPlugins();
    }
    
    sessionManager_->NewSession();
    
    if (hStatusBar_) {
        SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)L"New Session");
    }
}

void MainWindow::OnOpenSession() {
    if (!sessionManager_ || !processingChain_ || !pluginManager_) return;
    
    // Show file open dialog
    wchar_t fileName[MAX_PATH] = L"";
    
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Violet Session Files (*.violet)\\0*.violet\\0All Files (*.*)\\0*.*\\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"violet";
    ofn.lpstrTitle = L"Open Violet Session";
    
    if (GetOpenFileName(&ofn)) {
        std::string filePath = utils::WStringToString(fileName);
        
        if (sessionManager_->LoadSession(filePath, processingChain_.get(), pluginManager_.get())) {
            // Update UI
            if (activePluginsPanel_) {
                activePluginsPanel_->ClearPlugins();
                
                // Add loaded plugins to UI
                auto nodeIds = processingChain_->GetNodeIds();
                for (uint32_t nodeId : nodeIds) {
                    auto node = processingChain_->GetNode(nodeId);
                    if (node && node->GetPlugin()) {
                        std::string name = node->GetPlugin()->GetInfo().name;
                        std::string uri = node->GetPlugin()->GetInfo().uri;
                        activePluginsPanel_->AddPlugin(nodeId, name, uri);
                    }
                }
            }
            
            if (hStatusBar_) {
                std::wstring msg = L"Loaded: " + utils::StringToWString(filePath);
                SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)msg.c_str());
            }
        } else {
            MessageBox(hwnd_, L"Failed to load session file", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnSaveSession() {
    if (!sessionManager_ || !processingChain_) return;
    
    std::string currentPath = sessionManager_->GetCurrentSessionPath();
    
    if (currentPath.empty()) {
        // No current session, use Save As
        OnSaveSessionAs();
    } else {
        // Save to current path
        if (sessionManager_->SaveSession(currentPath, processingChain_.get())) {
            if (hStatusBar_) {
                std::wstring msg = L"Saved: " + utils::StringToWString(currentPath);
                SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)msg.c_str());
            }
        } else {
            MessageBox(hwnd_, L"Failed to save session file", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnSaveSessionAs() {
    if (!sessionManager_ || !processingChain_) return;
    
    // Show file save dialog
    wchar_t fileName[MAX_PATH] = L"";
    
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Violet Session Files (*.violet)\\0*.violet\\0All Files (*.*)\\0*.*\\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"violet";
    ofn.lpstrTitle = L"Save Violet Session As";
    
    if (GetSaveFileName(&ofn)) {
        std::string filePath = utils::WStringToString(fileName);
        
        if (sessionManager_->SaveSession(filePath, processingChain_.get())) {
            if (hStatusBar_) {
                std::wstring msg = L"Saved: " + utils::StringToWString(filePath);
                SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)msg.c_str());
            }
        } else {
            MessageBox(hwnd_, L"Failed to save session file", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnAudioSettings() {
    if (!audioEngine_) {
        MessageBox(hwnd_, L"Audio engine not initialized", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Create and show audio settings dialog
    AudioSettingsDialog settingsDialog;
    if (settingsDialog.Show(hwnd_, audioEngine_.get())) {
        // Settings were applied successfully
        if (hStatusBar_) {
            SendMessage(hStatusBar_, SB_SETTEXT, 0, (LPARAM)L"Audio settings updated");
        }
    }
}

void MainWindow::OnAbout() {
    AboutDialog aboutDialog;
    aboutDialog.Show(hwnd_);
}

void MainWindow::OnDpiChanged(UINT dpi, const RECT* rect) {
    // Update fonts
    if (titleFont_) DeleteObject(titleFont_);
    if (normalFont_) DeleteObject(normalFont_);
    
    titleBarHeight_ = DPI_SCALE_WINDOW(32, hwnd_);
    borderWidth_ = DPI_SCALE_WINDOW(1, hwnd_);
    
    titleFont_ = Theme::Instance().CreateScaledFont(DPI_SCALE_WINDOW(14, hwnd_), FW_SEMIBOLD);
    normalFont_ = Theme::Instance().CreateScaledFont(DPI_SCALE_WINDOW(11, hwnd_), FW_NORMAL);
    
    // Resize window
    DpiScaling::Instance().OnDpiChanged(hwnd_, dpi, rect);
    
    // Update layout
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    OnSize(clientRect.right, clientRect.bottom);
    
    // Force repaint
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void MainWindow::OnNcCalcSize(WPARAM wParam, LPARAM lParam, LRESULT& result) {
    // Remove default window border while keeping drop shadow
    if (wParam == TRUE) {
        NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
        
        // Adjust for borderless window
        params->rgrc[0].top += 0;  // No adjustment needed for top
        params->rgrc[0].bottom += 0;
        params->rgrc[0].left += 0;
        params->rgrc[0].right += 0;
        
        result = 0;
    }
}

void MainWindow::OnNcPaint() {
    // Custom non-client area painting for borderless window
    HDC hdc = GetWindowDC(hwnd_);
    if (!hdc) return;
    
    RECT rect;
    GetWindowRect(hwnd_, &rect);
    OffsetRect(&rect, -rect.left, -rect.top);
    
    // Draw custom title bar
    const auto& colors = Theme::Instance().GetColors();
    HBRUSH titleBrush = CreateSolidBrush(colors.surface);
    RECT titleRect = rect;
    titleRect.bottom = titleBarHeight_;
    FillRect(hdc, &titleRect, titleBrush);
    DeleteObject(titleBrush);
    
    // Draw title text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, colors.onSurface);
    SelectObject(hdc, titleFont_);
    
    RECT textRect = titleRect;
    textRect.left += DPI_SCALE(12);
    DrawText(hdc, L"Violet - LV2 Plugin Host", -1, &textRect, 
             DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw border
    HPEN borderPen = CreatePen(PS_SOLID, borderWidth_, colors.border);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    
    ReleaseDC(hwnd_, hdc);
}

void MainWindow::OnNcHitTest(int x, int y, LRESULT& result) {
    if (!borderless_) {
        result = 0;
        return;
    }
    
    RECT rect;
    GetWindowRect(hwnd_, &rect);
    
    // Convert screen coordinates to window coordinates
    int wx = x - rect.left;
    int wy = y - rect.top;
    
    int borderSize = DPI_SCALE(8);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Check for resize areas
    bool isLeft = wx < borderSize;
    bool isRight = wx > width - borderSize;
    bool isTop = wy < borderSize;
    bool isBottom = wy > height - borderSize;
    
    if (isTop && isLeft) result = HTTOPLEFT;
    else if (isTop && isRight) result = HTTOPRIGHT;
    else if (isBottom && isLeft) result = HTBOTTOMLEFT;
    else if (isBottom && isRight) result = HTBOTTOMRIGHT;
    else if (isTop) result = HTTOP;
    else if (isBottom) result = HTBOTTOM;
    else if (isLeft) result = HTLEFT;
    else if (isRight) result = HTRIGHT;
    else if (wy < titleBarHeight_) {
        // Title bar area - allow dragging
        // Check for window controls (close, min, max buttons would go here)
        result = HTCAPTION;
    }
    else {
        result = 0;
    }
}

} // namespace violet