#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include "violet/plugin_manager.h"

namespace violet {

class AudioProcessingChain;
class ProcessingNode;
class PluginInstance;

// Individual parameter control
struct ParameterControl {
    uint32_t parameterIndex;
    HWND labelStatic;      // Static text showing parameter name
    HWND valueStatic;      // Static text showing current value
    HWND slider;           // Slider control
    HWND resetButton;      // Reset to default button
    ParameterInfo info;
    int yPos;              // Y position in window
    
    ParameterControl()
        : parameterIndex(0)
        , labelStatic(nullptr)
        , valueStatic(nullptr)
        , slider(nullptr)
        , resetButton(nullptr)
        , yPos(0) {}
};

// Plugin Parameters Window - displays and controls plugin parameters
class PluginParametersWindow {
public:
    PluginParametersWindow();
    ~PluginParametersWindow();
    
    // Window management
    bool Create(HINSTANCE hInstance, HWND parent);
    void Show();
    void Hide();
    void Close();
    bool IsVisible() const;
    
    // Set plugin to display/control
    void SetPlugin(AudioProcessingChain* chain, uint32_t nodeId);
    
    // Update display
    void RefreshParameters();
    void UpdateParameterValue(uint32_t parameterIndex);
    
    // Get window handle
    HWND GetHandle() const { return hwnd_; }

private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnCreate();
    void OnDestroy();
    void OnSize(int width, int height);
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnHScroll(WPARAM wParam, LPARAM lParam);
    void OnVScroll(WPARAM wParam, LPARAM lParam);
    void OnTimer(WPARAM timerId);
    
    // Control creation
    void CreateControls();
    void DestroyControls();
    void CreateParameterControl(const ParameterInfo& param, int& yPos);
    
    // Parameter updates
    void OnSliderChange(HWND slider);
    void OnResetButton(uint32_t parameterIndex);
    void UpdateValueDisplay(uint32_t parameterIndex);
    void UpdateSliderPosition(uint32_t parameterIndex);
    
    // Utility
    std::wstring FormatParameterValue(const ParameterInfo& param, float value);
    float SliderPositionToValue(int sliderPos, const ParameterInfo& param);
    int ValueToSliderPosition(float value, const ParameterInfo& param);
    
    // Member variables
    HWND hwnd_;
    HWND scrollBar_;
    HWND pluginNameStatic_;
    HINSTANCE hInstance_;
    HWND parent_;
    
    AudioProcessingChain* processingChain_;
    uint32_t nodeId_;
    
    std::vector<ParameterControl> controls_;
    std::map<HWND, uint32_t> sliderToIndex_;
    std::map<HWND, uint32_t> buttonToIndex_;
    
    // Layout properties
    static const int WINDOW_WIDTH = 400;
    static const int WINDOW_MIN_HEIGHT = 200;
    static const int WINDOW_MAX_HEIGHT = 600;
    static const int CONTROL_HEIGHT = 60;
    static const int CONTROL_PADDING = 10;
    static const int SLIDER_WIDTH = 250;
    static const int VALUE_WIDTH = 60;
    static const int RESET_WIDTH = 50;
    static const int LABEL_HEIGHT = 16;
    static const int SLIDER_HEIGHT = 24;
    static const int MARGIN = 10;
    static const int SLIDER_RESOLUTION = 1000; // Slider precision
    
    // Scroll position
    int scrollPos_;
    int maxScrollPos_;
    
    // Update timer
    static const int TIMER_ID_UPDATE = 1;
    static const int UPDATE_INTERVAL_MS = 100;
    
    // User interaction state
    bool userIsInteracting_;
    
    // Window class
    static const wchar_t* CLASS_NAME;
};

} // namespace violet
