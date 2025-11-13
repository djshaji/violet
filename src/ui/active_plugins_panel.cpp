#include "violet/active_plugins_panel.h"
#include "violet/audio_processing_chain.h"
#include "violet/plugin_manager.h"
#include "violet/utils.h"
#include "violet/modern_controls.h"
#include "violet/knob_control.h"
#include <windowsx.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace violet {

const wchar_t* ActivePluginsPanel::CLASS_NAME = L"VioletActivePluginsPanel";

ActivePluginsPanel::ActivePluginsPanel()
    : hwnd_(nullptr)
    , hInstance_(nullptr)
    , removeAllButton_(nullptr)
    , processingChain_(nullptr)
    , selectedNodeId_(0)
    , hoveredPluginIndex_(-1)
    , scrollPos_(0)
    , maxScrollPos_(0)
    , userIsInteracting_(false)
    , activeSlider_(nullptr) {
}

ActivePluginsPanel::~ActivePluginsPanel() {
    // Destroy all parameter controls
    for (auto& plugin : plugins_) {
        DestroyParameterControls(plugin);
    }
    
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool ActivePluginsPanel::Create(HWND parent, HINSTANCE hInstance, int x, int y, int width, int height) {
    hInstance_ = hInstance;
    
    // Register knob control class
    KnobControl::RegisterClass(hInstance);
    
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    
    // Check if already registered
    WNDCLASSEX existingClass;
    if (!GetClassInfoEx(hInstance, CLASS_NAME, &existingClass)) {
        if (!RegisterClassEx(&wc)) {
            return false;
        }
    }
    
    // Create the window with scrollbar
    hwnd_ = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Active Plugins",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
        x, y, width, height,
        parent,
        nullptr,
        hInstance,
        this
    );
    
    if (!hwnd_) {
        return false;
    }
    
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    // Start update timer
    SetTimer(hwnd_, TIMER_ID_UPDATE, UPDATE_INTERVAL_MS, nullptr);
    
    return true;
}

void ActivePluginsPanel::SetProcessingChain(AudioProcessingChain* chain) {
    processingChain_ = chain;
}

