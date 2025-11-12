#include "violet/plugin_parameters_window.h"
#include "violet/audio_processing_chain.h"
#include "violet/plugin_manager.h"
#include "violet/utils.h"
#include <windowsx.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace violet {

const wchar_t* PluginParametersWindow::CLASS_NAME = L"VioletPluginParametersWindow";

PluginParametersWindow::PluginParametersWindow()
    : hwnd_(nullptr)
    , scrollBar_(nullptr)
    , pluginNameStatic_(nullptr)
    , hInstance_(nullptr)
    , parent_(nullptr)
    , processingChain_(nullptr)
    , nodeId_(0)
    , scrollPos_(0)
    , maxScrollPos_(0)
    , userIsInteracting_(false) {
}

PluginParametersWindow::~PluginParametersWindow() {
    Close();
}

bool PluginParametersWindow::Create(HINSTANCE hInstance, HWND parent) {
    hInstance_ = hInstance;
    parent_ = parent;
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    // Check if already registered
    WNDCLASSEX existingClass;
    if (!GetClassInfoEx(hInstance, CLASS_NAME, &existingClass)) {
        if (!RegisterClassEx(&wc)) {
            return false;
        }
    }
    
    // Create the window (initially hidden)
    hwnd_ = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        CLASS_NAME,
        L"Plugin Parameters",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_MIN_HEIGHT,
        parent,
        nullptr,
        hInstance,
        this
    );
    
    if (!hwnd_) {
        return false;
    }
    
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    return true;
}

void PluginParametersWindow::Show() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOW);
        UpdateWindow(hwnd_);
    }
}

