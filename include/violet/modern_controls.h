#pragma once

#include <windows.h>
#include "violet/theme.h"
#include "violet/dpi_scaling.h"

namespace violet {

// Modern control renderer for custom-drawn controls
class ModernControls {
public:
    // Button rendering
    static void DrawButton(HDC hdc, const RECT& rect, const wchar_t* text, 
                          bool isHovered, bool isPressed, bool isEnabled = true);
    static void DrawPrimaryButton(HDC hdc, const RECT& rect, const wchar_t* text,
                                 bool isHovered, bool isPressed, bool isEnabled = true);
    
    // Slider rendering
    static void DrawHorizontalSlider(HDC hdc, const RECT& rect, float value, 
                                    bool isHovered, bool isEnabled = true);
    static void DrawVerticalSlider(HDC hdc, const RECT& rect, float value,
                                  bool isHovered, bool isEnabled = true);
    
    // Checkbox rendering
    static void DrawCheckbox(HDC hdc, const RECT& rect, bool checked,
                           bool isHovered, bool isEnabled = true);
    
    // Text input rendering
    static void DrawTextInput(HDC hdc, const RECT& rect, const wchar_t* text,
                            bool isFocused, bool isEnabled = true);
    
    // Combo box rendering
    static void DrawComboBox(HDC hdc, const RECT& rect, const wchar_t* text,
                           bool isHovered, bool isDropped, bool isEnabled = true);
    
    // Separator/divider
    static void DrawSeparator(HDC hdc, const RECT& rect, bool isVertical = false);
    
    // Panel/surface
    static void DrawPanel(HDC hdc, const RECT& rect, bool elevated = false);
    
    // Progress bar
    static void DrawProgressBar(HDC hdc, const RECT& rect, float progress);
    
private:
    // Helper methods
    static RECT InflateRectDPI(const RECT& rect, int dx, int dy);
    static int ScaleDPI(int value);
    static void DrawRoundedRect(HDC hdc, const RECT& rect, int radius, HBRUSH brush, HPEN pen = nullptr);
    static void DrawText(HDC hdc, const RECT& rect, const wchar_t* text, 
                        COLORREF color, UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE);
};

// Custom window procedure for owner-drawn controls
LRESULT CALLBACK ModernButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                                 UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

} // namespace violet
