#include "violet/active_plugins_panel.h"
#include "violet/audio_processing_chain.h"
#include "violet/plugin_manager.h"
#include "violet/utils.h"
#include <windowsx.h>
#include <algorithm>

namespace violet {

const wchar_t* ActivePluginsPanel::CLASS_NAME = L"VioletActivePluginsPanel";

ActivePluginsPanel::ActivePluginsPanel()
    : hwnd_(nullptr)
    , hInstance_(nullptr)
    , processingChain_(nullptr)
    , selectedNodeId_(0)
    , hoveredPluginIndex_(-1) {
}

ActivePluginsPanel::~ActivePluginsPanel() {
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
    
    // Create the window
    hwnd_ = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Active Plugins",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
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
    plugin.xPos = 0;
    plugin.yPos = 0;
    plugin.width = PLUGIN_WIDTH;
    plugin.height = PLUGIN_HEIGHT;
    
    plugins_.push_back(plugin);
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
    plugins_.clear();
    selectedNodeId_ = 0;
    hoveredPluginIndex_ = -1;
    
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void ActivePluginsPanel::Refresh() {
    if (processingChain_) {
        // Sync with actual chain state
        auto nodeIds = processingChain_->GetNodeIds();
        
        // Remove plugins that no longer exist in chain
        plugins_.erase(
            std::remove_if(plugins_.begin(), plugins_.end(),
                          [&nodeIds](const ActivePluginInfo& p) {
                              return std::find(nodeIds.begin(), nodeIds.end(), p.nodeId) == nodeIds.end();
                          }),
            plugins_.end());
        
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

LRESULT CALLBACK ActivePluginsPanel::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ActivePluginsPanel* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<ActivePluginsPanel*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->hwnd_ = hwnd;
    } else {
        pThis = reinterpret_cast<ActivePluginsPanel*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
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
        
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void ActivePluginsPanel::OnCreate() {
    // Nothing to initialize yet
}

void ActivePluginsPanel::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    // Create back buffer for flicker-free drawing
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
    
    // Fill background
    FillRect(hdcMem, &rect, (HBRUSH)(COLOR_WINDOW + 1));
    
    if (plugins_.empty()) {
        DrawEmptyState(hdcMem);
    } else {
        // Draw connections between plugins
        for (size_t i = 0; i < plugins_.size(); ++i) {
            if (i > 0) {
                DrawConnection(hdcMem, plugins_[i - 1], plugins_[i]);
            }
        }
        
        // Draw plugins
        for (const auto& plugin : plugins_) {
            DrawPlugin(hdcMem, plugin);
        }
    }
    
    // Copy back buffer to screen
    BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
    
    // Cleanup
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    
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
        // TODO: Open plugin editor window
        // For now, just select it
        selectedNodeId_ = plugins_[index].nodeId;
    }
}

void ActivePluginsPanel::OnRButtonDown(int x, int y) {
    int index = HitTest(x, y);
    if (index >= 0) {
        selectedNodeId_ = plugins_[index].nodeId;
        InvalidateRect(hwnd_, nullptr, TRUE);
        
        // Convert client coordinates to screen coordinates
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
    
    if (selectedNodeId_ == 0) {
        return;
    }
    
    switch (wmId) {
    case ID_MENU_REMOVE:
        if (processingChain_) {
            processingChain_->RemovePlugin(selectedNodeId_);
            RemovePlugin(selectedNodeId_);
        }
        break;
        
    case ID_MENU_BYPASS:
        // TODO: Implement bypass toggle
        break;
        
    case ID_MENU_EDIT:
        // TODO: Open plugin editor
        break;
        
    case ID_MENU_MOVE_UP:
    case ID_MENU_MOVE_DOWN:
        // TODO: Implement reordering
        break;
    }
}

void ActivePluginsPanel::DrawPlugin(HDC hdc, const ActivePluginInfo& plugin) {
    bool isSelected = (plugin.nodeId == selectedNodeId_);
    
    // Determine colors
    COLORREF borderColor = isSelected ? RGB(0, 120, 215) : RGB(100, 100, 100);
    COLORREF bgColor = plugin.bypassed ? RGB(220, 220, 220) : RGB(240, 240, 240);
    COLORREF textColor = plugin.bypassed ? RGB(128, 128, 128) : RGB(0, 0, 0);
    
    // Draw border
    HPEN hPen = CreatePen(PS_SOLID, isSelected ? 2 : 1, borderColor);
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    
    Rectangle(hdc, plugin.xPos, plugin.yPos, 
              plugin.xPos + plugin.width, plugin.yPos + plugin.height);
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
    
    // Draw plugin name (truncate if too long)
    std::string displayName = plugin.name;
    if (displayName.length() > 15) {
        displayName = displayName.substr(0, 12) + "...";
    }
    
    RECT textRect = { 
        plugin.xPos + 5, 
        plugin.yPos + 5, 
        plugin.xPos + plugin.width - 5, 
        plugin.yPos + plugin.height - 5 
    };
    
    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    
    std::wstring wName = utils::StringToWString(displayName);
    DrawText(hdc, wName.c_str(), -1, &textRect, 
             DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    
    // Draw status indicator
    if (!plugin.active) {
        // Draw "inactive" indicator
        RECT statusRect = { 
            plugin.xPos + plugin.width - 15, 
            plugin.yPos + 5, 
            plugin.xPos + plugin.width - 5, 
            plugin.yPos + 15 
        };
        HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
        FillRect(hdc, &statusRect, redBrush);
        DeleteObject(redBrush);
    }
}

void ActivePluginsPanel::DrawConnection(HDC hdc, const ActivePluginInfo& from, const ActivePluginInfo& to) {
    // Draw line from right of 'from' to left of 'to'
    int x1 = from.xPos + from.width;
    int y1 = from.yPos + from.height / 2;
    int x2 = to.xPos;
    int y2 = to.yPos + to.height / 2;
    
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, x1, y1, nullptr);
    LineTo(hdc, x2, y2);
    
    // Draw arrow head
    int arrowSize = 8;
    POINT arrow[3];
    arrow[0].x = x2;
    arrow[0].y = y2;
    arrow[1].x = x2 - arrowSize;
    arrow[1].y = y2 - arrowSize / 2;
    arrow[2].x = x2 - arrowSize;
    arrow[2].y = y2 + arrowSize / 2;
    
    HBRUSH hBrush = CreateSolidBrush(RGB(100, 100, 100));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    Polygon(hdc, arrow, 3);
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void ActivePluginsPanel::DrawEmptyState(HDC hdc) {
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    SetTextColor(hdc, RGB(150, 150, 150));
    SetBkMode(hdc, TRANSPARENT);
    
    // Select a larger font
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    DrawText(hdc, L"No plugins loaded", -1, &rect, 
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    RECT rect2 = rect;
    rect2.top += 30;
    DrawText(hdc, L"Double-click a plugin in the browser to add it", -1, &rect2, 
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

int ActivePluginsPanel::HitTest(int x, int y) const {
    for (size_t i = 0; i < plugins_.size(); ++i) {
        const auto& plugin = plugins_[i];
        if (x >= plugin.xPos && x <= plugin.xPos + plugin.width &&
            y >= plugin.yPos && y <= plugin.yPos + plugin.height) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ActivePluginsPanel::RecalculateLayout() {
    if (plugins_.empty()) {
        return;
    }
    
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    int availableWidth = rect.right - 2 * MARGIN;
    int currentX = MARGIN;
    int currentY = MARGIN;
    
    for (auto& plugin : plugins_) {
        plugin.xPos = currentX;
        plugin.yPos = currentY;
        plugin.width = PLUGIN_WIDTH;
        plugin.height = PLUGIN_HEIGHT;
        
        currentX += PLUGIN_WIDTH + PLUGIN_SPACING;
        
        // Wrap to next line if needed
        if (currentX + PLUGIN_WIDTH > availableWidth) {
            currentX = MARGIN;
            currentY += PLUGIN_HEIGHT + PLUGIN_SPACING;
        }
    }
}

void ActivePluginsPanel::ShowContextMenu(int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    
    AppendMenu(hMenu, MF_STRING, ID_MENU_EDIT, L"Edit Parameters...");
    AppendMenu(hMenu, MF_STRING, ID_MENU_BYPASS, L"Bypass");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, ID_MENU_MOVE_UP, L"Move Up");
    AppendMenu(hMenu, MF_STRING, ID_MENU_MOVE_DOWN, L"Move Down");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, ID_MENU_REMOVE, L"Remove");
    
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, hwnd_, nullptr);
    DestroyMenu(hMenu);
}

} // namespace violet
