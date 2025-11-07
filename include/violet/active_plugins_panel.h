#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace violet {

class AudioProcessingChain;
class PluginInstance;

// Represents a plugin in the active chain
struct ActivePluginInfo {
    uint32_t nodeId;
    std::string name;
    std::string uri;
    bool bypassed;
    bool active;
    int xPos;
    int yPos;
    int width;
    int height;
};

// Active Plugins Panel - displays the plugin processing chain
class ActivePluginsPanel {
public:
    ActivePluginsPanel();
    ~ActivePluginsPanel();

    // Create the panel window
    bool Create(HWND parent, HINSTANCE hInstance, int x, int y, int width, int height);
    
    // Set the audio processing chain to display
    void SetProcessingChain(AudioProcessingChain* chain);
    
    // Get window handle
    HWND GetHandle() const { return hwnd_; }
    
    // Add a plugin to display
    void AddPlugin(uint32_t nodeId, const std::string& name, const std::string& uri);
    
    // Remove a plugin from display
    void RemovePlugin(uint32_t nodeId);
    
    // Clear all plugins
    void ClearPlugins();
    
    // Refresh the display
    void Refresh();
    
    // Resize the panel
    void Resize(int x, int y, int width, int height);
    
    // Get selected plugin node ID (0 if none)
    uint32_t GetSelectedPlugin() const { return selectedNodeId_; }

private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnCreate();
    void OnPaint();
    void OnSize(int width, int height);
    void OnLButtonDown(int x, int y);
    void OnLButtonDblClk(int x, int y);
    void OnRButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    
    // Drawing functions
    void DrawPlugin(HDC hdc, const ActivePluginInfo& plugin);
    void DrawConnection(HDC hdc, const ActivePluginInfo& from, const ActivePluginInfo& to);
    void DrawEmptyState(HDC hdc);
    
    // Hit testing
    int HitTest(int x, int y) const;
    
    // Layout management
    void RecalculateLayout();
    
    // Context menu
    void ShowContextMenu(int x, int y);
    
    // Member variables
    HWND hwnd_;
    HINSTANCE hInstance_;
    
    AudioProcessingChain* processingChain_;
    std::vector<ActivePluginInfo> plugins_;
    
    uint32_t selectedNodeId_;
    int hoveredPluginIndex_;
    
    // Layout properties
    static const int PLUGIN_WIDTH = 120;
    static const int PLUGIN_HEIGHT = 80;
    static const int PLUGIN_SPACING = 20;
    static const int MARGIN = 20;
    
    // Window class
    static const wchar_t* CLASS_NAME;
    
    // Menu IDs
    static const int ID_MENU_REMOVE = 2001;
    static const int ID_MENU_BYPASS = 2002;
    static const int ID_MENU_EDIT = 2003;
    static const int ID_MENU_MOVE_UP = 2004;
    static const int ID_MENU_MOVE_DOWN = 2005;
};

} // namespace violet
