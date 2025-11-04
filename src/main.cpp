#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <memory>
#include "violet/main_window.h"
#include "violet/config_manager.h"

#pragma comment(lib, "comctl32.lib")

// Global application instance
HINSTANCE g_hInstance = nullptr;

// Application class to manage global state
class VioletApplication {
public:
    VioletApplication() = default;
    ~VioletApplication() = default;

    bool Initialize(HINSTANCE hInstance) {
        g_hInstance = hInstance;
        
        // Initialize common controls
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_WIN95_CLASSES;
        if (!InitCommonControlsEx(&icex)) {
            MessageBox(nullptr, L"Failed to initialize common controls", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Initialize configuration manager
        config_manager_ = std::make_unique<violet::ConfigManager>();
        if (!config_manager_->Initialize()) {
            MessageBox(nullptr, L"Failed to initialize configuration", L"Warning", MB_OK | MB_ICONWARNING);
            // Continue anyway with default settings
        }

        // Create main window
        main_window_ = std::make_unique<violet::MainWindow>();
        if (!main_window_->Create(hInstance)) {
            MessageBox(nullptr, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }

        return true;
    }

    int Run() {
        if (!main_window_) {
            return -1;
        }

        main_window_->Show();

        // Main message loop
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return static_cast<int>(msg.wParam);
    }

    void Shutdown() {
        if (config_manager_) {
            config_manager_->Save();
        }
    }

private:
    std::unique_ptr<violet::MainWindow> main_window_;
    std::unique_ptr<violet::ConfigManager> config_manager_;
};

// Windows entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Enable DPI awareness for modern Windows
    SetProcessDPIAware();

    VioletApplication app;
    
    if (!app.Initialize(hInstance)) {
        return -1;
    }

    int result = app.Run();
    
    app.Shutdown();
    
    return result;
}

// Console entry point for debugging builds
#ifdef _CONSOLE
int main(int argc, char* argv[]) {
    std::cout << "Violet LV2 Plugin Host (Debug Console Mode)" << std::endl;
    std::cout << "Starting GUI application..." << std::endl;
    
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    return WinMain(hInstance, nullptr, GetCommandLineA(), SW_SHOWDEFAULT);
}
#endif