#include "violet/active_plugins_panel.h"
#include "violet/audio_processing_chain.h"
#include "violet/plugin_manager.h"
#include "violet/utils.h"
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
    , processingChain_(nullptr)
    , selectedNodeId_(0)
    , hoveredPluginIndex_(-1)
    , scrollPos_(0)
    , maxScrollPos_(0) {
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
    
    plugins_.push_back(plugin);
    
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
        DestroyParameterControls(plugin);
    }
    plugins_.clear();
    sliderToParam_.clear();
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

void ActivePluginsPanel::Resize(int x, int y, int width, int height) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, x, y, width, height, SWP_NOZORDER);
        RecalculateLayout();
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
        
        // Slider
        control.slider = CreateWindowEx(
            0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | TBS_NOTICKS,
            MARGIN + LABEL_WIDTH + VALUE_WIDTH + 20, absoluteY + 2,
            SLIDER_WIDTH, 20,
            hwnd_, nullptr, hInstance_, nullptr
        );
        
        SendMessage(control.slider, TBM_SETRANGE, TRUE, MAKELONG(0, SLIDER_RESOLUTION));
        int sliderPos = ValueToSliderPos(currentValue, param);
        SendMessage(control.slider, TBM_SETPOS, TRUE, sliderPos);
        
        sliderToParam_[control.slider] = std::make_pair(plugin.nodeId, param.index);
        
        plugin.parameters.push_back(control);
        yOffset += PARAM_HEIGHT;
    }
    
    plugin.height = yOffset;
}

void ActivePluginsPanel::DestroyParameterControls(ActivePluginInfo& plugin) {
    for (auto& control : plugin.parameters) {
        if (control.labelStatic) {
            sliderToParam_.erase(control.slider);
            DestroyWindow(control.labelStatic);
        }
        if (control.valueStatic) DestroyWindow(control.valueStatic);
        if (control.slider) DestroyWindow(control.slider);
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
        
        // Update slider position
        int sliderPos = ValueToSliderPos(value, control.info);
        SendMessage(control.slider, TBM_SETPOS, TRUE, sliderPos);
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
        
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void ActivePluginsPanel::OnCreate() {
    // Nothing specific to initialize
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
    (void)lParam;
    int wmId = LOWORD(wParam);
    
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
    HWND slider = reinterpret_cast<HWND>(lParam);
    if (slider) {
        OnSliderChange(slider);
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
    if (timerId == TIMER_ID_UPDATE) {
        // Update all parameter displays
        for (auto& plugin : plugins_) {
            if (plugin.expanded) {
                UpdateParameterControls(plugin);
            }
        }
    }
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
    
    int currentY = MARGIN;
    
    for (auto& plugin : plugins_) {
        plugin.yPos = currentY;
        
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
            if (control.slider) {
                SetWindowPos(control.slider, nullptr,
                           MARGIN + LABEL_WIDTH + VALUE_WIDTH + 20, absoluteY + 2, 0, 0,
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