void PluginParametersWindow::Hide() {
    if (hwnd_) {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void PluginParametersWindow::Close() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool PluginParametersWindow::IsVisible() const {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void PluginParametersWindow::SetPlugin(AudioProcessingChain* chain, uint32_t nodeId) {
    processingChain_ = chain;
    nodeId_ = nodeId;
    
    if (!processingChain_ || nodeId_ == 0) {
        Hide();
        return;
    }
    
    // Get plugin info and update window title
    ProcessingNode* node = processingChain_->GetNode(nodeId_);
    if (node) {
        PluginInstance* plugin = node->GetPlugin();
        if (plugin) {
            const PluginInfo& info = plugin->GetInfo();
            std::wstring title = L"Parameters - " + utils::StringToWString(info.name);
            SetWindowText(hwnd_, title.c_str());
            
            // Recreate controls
            CreateControls();
            
            // Start update timer
            SetTimer(hwnd_, TIMER_ID_UPDATE, UPDATE_INTERVAL_MS, nullptr);
        }
    }
}

void PluginParametersWindow::RefreshParameters() {
    if (!processingChain_ || nodeId_ == 0) return;
    
    ProcessingNode* node = processingChain_->GetNode(nodeId_);
    if (!node) return;
    
    PluginInstance* plugin = node->GetPlugin();
    if (!plugin) return;
    
    // Update all parameter displays
    for (const auto& control : controls_) {
        UpdateParameterValue(control.parameterIndex);
    }
}

void PluginParametersWindow::UpdateParameterValue(uint32_t parameterIndex) {
    if (!processingChain_ || nodeId_ == 0) return;
    
    ProcessingNode* node = processingChain_->GetNode(nodeId_);
    if (!node) return;
    
    PluginInstance* plugin = node->GetPlugin();
    if (!plugin) return;
    
    // Find the control
    for (auto& control : controls_) {
        if (control.parameterIndex == parameterIndex) {
            float value = plugin->GetParameter(parameterIndex);
            
            // Update slider position
            int sliderPos = ValueToSliderPosition(value, control.info);
            SendMessage(control.slider, TBM_SETPOS, TRUE, sliderPos);
            
            // Update value display
            std::wstring valueText = FormatParameterValue(control.info, value);
            SetWindowText(control.valueStatic, valueText.c_str());
            
            break;
        }
    }
}

LRESULT CALLBACK PluginParametersWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PluginParametersWindow* window = nullptr;
    
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<PluginParametersWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<PluginParametersWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (window) {
        return window->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT PluginParametersWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            OnCreate();
            return 0;
            
        case WM_DESTROY:
            OnDestroy();
            return 0;
            
        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;
            
        case WM_HSCROLL:
            OnHScroll(wParam, lParam);
            return 0;
            
        case WM_VSCROLL:
            OnVScroll(wParam, lParam);
            return 0;
            
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
            
        case WM_CLOSE:
            Hide();
            return 0;
    }
    
    return DefWindowProc(hwnd_, uMsg, wParam, lParam);
}

void PluginParametersWindow::OnCreate() {
    // Create plugin name label
    pluginNameStatic_ = CreateWindowEx(
        0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        MARGIN, MARGIN, WINDOW_WIDTH - 2 * MARGIN, LABEL_HEIGHT,
        hwnd_, nullptr, hInstance_, nullptr
    );
    
    // Set font
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(pluginNameStatic_, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void PluginParametersWindow::OnDestroy() {
    DestroyControls();
    KillTimer(hwnd_, TIMER_ID_UPDATE);
}

void PluginParametersWindow::OnSize(int width, int height) {
    // Update plugin name label
    if (pluginNameStatic_) {
        SetWindowPos(pluginNameStatic_, nullptr, MARGIN, MARGIN,
                    width - 2 * MARGIN, LABEL_HEIGHT, SWP_NOZORDER);
    }
    
    // Update scroll bar range
    if (!controls_.empty()) {
        int totalHeight = controls_.back().yPos + CONTROL_HEIGHT + MARGIN;
        int visibleHeight = height - 2 * MARGIN - LABEL_HEIGHT - MARGIN;
        
        if (totalHeight > visibleHeight) {
            maxScrollPos_ = totalHeight - visibleHeight;
            
            SCROLLINFO si = {};
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = totalHeight;
            si.nPage = visibleHeight;
            si.nPos = scrollPos_;
            SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
        } else {
            maxScrollPos_ = 0;
            scrollPos_ = 0;
            ShowScrollBar(hwnd_, SB_VERT, FALSE);
        }
    }
}

void PluginParametersWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
    HWND control = reinterpret_cast<HWND>(lParam);
    
    if (HIWORD(wParam) == BN_CLICKED) {
        // Check if it's a reset button
        auto it = buttonToIndex_.find(control);
        if (it != buttonToIndex_.end()) {
            OnResetButton(it->second);
        }
    }
}

void PluginParametersWindow::OnHScroll(WPARAM wParam, LPARAM lParam) {
    HWND slider = reinterpret_cast<HWND>(lParam);
    if (slider) {
        OnSliderChange(slider);
    }
}

void PluginParametersWindow::OnVScroll(WPARAM wParam, LPARAM lParam) {
    int action = LOWORD(wParam);
    
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd_, SB_VERT, &si);
    
    int oldPos = scrollPos_;
    
    switch (action) {
        case SB_LINEUP:
            scrollPos_ = std::max(0, scrollPos_ - CONTROL_HEIGHT);
            break;
        case SB_LINEDOWN:
            scrollPos_ = std::min(maxScrollPos_, scrollPos_ + CONTROL_HEIGHT);
            break;
        case SB_PAGEUP:
            scrollPos_ = std::max(0, scrollPos_ - (int)si.nPage);
            break;
        case SB_PAGEDOWN:
            scrollPos_ = std::min(maxScrollPos_, scrollPos_ + (int)si.nPage);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            scrollPos_ = HIWORD(wParam);
            break;
    }
    
    scrollPos_ = std::max(0, std::min(maxScrollPos_, scrollPos_));
    
    if (scrollPos_ != oldPos) {
        si.fMask = SIF_POS;
        si.nPos = scrollPos_;
        SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
        
        // Move all controls
        int offset = oldPos - scrollPos_;
        for (const auto& control : controls_) {
            if (control.labelStatic) {
                RECT rect;
                GetWindowRect(control.labelStatic, &rect);
                ScreenToClient(hwnd_, (POINT*)&rect);
                SetWindowPos(control.labelStatic, nullptr,
                           rect.left, rect.top + offset, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            if (control.valueStatic) {
                RECT rect;
                GetWindowRect(control.valueStatic, &rect);
                ScreenToClient(hwnd_, (POINT*)&rect);
                SetWindowPos(control.valueStatic, nullptr,
                           rect.left, rect.top + offset, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            if (control.slider) {
                RECT rect;
                GetWindowRect(control.slider, &rect);
                ScreenToClient(hwnd_, (POINT*)&rect);
                SetWindowPos(control.slider, nullptr,
                           rect.left, rect.top + offset, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            if (control.resetButton) {
                RECT rect;
                GetWindowRect(control.resetButton, &rect);
                ScreenToClient(hwnd_, (POINT*)&rect);
                SetWindowPos(control.resetButton, nullptr,
                           rect.left, rect.top + offset, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }
}

void PluginParametersWindow::OnTimer(WPARAM timerId) {
    if (timerId == TIMER_ID_UPDATE && !userIsInteracting_) {
        RefreshParameters();
    } else if (timerId == 2) {
        // Reset interaction flag
        userIsInteracting_ = false;
        KillTimer(hwnd_, 2);
    }
}

void PluginParametersWindow::CreateControls() {
    DestroyControls();
    
    if (!processingChain_ || nodeId_ == 0) return;
    
    ProcessingNode* node = processingChain_->GetNode(nodeId_);
    if (!node) return;
    
    PluginInstance* plugin = node->GetPlugin();
    if (!plugin) return;
    
    // Get parameters
    std::vector<ParameterInfo> params = plugin->GetParameters();
    
    int yPos = MARGIN + LABEL_HEIGHT + MARGIN;
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    
    for (const auto& param : params) {
        CreateParameterControl(param, yPos);
    }
    
    // Update window size and scroll bar
    int totalHeight = yPos + MARGIN;
    int desiredHeight = std::min(WINDOW_MAX_HEIGHT, std::max(WINDOW_MIN_HEIGHT, totalHeight));
    
    RECT rect;
    GetWindowRect(hwnd_, &rect);
    SetWindowPos(hwnd_, nullptr, 0, 0, WINDOW_WIDTH, desiredHeight,
                SWP_NOMOVE | SWP_NOZORDER);
    
    OnSize(WINDOW_WIDTH, desiredHeight);
}

void PluginParametersWindow::DestroyControls() {
    for (auto& control : controls_) {
        if (control.labelStatic) DestroyWindow(control.labelStatic);
        if (control.valueStatic) DestroyWindow(control.valueStatic);
        if (control.slider) DestroyWindow(control.slider);
        if (control.resetButton) DestroyWindow(control.resetButton);
    }
    
    controls_.clear();
    sliderToIndex_.clear();
    buttonToIndex_.clear();
}

void PluginParametersWindow::CreateParameterControl(const ParameterInfo& param, int& yPos) {
    ParameterControl control;
    control.parameterIndex = param.index;
    control.info = param;
    control.yPos = yPos - scrollPos_;
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    
    // Parameter name label
    control.labelStatic = CreateWindowEx(
        0, L"STATIC",
        utils::StringToWString(param.name).c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        MARGIN, control.yPos,
        SLIDER_WIDTH, LABEL_HEIGHT,
        hwnd_, nullptr, hInstance_, nullptr
    );
    SendMessage(control.labelStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Value display
    float currentValue = 0.0f;
    if (processingChain_) {
        ProcessingNode* node = processingChain_->GetNode(nodeId_);
        if (node) {
            PluginInstance* plugin = node->GetPlugin();
            if (plugin) {
                currentValue = plugin->GetParameter(param.index);
            }
        }
    }
    
    std::wstring valueText = FormatParameterValue(param, currentValue);
    control.valueStatic = CreateWindowEx(
        0, L"STATIC",
        valueText.c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        MARGIN + SLIDER_WIDTH + CONTROL_PADDING,
        control.yPos,
        VALUE_WIDTH, LABEL_HEIGHT,
        hwnd_, nullptr, hInstance_, nullptr
    );
    SendMessage(control.valueStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Slider control
    control.slider = CreateWindowEx(
        0, TRACKBAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        MARGIN, control.yPos + LABEL_HEIGHT + 2,
        SLIDER_WIDTH, SLIDER_HEIGHT,
        hwnd_, nullptr, hInstance_, nullptr
    );
    
    SendMessage(control.slider, TBM_SETRANGE, TRUE, MAKELONG(0, SLIDER_RESOLUTION));
    int sliderPos = ValueToSliderPosition(currentValue, param);
    SendMessage(control.slider, TBM_SETPOS, TRUE, sliderPos);
    SendMessage(control.slider, TBM_SETTICFREQ, SLIDER_RESOLUTION / 10, 0);
    
    sliderToIndex_[control.slider] = param.index;
    
    // Reset button
    control.resetButton = CreateWindowEx(
        0, L"BUTTON", L"Reset",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN + SLIDER_WIDTH + CONTROL_PADDING + VALUE_WIDTH + CONTROL_PADDING,
        control.yPos + LABEL_HEIGHT,
        RESET_WIDTH, SLIDER_HEIGHT,
        hwnd_, nullptr, hInstance_, nullptr
    );
    SendMessage(control.resetButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    buttonToIndex_[control.resetButton] = param.index;
    
    controls_.push_back(control);
    
    yPos += CONTROL_HEIGHT;
}

void PluginParametersWindow::OnSliderChange(HWND slider) {
    auto it = sliderToIndex_.find(slider);
    if (it == sliderToIndex_.end()) return;
    
    uint32_t paramIndex = it->second;
    
    // Set interaction flag to prevent timer interference
    userIsInteracting_ = true;
    
    // Find the control
    for (const auto& control : controls_) {
        if (control.parameterIndex == paramIndex) {
            // Get slider position
            int sliderPos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
            float value = SliderPositionToValue(sliderPos, control.info);
            
            // Update plugin parameter
            if (processingChain_) {
                processingChain_->SetParameter(nodeId_, paramIndex, value);
            }
            
            // Update value display
            UpdateValueDisplay(paramIndex);
            
            // Reset interaction flag after a delay
            SetTimer(hwnd_, 2, 150, nullptr); // Reset after 150ms
            
            break;
        }
    }
}

void PluginParametersWindow::OnResetButton(uint32_t parameterIndex) {
    // Find the control
    for (const auto& control : controls_) {
        if (control.parameterIndex == parameterIndex) {
            float defaultValue = control.info.defaultValue;
            
            // Update plugin parameter
            if (processingChain_) {
                processingChain_->SetParameter(nodeId_, parameterIndex, defaultValue);
            }
            
            // Update slider and value display
            UpdateSliderPosition(parameterIndex);
            UpdateValueDisplay(parameterIndex);
            
            break;
        }
    }
}

void PluginParametersWindow::UpdateValueDisplay(uint32_t parameterIndex) {
    if (!processingChain_ || nodeId_ == 0) return;
    
    ProcessingNode* node = processingChain_->GetNode(nodeId_);
    if (!node) return;
    
    PluginInstance* plugin = node->GetPlugin();
    if (!plugin) return;
    
    for (const auto& control : controls_) {
        if (control.parameterIndex == parameterIndex) {
            float value = plugin->GetParameter(parameterIndex);
            std::wstring valueText = FormatParameterValue(control.info, value);
            SetWindowText(control.valueStatic, valueText.c_str());
            break;
        }
    }
}

void PluginParametersWindow::UpdateSliderPosition(uint32_t parameterIndex) {
    if (!processingChain_ || nodeId_ == 0) return;
    
    ProcessingNode* node = processingChain_->GetNode(nodeId_);
    if (!node) return;
    
    PluginInstance* plugin = node->GetPlugin();
    if (!plugin) return;
    
    for (const auto& control : controls_) {
        if (control.parameterIndex == parameterIndex) {
            float value = plugin->GetParameter(parameterIndex);
            int sliderPos = ValueToSliderPosition(value, control.info);
            SendMessage(control.slider, TBM_SETPOS, TRUE, sliderPos);
            break;
        }
    }
}

std::wstring PluginParametersWindow::FormatParameterValue(const ParameterInfo& param, float value) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(param.isInteger ? 0 : 2) << value;
    return ss.str();
}

float PluginParametersWindow::SliderPositionToValue(int sliderPos, const ParameterInfo& param) {
    float normalized = (float)sliderPos / (float)SLIDER_RESOLUTION;
    float value = param.minimum + normalized * (param.maximum - param.minimum);
    
    if (param.isInteger) {
        value = std::round(value);
    }
    
    return std::max(param.minimum, std::min(param.maximum, value));
}

int PluginParametersWindow::ValueToSliderPosition(float value, const ParameterInfo& param) {
    float range = param.maximum - param.minimum;
    if (range <= 0.0f) return 0;
    
    float normalized = (value - param.minimum) / range;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    return (int)(normalized * SLIDER_RESOLUTION);
}

} // namespace violet