void ActivePluginsPanel::AddPlugin(uint32_t nodeId, const std::string& name, const std::string& uri) {
    ActivePluginInfo plugin;
    plugin.nodeId = nodeId;
    plugin.name = name;
    plugin.uri = uri;
    plugin.bypassed = false;
    plugin.active = true;
    plugin.expanded = true;  // Auto-expand to show controls
    plugin.yPos = 0;
    plugin.height = PLUGIN_HEADER_HEIGHT;
    plugin.bypassButton = nullptr;
    plugin.removeButton = nullptr;
    
    plugins_.push_back(plugin);
    
    // Create header buttons
    CreateHeaderButtons(plugins_.back());
    
    // Create parameter controls immediately
    CreateParameterControls(plugins_.back());
    
    RecalculateLayout();
    
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::RemovePlugin(uint32_t nodeId) {
    auto it = std::find_if(plugins_.begin(), plugins_.end(),
                          [nodeId](const ActivePluginInfo& p) {
                              return p.nodeId == nodeId;
                          });
    
    if (it != plugins_.end()) {
        DestroyHeaderButtons(*it);
        DestroyParameterControls(*it);
        plugins_.erase(it);
        RecalculateLayout();
        
        if (selectedNodeId_ == nodeId) {
            selectedNodeId_ = 0;
        }
        
        if (hwnd_) {
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
    }
}

void ActivePluginsPanel::ClearPlugins() {
    for (auto& plugin : plugins_) {
        DestroyHeaderButtons(plugin);
        DestroyParameterControls(plugin);
    }
    plugins_.clear();
    sliderToParam_.clear();
    buttonToNode_.clear();
    selectedNodeId_ = 0;
    hoveredPluginIndex_ = -1;
    
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::Refresh() {
    if (processingChain_) {
        auto nodeIds = processingChain_->GetNodeIds();
        
        // Remove plugins that no longer exist
        for (auto it = plugins_.begin(); it != plugins_.end(); ) {
            if (std::find(nodeIds.begin(), nodeIds.end(), it->nodeId) == nodeIds.end()) {
                DestroyParameterControls(*it);
                it = plugins_.erase(it);
            } else {
                ++it;
            }
        }
        
        RecalculateLayout();
    }
    
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::LoadPluginFromUri(const std::string& uri) {
    if (!processingChain_ || uri.empty()) {
        return;
    }
    
    // Load the plugin via the processing chain
    uint32_t nodeId = processingChain_->AddPlugin(uri);
    if (nodeId != 0) {
        // Get plugin info from the node
        auto node = processingChain_->GetNode(nodeId);
        if (node && node->GetPlugin()) {
            std::string name = node->GetPlugin()->GetInfo().name;
            AddPlugin(nodeId, name, uri);
        }
    }
}

void ActivePluginsPanel::Resize(int x, int y, int width, int height) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER);
        RecalculateLayout();
    }
}

void ActivePluginsPanel::CreateHeaderButtons(ActivePluginInfo& plugin) {
    if (!hwnd_) return;
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    
    int y = plugin.yPos - scrollPos_;
    int buttonY = y + (PLUGIN_HEADER_HEIGHT - BUTTON_HEIGHT) / 2;
    
    // Bypass button (right side, before remove button)
    plugin.bypassButton = CreateWindowEx(
        0, L"BUTTON",
        plugin.bypassed ? L"Enable" : L"Bypass",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        clientWidth - MARGIN - (BUTTON_WIDTH * 2 + 10), buttonY,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd_, (HMENU)(UINT_PTR)(ID_BUTTON_BYPASS_BASE + plugin.nodeId),
        hInstance_, nullptr
    );
    SendMessage(plugin.bypassButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    buttonToNode_[plugin.bypassButton] = plugin.nodeId;
    
    // Remove button (rightmost)
    plugin.removeButton = CreateWindowEx(
        0, L"BUTTON", L"Remove",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        clientWidth - MARGIN - BUTTON_WIDTH, buttonY,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd_, (HMENU)(UINT_PTR)(ID_BUTTON_REMOVE_BASE + plugin.nodeId),
        hInstance_, nullptr
    );
    SendMessage(plugin.removeButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    buttonToNode_[plugin.removeButton] = plugin.nodeId;
}

void ActivePluginsPanel::DestroyHeaderButtons(ActivePluginInfo& plugin) {
    if (plugin.bypassButton) {
        buttonToNode_.erase(plugin.bypassButton);
        DestroyWindow(plugin.bypassButton);
        plugin.bypassButton = nullptr;
    }
    if (plugin.removeButton) {
        buttonToNode_.erase(plugin.removeButton);
        DestroyWindow(plugin.removeButton);
        plugin.removeButton = nullptr;
    }
}

void ActivePluginsPanel::CreateParameterControls(ActivePluginInfo& plugin) {
    if (!processingChain_ || !hwnd_) return;
    
    ProcessingNode* node = processingChain_->GetNode(plugin.nodeId);
    if (!node) return;
    
    PluginInstance* instance = node->GetPlugin();
    if (!instance) return;
    
    std::vector<ParameterInfo> params = instance->GetParameters();
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    int yOffset = PLUGIN_HEADER_HEIGHT;
    
    for (const auto& param : params) {
        InlineParameterControl control;
        control.parameterIndex = param.index;
        control.info = param;
        control.yOffset = yOffset;
        
        int absoluteY = plugin.yPos + yOffset - scrollPos_;
        
        // Parameter label
        control.labelStatic = CreateWindowEx(
            0, L"STATIC",
            utils::StringToWString(param.name).c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            MARGIN, absoluteY,
            LABEL_WIDTH, 20,
            hwnd_, nullptr, hInstance_, nullptr
        );
        SendMessage(control.labelStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Value display
        float currentValue = instance->GetParameter(param.index);
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(param.isInteger ? 0 : 2) << currentValue;
        
        control.valueStatic = CreateWindowEx(
            0, L"STATIC",
            ss.str().c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            MARGIN + LABEL_WIDTH + 10, absoluteY,
            VALUE_WIDTH, 20,
            hwnd_, nullptr, hInstance_, nullptr
        );
        SendMessage(control.valueStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Minus button
        control.minusButton = CreateWindowEx(
            0, L"BUTTON", L"-",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + LABEL_WIDTH + VALUE_WIDTH + 20, absoluteY,
            20, 20,
            hwnd_, nullptr, hInstance_, nullptr
        );
        SendMessage(control.minusButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Knob control
        control.knob = new KnobControl();
        int knobSize = 50;
        control.knob->Create(hwnd_, hInstance_, 
            MARGIN + LABEL_WIDTH + VALUE_WIDTH + 45, absoluteY - 15,
            knobSize, 1000 + (int)plugin.parameters.size());
        control.knob->SetRange(param.minimum, param.maximum);
        control.knob->SetValue(currentValue);
        
        // Plus button
        control.plusButton = CreateWindowEx(
            0, L"BUTTON", L"+",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + LABEL_WIDTH + VALUE_WIDTH + 45 + knobSize + 5, absoluteY,
            20, 20,
            hwnd_, nullptr, hInstance_, nullptr
        );
        SendMessage(control.plusButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        sliderToParam_[control.knob->GetHandle()] = std::make_pair(plugin.nodeId, param.index);
        sliderToParam_[control.minusButton] = std::make_pair(plugin.nodeId, param.index);
        sliderToParam_[control.plusButton] = std::make_pair(plugin.nodeId, param.index);
        
        plugin.parameters.push_back(control);
        yOffset += PARAM_HEIGHT;
    }
    
    plugin.height = yOffset;
}

void ActivePluginsPanel::DestroyParameterControls(ActivePluginInfo& plugin) {
    for (auto& control : plugin.parameters) {
        if (control.labelStatic) {
            if (control.knob) {
                sliderToParam_.erase(control.knob->GetHandle());
            }
            sliderToParam_.erase(control.minusButton);
            sliderToParam_.erase(control.plusButton);
            DestroyWindow(control.labelStatic);
        }
        if (control.valueStatic) DestroyWindow(control.valueStatic);
        if (control.minusButton) DestroyWindow(control.minusButton);
        if (control.knob) {
            delete control.knob;
            control.knob = nullptr;
        }
        if (control.plusButton) DestroyWindow(control.plusButton);
    }
    plugin.parameters.clear();
    plugin.height = PLUGIN_HEADER_HEIGHT;
}

void ActivePluginsPanel::UpdateParameterControls(ActivePluginInfo& plugin) {
    if (!processingChain_) return;
    
    ProcessingNode* node = processingChain_->GetNode(plugin.nodeId);
    if (!node) return;
    
    PluginInstance* instance = node->GetPlugin();
    if (!instance) return;
    
    for (auto& control : plugin.parameters) {
        float value = instance->GetParameter(control.parameterIndex);
        
        // Update value display
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(control.info.isInteger ? 0 : 2) << value;
        SetWindowText(control.valueStatic, ss.str().c_str());
        
        // Only update knob if:
        // 1. User is NOT currently interacting with any slider, OR
        // 2. This specific knob is NOT the one being interacted with
        bool shouldUpdate = !userIsInteracting_ || (control.knob && control.knob->GetHandle() != activeSlider_);
        
        if (shouldUpdate && control.knob) {
            // Get current knob value
            float currentValue = control.knob->GetValue();
            
            // Only update if the value has changed significantly
            // Use a larger threshold for 0.0-1.0 ranges to avoid precision issues
            float threshold = (control.info.maximum - control.info.minimum) / 100.0f;
            if (std::abs(value - currentValue) > threshold) {
                control.knob->SetValue(value);
            }
        }
    }
}

void ActivePluginsPanel::TogglePluginExpanded(size_t pluginIndex) {
    if (pluginIndex >= plugins_.size()) return;
    
    auto& plugin = plugins_[pluginIndex];
    plugin.expanded = !plugin.expanded;
    
    if (plugin.expanded) {
        CreateParameterControls(plugin);
    } else {
        DestroyParameterControls(plugin);
    }
    
    RecalculateLayout();
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void ActivePluginsPanel::OnSliderChange(HWND slider) {
    auto it = sliderToParam_.find(slider);
    if (it == sliderToParam_.end()) return;
    
    uint32_t nodeId = it->second.first;
    uint32_t paramIndex = it->second.second;
    
    // Find the plugin and parameter info
    for (auto& plugin : plugins_) {
        if (plugin.nodeId == nodeId) {
            for (auto& control : plugin.parameters) {
                if (control.parameterIndex == paramIndex) {
                    // Get current slider position
                    int sliderPos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
                    float value = SliderPosToValue(sliderPos, control.info);
                    
                    if (processingChain_) {
                        processingChain_->SetParameter(nodeId, paramIndex, value);
                    }
                    
                    // Update value display
                    std::wstringstream ss;
                    ss << std::fixed << std::setprecision(control.info.isInteger ? 0 : 2) << value;
                    SetWindowText(control.valueStatic, ss.str().c_str());
                    
                    return;
                }
            }
        }
    }
}

float ActivePluginsPanel::SliderPosToValue(int pos, const ParameterInfo& info) {
    float normalized = (float)pos / (float)SLIDER_RESOLUTION;
    float value = info.minimum + normalized * (info.maximum - info.minimum);
    
    if (info.isInteger) {
        value = std::round(value);
    }
    
    return std::max(info.minimum, std::min(info.maximum, value));
}

int ActivePluginsPanel::ValueToSliderPos(float value, const ParameterInfo& info) {
    float range = info.maximum - info.minimum;
    if (range <= 0.0f) return 0;
    
    float normalized = (value - info.minimum) / range;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    return (int)(normalized * SLIDER_RESOLUTION);
}

LRESULT CALLBACK ActivePluginsPanel::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ActivePluginsPanel* panel = nullptr;
    
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        panel = reinterpret_cast<ActivePluginsPanel*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
    } else {
        panel = reinterpret_cast<ActivePluginsPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (panel) {
        return panel->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT ActivePluginsPanel::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        OnCreate();
        return 0;
        
    case WM_PAINT:
        OnPaint();
        return 0;
        
    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
        
    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
        
    case WM_LBUTTONDBLCLK:
        OnLButtonDblClk(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
        
    case WM_RBUTTONDOWN:
        OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
        
    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
        
    case WM_NOTIFY:
        return OnNotify(wParam, lParam);
        
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void ActivePluginsPanel::OnCreate() {
    // Create "Remove All" button at the top
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    
    removeAllButton_ = CreateWindowEx(
        0, L"BUTTON", L"Remove All Plugins",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN, 5, 150, 25,
        hwnd_, (HMENU)(UINT_PTR)ID_BUTTON_REMOVE_ALL, hInstance_, nullptr
    );
    SendMessage(removeAllButton_, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void ActivePluginsPanel::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    // Fill background
    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
    
    if (plugins_.empty()) {
        DrawEmptyState(hdc);
    } else {
        for (const auto& plugin : plugins_) {
            DrawPlugin(hdc, plugin);
        }
    }
    
    EndPaint(hwnd_, &ps);
}

void ActivePluginsPanel::OnSize(int width, int height) {
    (void)width;
    (void)height;
    RecalculateLayout();
}

void ActivePluginsPanel::OnLButtonDown(int x, int y) {
    int index = HitTest(x, y);
    if (index >= 0) {
        selectedNodeId_ = plugins_[index].nodeId;
        InvalidateRect(hwnd_, nullptr, TRUE);
    } else {
        selectedNodeId_ = 0;
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::OnLButtonDblClk(int x, int y) {
    int index = HitTest(x, y);
    if (index >= 0) {
        // Toggle expansion
        TogglePluginExpanded(index);
    }
}

void ActivePluginsPanel::OnRButtonDown(int x, int y) {
    int index = HitTest(x, y);
    if (index >= 0) {
        selectedNodeId_ = plugins_[index].nodeId;
        InvalidateRect(hwnd_, nullptr, TRUE);
        
        POINT pt = { x, y };
        ClientToScreen(hwnd_, &pt);
        ShowContextMenu(pt.x, pt.y);
    }
}

void ActivePluginsPanel::OnMouseMove(int x, int y) {
    int newHovered = HitTest(x, y);
    if (newHovered != hoveredPluginIndex_) {
        hoveredPluginIndex_ = newHovered;
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::OnCommand(WPARAM wParam, LPARAM lParam) {
    int wmId = LOWORD(wParam);
    HWND hwndCtl = reinterpret_cast<HWND>(lParam);
    
    // Check for Remove All button
    if (wmId == ID_BUTTON_REMOVE_ALL) {
        if (processingChain_) {
            // Remove all plugins from chain
            auto nodeIds = processingChain_->GetNodeIds();
            for (auto nodeId : nodeIds) {
                processingChain_->RemovePlugin(nodeId);
            }
        }
        ClearPlugins();
        InvalidateRect(hwnd_, nullptr, TRUE);
        return;
    }
    
    // Check for parameter +/- buttons
    auto paramIt = sliderToParam_.find(hwndCtl);
    if (paramIt != sliderToParam_.end()) {
        uint32_t nodeId = paramIt->second.first;
        uint32_t paramIndex = paramIt->second.second;
        
        // Find the control to determine which button was clicked
        for (auto& plugin : plugins_) {
            if (plugin.nodeId == nodeId) {
                for (auto& control : plugin.parameters) {
                    if (control.parameterIndex == paramIndex) {
                        bool isMinus = (hwndCtl == control.minusButton);
                        bool isPlus = (hwndCtl == control.plusButton);
                        
                        if (isMinus || isPlus) {
                            // Set interaction tracking
                            userIsInteracting_ = true;
                            if (control.knob) {
                                activeSlider_ = control.knob->GetHandle();
                            }
                            
                            // Kill any existing timer
                            KillTimer(hwnd_, TIMER_ID_INTERACTION);
                            
                            // Get current value from knob
                            float currentValue = control.knob ? control.knob->GetValue() : control.info.defaultValue;
                            
                            // Calculate step size (1% of range, or 1 for integers)
                            float step;
                            if (control.info.isInteger) {
                                step = 1.0f;
                            } else {
                                step = (control.info.maximum - control.info.minimum) * 0.01f;
                            }
                            
                            // Adjust value
                            float newValue = currentValue + (isPlus ? step : -step);
                            newValue = std::max(control.info.minimum, std::min(control.info.maximum, newValue));
                            
                            // Update knob and parameter
                            if (control.knob) {
                                control.knob->SetValue(newValue);
                            }
                            
                            if (processingChain_) {
                                processingChain_->SetParameter(nodeId, paramIndex, newValue);
                            }
                            
                            // Update value display
                            std::wstringstream ss;
                            ss << std::fixed << std::setprecision(control.info.isInteger ? 0 : 2) << newValue;
                            SetWindowText(control.valueStatic, ss.str().c_str());
                            
                            // Start reset timer
                            SetTimer(hwnd_, TIMER_ID_INTERACTION, 500, nullptr);
                            
                            return;
                        }
                    }
                }
            }
        }
    }
    
    // Check for plugin-specific buttons
    auto btnIt = buttonToNode_.find(hwndCtl);
    if (btnIt != buttonToNode_.end()) {
        uint32_t nodeId = btnIt->second;
        
        // Determine if it's bypass or remove based on button ID
        if (wmId >= ID_BUTTON_BYPASS_BASE && wmId < ID_BUTTON_REMOVE_BASE) {
            // Bypass button clicked
            if (processingChain_) {
                ProcessingNode* node = processingChain_->GetNode(nodeId);
                if (node) {
                    bool bypassed = node->IsBypassed();
                    node->SetBypassed(!bypassed);
                    
                    // Update button text
                    for (auto& plugin : plugins_) {
                        if (plugin.nodeId == nodeId) {
                            plugin.bypassed = !bypassed;
                            SetWindowText(plugin.bypassButton, 
                                        plugin.bypassed ? L"Enable" : L"Bypass");
                            break;
                        }
                    }
                    InvalidateRect(hwnd_, nullptr, TRUE);
                }
            }
        } else if (wmId >= ID_BUTTON_REMOVE_BASE) {
            // Remove button clicked
            if (processingChain_) {
                processingChain_->RemovePlugin(nodeId);
            }
            RemovePlugin(nodeId);
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
        return;
    }
    
    // Handle menu commands
    switch (wmId) {
    case ID_MENU_REMOVE:
        if (selectedNodeId_ != 0 && processingChain_) {
            processingChain_->RemovePlugin(selectedNodeId_);
            RemovePlugin(selectedNodeId_);
        }
        break;
        
    case ID_MENU_BYPASS:
        if (selectedNodeId_ != 0 && processingChain_) {
            ProcessingNode* node = processingChain_->GetNode(selectedNodeId_);
            if (node) {
                node->SetBypassed(!node->IsBypassed());
                InvalidateRect(hwnd_, nullptr, TRUE);
            }
        }
        break;
        
    default:
        break;
    }
}

void ActivePluginsPanel::OnHScroll(WPARAM wParam, LPARAM lParam) {
    HWND control = reinterpret_cast<HWND>(lParam);
    if (!control) return;
    
    UINT scrollCode = LOWORD(wParam);
    
    // Check if this is a knob control
    auto it = sliderToParam_.find(control);
    if (it != sliderToParam_.end()) {
        if (scrollCode == TB_ENDTRACK) {
            // Dragging ended, start the reset timer
            SetTimer(hwnd_, TIMER_ID_INTERACTION, 500, nullptr);
            return;
        }
        
        // During dragging
        userIsInteracting_ = true;
        activeSlider_ = control;
        
        // Kill any existing timer to prevent premature reset during dragging
        KillTimer(hwnd_, TIMER_ID_INTERACTION);
        
        // Find the knob and update parameter
        uint32_t nodeId = it->second.first;
        uint32_t paramIndex = it->second.second;
        
        for (auto& plugin : plugins_) {
            if (plugin.nodeId == nodeId) {
                for (auto& ctrl : plugin.parameters) {
                    if (ctrl.parameterIndex == paramIndex && ctrl.knob && ctrl.knob->GetHandle() == control) {
                        float value = ctrl.knob->GetValue();
                        
                        if (processingChain_) {
                            processingChain_->SetParameter(nodeId, paramIndex, value);
                        }
                        
                        // Update value display
                        std::wstringstream ss;
                        ss << std::fixed << std::setprecision(ctrl.info.isInteger ? 0 : 2) << value;
                        SetWindowText(ctrl.valueStatic, ss.str().c_str());
                        
                        return;
                    }
                }
            }
        }
    }
}

void ActivePluginsPanel::OnVScroll(WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    
    int action = LOWORD(wParam);
    
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd_, SB_VERT, &si);
    
    int oldPos = scrollPos_;
    
    switch (action) {
        case SB_LINEUP:
            scrollPos_ = std::max(0, scrollPos_ - PARAM_HEIGHT);
            break;
        case SB_LINEDOWN:
            scrollPos_ = std::min(maxScrollPos_, scrollPos_ + PARAM_HEIGHT);
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
        
        // Update control positions
        RecalculateLayout();
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::OnTimer(WPARAM timerId) {
    if (timerId == TIMER_ID_UPDATE && !userIsInteracting_) {
        // Update all parameter displays
        for (auto& plugin : plugins_) {
            if (plugin.expanded) {
                UpdateParameterControls(plugin);
            }
        }
    } else if (timerId == TIMER_ID_INTERACTION) {
        // Reset interaction flag and clear active slider
        userIsInteracting_ = false;
        activeSlider_ = nullptr;
        KillTimer(hwnd_, TIMER_ID_INTERACTION);
    }
}

LRESULT ActivePluginsPanel::OnNotify(WPARAM wParam, LPARAM lParam) {
    // No custom drawing - use default trackbar rendering
    return 0;
}

void ActivePluginsPanel::DrawPlugin(HDC hdc, const ActivePluginInfo& plugin) {
    int y = plugin.yPos - scrollPos_;
    
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    // Draw plugin header background
    RECT headerRect = { MARGIN, y, rect.right - MARGIN, y + PLUGIN_HEADER_HEIGHT };
    
    COLORREF bgColor = (plugin.nodeId == selectedNodeId_) ? RGB(200, 220, 255) : RGB(240, 240, 240);
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &headerRect, hBrush);
    DeleteObject(hBrush);
    
    // Draw border
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    Rectangle(hdc, headerRect.left, headerRect.top, headerRect.right, headerRect.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Draw plugin name
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    std::wstring nameText = utils::StringToWString(plugin.name);
    RECT textRect = headerRect;
    textRect.left += 10;
    DrawText(hdc, nameText.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw expand/collapse indicator
    std::wstring indicator = plugin.expanded ? L"▼" : L"▶";
    RECT indRect = headerRect;
    indRect.right -= 10;
    DrawText(hdc, indicator.c_str(), -1, &indRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

void ActivePluginsPanel::DrawEmptyState(HDC hdc) {
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    SetTextColor(hdc, RGB(150, 150, 150));
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, L"No plugins loaded\nDouble-click plugins in the browser to load", -1, &rect, 
             DT_CENTER | DT_VCENTER);
}

int ActivePluginsPanel::HitTest(int x, int y) const {
    (void)x;
    
    for (size_t i = 0; i < plugins_.size(); ++i) {
        int pluginY = plugins_[i].yPos - scrollPos_;
        if (y >= pluginY && y < pluginY + PLUGIN_HEADER_HEIGHT) {
            return (int)i;
        }
    }
    
    return -1;
}

void ActivePluginsPanel::RecalculateLayout() {
    RECT clientRect;
    GetClientRect(hwnd_, &clientRect);
    int clientHeight = clientRect.bottom - clientRect.top;
    int clientWidth = clientRect.right - clientRect.left;
    
    // Position Remove All button at the top
    if (removeAllButton_) {
        SetWindowPos(removeAllButton_, nullptr, MARGIN, 5, 0, 0,
                   SWP_NOSIZE | SWP_NOZORDER);
    }
    
    int currentY = 40;  // Start below Remove All button
    
    for (auto& plugin : plugins_) {
        plugin.yPos = currentY;
        
        // Update header button positions
        int y = plugin.yPos - scrollPos_;
        int buttonY = y + (PLUGIN_HEADER_HEIGHT - BUTTON_HEIGHT) / 2;
        
        if (plugin.bypassButton) {
            SetWindowPos(plugin.bypassButton, nullptr,
                       clientWidth - MARGIN - (BUTTON_WIDTH * 2 + 10), buttonY,
                       0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        if (plugin.removeButton) {
            SetWindowPos(plugin.removeButton, nullptr,
                       clientWidth - MARGIN - BUTTON_WIDTH, buttonY,
                       0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        
        // Update control positions
        for (auto& control : plugin.parameters) {
            int absoluteY = plugin.yPos + control.yOffset - scrollPos_;
            
            if (control.labelStatic) {
                SetWindowPos(control.labelStatic, nullptr,
                           MARGIN, absoluteY, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            if (control.valueStatic) {
                SetWindowPos(control.valueStatic, nullptr,
                           MARGIN + LABEL_WIDTH + 10, absoluteY, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            if (control.minusButton) {
                SetWindowPos(control.minusButton, nullptr,
                           MARGIN + LABEL_WIDTH + VALUE_WIDTH + 20, absoluteY, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            if (control.knob) {
                HWND knobHwnd = control.knob->GetHandle();
                if (knobHwnd) {
                    SetWindowPos(knobHwnd, nullptr,
                               MARGIN + LABEL_WIDTH + VALUE_WIDTH + 45, absoluteY - 15, 0, 0,
                               SWP_NOSIZE | SWP_NOZORDER);
                }
            }
            if (control.plusButton) {
                SetWindowPos(control.plusButton, nullptr,
                           MARGIN + LABEL_WIDTH + VALUE_WIDTH + 45 + 50 + 5, absoluteY, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
        }
        
        currentY += plugin.height + PLUGIN_SPACING;
    }
    
    int totalHeight = currentY;
    
    // Update scrollbar
    if (totalHeight > clientHeight) {
        maxScrollPos_ = totalHeight - clientHeight;
        
        SCROLLINFO si = {};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = totalHeight;
        si.nPage = clientHeight;
        si.nPos = scrollPos_;
        SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
        ShowScrollBar(hwnd_, SB_VERT, TRUE);
    } else {
        maxScrollPos_ = 0;
        scrollPos_ = 0;
        ShowScrollBar(hwnd_, SB_VERT, FALSE);
    }
}

void ActivePluginsPanel::ShowContextMenu(int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    
    AppendMenu(hMenu, MF_STRING, ID_MENU_BYPASS, L"Toggle Bypass");
    AppendMenu(hMenu, MF_STRING, ID_MENU_REMOVE, L"Remove Plugin");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, ID_MENU_MOVE_UP, L"Move Up");
    AppendMenu(hMenu, MF_STRING, ID_MENU_MOVE_DOWN, L"Move Down");
    
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, x, y, 0, hwnd_, nullptr);
    DestroyMenu(hMenu);
}

} // namespace violet
