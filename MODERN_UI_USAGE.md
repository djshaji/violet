# Modern UI Usage Examples

## Quick Start

### Using the Theme System

```cpp
#include "violet/theme.h"

// Get theme instance (singleton)
auto& theme = Theme::Instance();

// Switch themes
theme.SetMode(ThemeMode::Dark);   // Dark theme
theme.SetMode(ThemeMode::Light);  // Light theme
theme.SetMode(ThemeMode::System); // Follow system theme

// Get colors
const auto& colors = theme.GetColors();
COLORREF bgColor = colors.background;
COLORREF primaryColor = colors.primary;

// Get cached brushes/pens
HBRUSH bgBrush = theme.GetBackgroundBrush();
HPEN borderPen = theme.GetBorderPen();

// Create scaled fonts
HFONT titleFont = theme.CreateScaledFont(14, FW_SEMIBOLD);
HFONT normalFont = theme.CreateScaledFont(11, FW_NORMAL);
```

### Using DPI Scaling

```cpp
#include "violet/dpi_scaling.h"

// Initialize (call once at startup)
DpiScaling::Instance().Initialize(hwnd);

// Scale values
int scaledWidth = DPI_SCALE(200);          // Scale 200 logical pixels
float scaledF = DPI_SCALE_F(10.5f);        // Scale floating point
int scaledWindow = DPI_SCALE_WINDOW(100, hwnd); // Scale for specific window

// Get scaling factor
float factor = DpiScaling::Instance().GetScaleFactor(hwnd);

// Handle DPI changes
void OnDpiChanged(UINT dpi, const RECT* rect) {
    DpiScaling::Instance().OnDpiChanged(hwnd_, dpi, rect);
    // Update your UI elements...
}
```

### Drawing Modern Controls

```cpp
#include "violet/modern_controls.h"

void OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    // Draw a button
    RECT buttonRect = {10, 10, 110, 40};
    bool isHovered = false;  // Track hover state
    bool isPressed = false;  // Track pressed state
    ModernControls::DrawButton(hdc, buttonRect, L"Click Me", isHovered, isPressed);
    
    // Draw a primary button
    RECT primaryRect = {120, 10, 220, 40};
    ModernControls::DrawPrimaryButton(hdc, primaryRect, L"Primary", isHovered, isPressed);
    
    // Draw a horizontal slider
    RECT sliderRect = {10, 50, 210, 70};
    float value = 0.65f;  // 65%
    ModernControls::DrawHorizontalSlider(hdc, sliderRect, value, isHovered);
    
    // Draw a checkbox
    RECT checkboxRect = {10, 80, 28, 98};
    bool checked = true;
    ModernControls::DrawCheckbox(hdc, checkboxRect, checked, isHovered);
    
    // Draw a panel
    RECT panelRect = {10, 110, 210, 210};
    ModernControls::DrawPanel(hdc, panelRect, true); // elevated panel
    
    // Draw a progress bar
    RECT progressRect = {10, 220, 210, 240};
    float progress = 0.75f;  // 75%
    ModernControls::DrawProgressBar(hdc, progressRect, progress);
    
    EndPaint(hwnd, &ps);
}
```

## Creating a Modern Window

