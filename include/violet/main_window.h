#pragma once

#include <windows.h>
#include <string>
#include <memory>
#include <cstdint>

namespace violet {

// Forward declarations
class PluginBrowser;
class ActivePluginsPanel;
class PluginParametersWindow;
class PluginManager;
class AudioEngine;
class AudioProcessingChain;
class SessionManager;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    // Create the main window
    bool Create(HINSTANCE hInstance);
    
    // Show/hide the window
    void Show(int nCmdShow = SW_SHOW);
    void Hide();
    
    // Get window handle
    HWND GetHandle() const { return hwnd_; }
    
    // Window dimensions
    void SetSize(int width, int height);
    void GetSize(int& width, int& height) const;
    
private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnCreate();
    void OnDestroy();
    void OnPaint();
    void OnSize(int width, int height);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    
    // UI creation
    void CreateControls();
    void CreateMenuBar();
    void CreateStatusBar();
    void CreateToolBar();
    
    // Layout management
    void UpdateLayout();
    
    // Plugin management
    void LoadPlugin(const std::string& pluginUri);
    void ShowPluginParameters(uint32_t nodeId);
    
    // Session management
    void OnNewSession();
    void OnOpenSession();
    void OnSaveSession();
    void OnSaveSessionAs();
    
    // Member variables
    HWND hwnd_;
    HWND hStatusBar_;
    HWND hToolBar_;
    HINSTANCE hInstance_;
    
    // Child windows
    std::unique_ptr<PluginBrowser> pluginBrowser_;
    std::unique_ptr<ActivePluginsPanel> activePluginsPanel_;
    std::unique_ptr<PluginParametersWindow> parametersWindow_;
    
    // Backend components (to be initialized)
    std::unique_ptr<PluginManager> pluginManager_;
    std::unique_ptr<AudioEngine> audioEngine_;
    std::unique_ptr<AudioProcessingChain> processingChain_;
    std::unique_ptr<SessionManager> sessionManager_;
    
    // Window properties
    static const wchar_t* CLASS_NAME;
    static const int DEFAULT_WIDTH = 1000;
    static const int DEFAULT_HEIGHT = 700;
    static const int MIN_WIDTH = 800;
    static const int MIN_HEIGHT = 600;
    static const int PLUGIN_BROWSER_WIDTH = 250;
};

} // namespace violet