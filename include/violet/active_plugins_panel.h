#pragma once

#include <windows.h>
#include <commctrl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "violet/plugin_manager.h"

namespace violet {

class AudioProcessingChain;
class PluginInstance;

// Parameter control for inline display
struct InlineParameterControl {
    uint32_t parameterIndex;
    ParameterInfo info;
    HWND labelStatic;
    HWND valueStatic;
    HWND slider;
    int yOffset;  // Offset from plugin top
};

// Represents a plugin in the active chain
struct ActivePluginInfo {
    uint32_t nodeId;
    std::string name;
    std::string uri;
    bool bypassed;
    bool active;
    bool expanded;  // Show parameters
    int yPos;
    int height;
    HWND bypassButton;
    HWND removeButton;
    std::vector<InlineParameterControl> parameters;
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
    
    // Load plugin from drag-drop
    void LoadPluginFromUri(const std::string& uri);

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
    void OnHScroll(WPARAM wParam, LPARAM lParam);
    void OnVScroll(WPARAM wParam, LPARAM lParam);
    void OnTimer(WPARAM timerId);
    void OnDropFiles(HDROP hDrop);
    
    // Drawing functions
    void DrawPlugin(HDC hdc, const ActivePluginInfo& plugin);
    void DrawEmptyState(HDC hdc);
    
    // Parameter control functions
    void CreateParameterControls(ActivePluginInfo& plugin);
    void DestroyParameterControls(ActivePluginInfo& plugin);
    void UpdateParameterControls(ActivePluginInfo& plugin);
    void CreateHeaderButtons(ActivePluginInfo& plugin);
    void DestroyHeaderButtons(ActivePluginInfo& plugin);
    void TogglePluginExpanded(size_t pluginIndex);
    void OnSliderChange(HWND slider);
    void UpdateParameterValue(uint32_t nodeId, uint32_t paramIndex);
    float SliderPosToValue(int pos, const ParameterInfo& info);
    int ValueToSliderPos(float value, const ParameterInfo& info);
    
    // Hit testing
    int HitTest(int x, int y) const;
    
    // Layout management
    void RecalculateLayout();
    
    // Context menu
    void ShowContextMenu(int x, int y);
    
    // Member variables
    HWND hwnd_;
    HINSTANCE hInstance_;
    HWND removeAllButton_;
    
    AudioProcessingChain* processingChain_;
    std::vector<ActivePluginInfo> plugins_;
    
    uint32_t selectedNodeId_;
    int hoveredPluginIndex_;
    
    // Slider tracking
    std::map<HWND, std::pair<uint32_t, uint32_t>> sliderToParam_;  // slider -> (nodeId, paramIndex)
    std::map<HWND, uint32_t> buttonToNode_;  // button -> nodeId
    
    // Scroll position
    int scrollPos_;
    int maxScrollPos_;
    
    // Layout properties
    static const int PLUGIN_HEADER_HEIGHT = 45;
    static const int BUTTON_WIDTH = 70;
    static const int BUTTON_HEIGHT = 24;
    static const int PARAM_HEIGHT = 50;
    static const int PLUGIN_SPACING = 5;
    static const int MARGIN = 10;
    static const int SLIDER_WIDTH = 200;
    static const int VALUE_WIDTH = 60;
    static const int LABEL_WIDTH = 150;
    static const int SLIDER_RESOLUTION = 1000;
    
    // Update timer
    static const int TIMER_ID_UPDATE = 2;
    static const int UPDATE_INTERVAL_MS = 100;
    
    // Window class
    static const wchar_t* CLASS_NAME;
    
    // Menu IDs
    static const int ID_MENU_REMOVE = 2001;
    static const int ID_MENU_BYPASS = 2002;
    static const int ID_MENU_EDIT = 2003;
    static const int ID_MENU_MOVE_UP = 2004;
    static const int ID_MENU_MOVE_DOWN = 2005;
    
    // Button IDs
    static const int ID_BUTTON_REMOVE_ALL = 3001;
    static const int ID_BUTTON_BYPASS_BASE = 4000;  // + nodeId
    static const int ID_BUTTON_REMOVE_BASE = 5000;  // + nodeId
};

} // namespace violet