```cpp
#include "violet/main_window.h"
#include "violet/theme.h"
#include "violet/dpi_scaling.h"

class MyModernWindow {
public:
    bool Create(HINSTANCE hInstance) {
        // Initialize DPI scaling first
        DpiScaling::Instance().Initialize();
        
        // Register window class
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = Theme::Instance().GetBackgroundBrush();
        wc.lpszClassName = L"MyModernWindow";
        RegisterClassEx(&wc);
        
        // Create borderless window
        DWORD style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        
        int width = DPI_SCALE(800);
        int height = DPI_SCALE(600);
        
        hwnd_ = CreateWindowEx(
            WS_EX_APPWINDOW,
            L"MyModernWindow",
            L"My Modern App",
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            nullptr, nullptr, hInstance, this
        );
        
        return hwnd_ != nullptr;
    }
    
private:
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_DPICHANGED: {
            UINT dpi = HIWORD(wParam);
            RECT* rect = reinterpret_cast<RECT*>(lParam);
            DpiScaling::Instance().OnDpiChanged(hwnd_, dpi, rect);
            UpdateFonts();
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd_, &ps);
            
            const auto& colors = Theme::Instance().GetColors();
            
            // Fill background
            HBRUSH bgBrush = Theme::Instance().GetBackgroundBrush();
            FillRect(hdc, &ps.rcPaint, bgBrush);
            
            // Draw custom title bar
            RECT titleRect;
            GetClientRect(hwnd_, &titleRect);
            titleRect.bottom = DPI_SCALE(32);
            
            HBRUSH titleBrush = Theme::Instance().GetSurfaceBrush();
            FillRect(hdc, &titleRect, titleBrush);
            
            // Draw title text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, colors.onSurface);
            SelectObject(hdc, titleFont_);
            
            RECT textRect = titleRect;
            textRect.left += DPI_SCALE(12);
            DrawText(hdc, L"My Modern App", -1, &textRect, 
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            
            EndPaint(hwnd_, &ps);
            return 0;
        }
        
        case WM_NCHITTEST: {
            LRESULT result = 0;
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            
            RECT rect;
            GetWindowRect(hwnd_, &rect);
            
            int wx = x - rect.left;
            int wy = y - rect.top;
            
            int borderSize = DPI_SCALE(8);
            int titleHeight = DPI_SCALE(32);
            
            // Check for resize areas
            bool isLeft = wx < borderSize;
            bool isRight = wx > (rect.right - rect.left) - borderSize;
            bool isTop = wy < borderSize;
            bool isBottom = wy > (rect.bottom - rect.top) - borderSize;
            
            if (isTop && isLeft) return HTTOPLEFT;
            if (isTop && isRight) return HTTOPRIGHT;
            if (isBottom && isLeft) return HTBOTTOMLEFT;
            if (isBottom && isRight) return HTBOTTOMRIGHT;
            if (isTop) return HTTOP;
            if (isBottom) return HTBOTTOM;
            if (isLeft) return HTLEFT;
            if (isRight) return HTRIGHT;
            if (wy < titleHeight) return HTCAPTION; // Draggable title bar
            
            return HTCLIENT;
        }
        
        default:
            return DefWindowProc(hwnd_, uMsg, wParam, lParam);
        }
    }
    
    void UpdateFonts() {
        if (titleFont_) DeleteObject(titleFont_);
        if (normalFont_) DeleteObject(normalFont_);
        
        titleFont_ = Theme::Instance().CreateScaledFont(DPI_SCALE(14), FW_SEMIBOLD);
        normalFont_ = Theme::Instance().CreateScaledFont(DPI_SCALE(11), FW_NORMAL);
    }
    
    HWND hwnd_;
    HFONT titleFont_;
    HFONT normalFont_;
};
```

## Color Reference

### Dark Theme Colors
```cpp
RGB(18, 18, 18)      // background
RGB(30, 30, 30)      // surface
RGB(42, 42, 42)      // surfaceVariant
RGB(187, 134, 252)   // primary (purple)
RGB(208, 170, 255)   // primaryVariant
RGB(3, 218, 198)     // secondary (teal)
RGB(230, 230, 230)   // onBackground/onSurface
RGB(60, 60, 60)      // border
RGB(90, 90, 90)      // borderHover
```

### Light Theme Colors
```cpp
RGB(250, 250, 250)   // background
RGB(255, 255, 255)   // surface
RGB(245, 245, 245)   // surfaceVariant
RGB(98, 0, 238)      // primary (purple)
RGB(123, 31, 252)    // primaryVariant
RGB(3, 218, 198)     // secondary (teal)
RGB(33, 33, 33)      // onBackground/onSurface
RGB(224, 224, 224)   // border
RGB(189, 189, 189)   // borderHover
```

## Best Practices

1. **Always Initialize DPI First**: Call `DpiScaling::Instance().Initialize()` before creating windows
2. **Use DPI_SCALE Macros**: Never hard-code pixel values, always use `DPI_SCALE()`
3. **Cache Fonts**: Create fonts once, reuse them, delete on cleanup
4. **Handle WM_DPICHANGED**: Always respond to DPI changes for per-monitor awareness
5. **Use Theme Colors**: Get colors from `Theme::Instance().GetColors()` for consistency
6. **Clean Up Resources**: Delete fonts, brushes, and pens when done
7. **Test Multiple DPIs**: Test your UI at 100%, 125%, 150%, 200%, and 300% scaling

## Performance Tips

1. Use cached theme brushes instead of creating new ones:
   ```cpp
   // Good
   HBRUSH brush = Theme::Instance().GetBackgroundBrush();
   
   // Bad - creates new brush every time
   HBRUSH brush = CreateSolidBrush(colors.background);
   ```

2. Scale dimensions once, cache the results:
   ```cpp
   // In constructor or OnCreate
   scaledMargin_ = DPI_SCALE(10);
   
   // In drawing code
   rect.left += scaledMargin_;  // Use cached value
   ```

3. Use `ModernControls` static methods instead of drawing manually
4. Minimize InvalidateRect calls - only redraw what changed
5. Use double-buffering for complex custom-drawn controls
