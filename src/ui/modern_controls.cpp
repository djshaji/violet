#include "violet/modern_controls.h"
#include <algorithm>
#include <cmath>

namespace violet {

int ModernControls::ScaleDPI(int value) {
    return DPI_SCALE(value);
}

RECT ModernControls::InflateRectDPI(const RECT& rect, int dx, int dy) {
    RECT result = rect;
    InflateRect(&result, ScaleDPI(dx), ScaleDPI(dy));
    return result;
}

void ModernControls::DrawRoundedRect(HDC hdc, const RECT& rect, int radius, HBRUSH brush, HPEN pen) {
    int scaledRadius = ScaleDPI(radius);
    
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = pen ? SelectObject(hdc, pen) : SelectObject(hdc, GetStockObject(NULL_PEN));
    
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 
             scaledRadius, scaledRadius);
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
}

void ModernControls::DrawText(HDC hdc, const RECT& rect, const wchar_t* text, 
                             COLORREF color, UINT format) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    
    HFONT font = Theme::Instance().CreateScaledFont(ScaleDPI(11));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    
    ::DrawText(hdc, text, -1, const_cast<RECT*>(&rect), format);
    
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void ModernControls::DrawButton(HDC hdc, const RECT& rect, const wchar_t* text,
                               bool isHovered, bool isPressed, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    COLORREF bgColor = colors.surface;
    COLORREF borderColor = colors.border;
    COLORREF textColor = colors.onSurface;
    
    if (!isEnabled) {
        textColor = RGB(
            GetRValue(textColor) / 2,
            GetGValue(textColor) / 2,
            GetBValue(textColor) / 2
        );
    } else if (isPressed) {
        bgColor = colors.surfaceVariant;
    } else if (isHovered) {
        borderColor = colors.borderHover;
    }
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    DrawRoundedRect(hdc, rect, 4, bgBrush, borderPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
    
    // Draw text
    DrawText(hdc, rect, text, textColor);
}

void ModernControls::DrawPrimaryButton(HDC hdc, const RECT& rect, const wchar_t* text,
                                      bool isHovered, bool isPressed, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    COLORREF bgColor = colors.primary;
    COLORREF textColor = colors.onPrimary;
    
    if (!isEnabled) {
        bgColor = RGB(
            GetRValue(bgColor) / 2,
            GetGValue(bgColor) / 2,
            GetBValue(bgColor) / 2
        );
    } else if (isPressed) {
        bgColor = RGB(
            std::max(0, GetRValue(bgColor) - 20),
            std::max(0, GetGValue(bgColor) - 20),
            std::max(0, GetBValue(bgColor) - 20)
        );
    } else if (isHovered) {
        bgColor = colors.primaryVariant;
    }
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    DrawRoundedRect(hdc, rect, 4, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw text
    DrawText(hdc, rect, text, textColor);
}

void ModernControls::DrawHorizontalSlider(HDC hdc, const RECT& rect, float value,
                                         bool isHovered, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    int trackHeight = ScaleDPI(6);  // Increased from 4 to 6
    int thumbSize = ScaleDPI(24);   // Increased from 16 to 24
    
    // Draw track background
    RECT trackRect = rect;
    trackRect.top = (rect.top + rect.bottom - trackHeight) / 2;
    trackRect.bottom = trackRect.top + trackHeight;
    
    COLORREF trackColor = isEnabled ? colors.surfaceVariant : colors.border;
    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    DrawRoundedRect(hdc, trackRect, 2, trackBrush);
    DeleteObject(trackBrush);
    
    // Draw filled portion
    if (isEnabled && value > 0.0f) {
        RECT filledRect = trackRect;
        filledRect.right = filledRect.left + 
            static_cast<int>((filledRect.right - filledRect.left) * std::clamp(value, 0.0f, 1.0f));
        
        HBRUSH filledBrush = CreateSolidBrush(colors.primary);
        DrawRoundedRect(hdc, filledRect, 2, filledBrush);
        DeleteObject(filledBrush);
    }
    
    // Draw thumb
    int thumbX = rect.left + static_cast<int>((rect.right - rect.left - thumbSize) * 
                                               std::clamp(value, 0.0f, 1.0f));
    RECT thumbRect = {
        thumbX,
        (rect.top + rect.bottom - thumbSize) / 2,
        thumbX + thumbSize,
        (rect.top + rect.bottom + thumbSize) / 2
    };
    
    COLORREF thumbColor = isEnabled ? (isHovered ? colors.primaryVariant : colors.primary) 
                                    : colors.border;
    HBRUSH thumbBrush = CreateSolidBrush(thumbColor);
    HGDIOBJ oldBrush = SelectObject(hdc, thumbBrush);
    HPEN thumbPen = CreatePen(PS_SOLID, 1, thumbColor);
    HGDIOBJ oldPen = SelectObject(hdc, thumbPen);
    Ellipse(hdc, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(thumbPen);
    DeleteObject(thumbBrush);
}

void ModernControls::DrawVerticalSlider(HDC hdc, const RECT& rect, float value,
                                       bool isHovered, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    int trackWidth = ScaleDPI(6);   // Increased from 4 to 6
    int thumbSize = ScaleDPI(24);   // Increased from 16 to 24
    
    // Draw track background
    RECT trackRect = rect;
    trackRect.left = (rect.left + rect.right - trackWidth) / 2;
    trackRect.right = trackRect.left + trackWidth;
    
    COLORREF trackColor = isEnabled ? colors.surfaceVariant : colors.border;
    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    DrawRoundedRect(hdc, trackRect, 2, trackBrush);
    DeleteObject(trackBrush);
    
    // Draw filled portion
    if (isEnabled && value > 0.0f) {
        RECT filledRect = trackRect;
        int fillHeight = static_cast<int>((filledRect.bottom - filledRect.top) * 
                                         std::clamp(value, 0.0f, 1.0f));
        filledRect.top = filledRect.bottom - fillHeight;
        
        HBRUSH filledBrush = CreateSolidBrush(colors.primary);
        DrawRoundedRect(hdc, filledRect, 2, filledBrush);
        DeleteObject(filledBrush);
    }
    
    // Draw thumb
    int thumbY = rect.bottom - static_cast<int>((rect.bottom - rect.top - thumbSize) * 
                                                 std::clamp(value, 0.0f, 1.0f));
    RECT thumbRect = {
        (rect.left + rect.right - thumbSize) / 2,
        thumbY - thumbSize / 2,
        (rect.left + rect.right + thumbSize) / 2,
        thumbY + thumbSize / 2
    };
    
    COLORREF thumbColor = isEnabled ? (isHovered ? colors.primaryVariant : colors.primary) 
                                    : colors.border;
    HBRUSH thumbBrush = CreateSolidBrush(thumbColor);
    HGDIOBJ oldBrush = SelectObject(hdc, thumbBrush);
    HPEN thumbPen = CreatePen(PS_SOLID, 1, thumbColor);
    HGDIOBJ oldPen = SelectObject(hdc, thumbPen);
    Ellipse(hdc, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(thumbPen);
    DeleteObject(thumbBrush);
}

void ModernControls::DrawCheckbox(HDC hdc, const RECT& rect, bool checked,
                                 bool isHovered, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    int size = std::min(rect.right - rect.left, rect.bottom - rect.top);
    size = ScaleDPI(18);
    
    RECT boxRect = {
        rect.left,
        (rect.top + rect.bottom - size) / 2,
        rect.left + size,
        (rect.top + rect.bottom + size) / 2
    };
    
    COLORREF bgColor = checked ? colors.primary : colors.surface;
    COLORREF borderColor = checked ? colors.primary : 
                          (isHovered ? colors.borderHover : colors.border);
    
    if (!isEnabled) {
        bgColor = colors.surfaceVariant;
        borderColor = colors.border;
    }
    
    // Draw box
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    DrawRoundedRect(hdc, boxRect, 3, bgBrush, borderPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
    
    // Draw checkmark
    if (checked) {
        HPEN checkPen = CreatePen(PS_SOLID, ScaleDPI(2), colors.onPrimary);
        HGDIOBJ oldPen = SelectObject(hdc, checkPen);
        
        int padding = ScaleDPI(4);
        MoveToEx(hdc, boxRect.left + padding, boxRect.top + size / 2, nullptr);
        LineTo(hdc, boxRect.left + size / 2 - padding / 2, boxRect.bottom - padding);
        LineTo(hdc, boxRect.right - padding, boxRect.top + padding);
        
        SelectObject(hdc, oldPen);
        DeleteObject(checkPen);
    }
}

void ModernControls::DrawTextInput(HDC hdc, const RECT& rect, const wchar_t* text,
                                  bool isFocused, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    COLORREF bgColor = isEnabled ? colors.surface : colors.surfaceVariant;
    COLORREF borderColor = isFocused ? colors.primary : colors.border;
    COLORREF textColor = isEnabled ? colors.onSurface : colors.border;
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    HPEN borderPen = CreatePen(PS_SOLID, isFocused ? 2 : 1, borderColor);
    DrawRoundedRect(hdc, rect, 4, bgBrush, borderPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
    
    // Draw text
    RECT textRect = InflateRectDPI(rect, -8, 0);
    DrawText(hdc, textRect, text, textColor, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void ModernControls::DrawComboBox(HDC hdc, const RECT& rect, const wchar_t* text,
                                 bool isHovered, bool isDropped, bool isEnabled) {
    const auto& colors = Theme::Instance().GetColors();
    
    COLORREF bgColor = isEnabled ? colors.surface : colors.surfaceVariant;
    COLORREF borderColor = isDropped ? colors.primary : 
                          (isHovered ? colors.borderHover : colors.border);
    COLORREF textColor = isEnabled ? colors.onSurface : colors.border;
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    DrawRoundedRect(hdc, rect, 4, bgBrush, borderPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
    
    // Draw text
    RECT textRect = InflateRectDPI(rect, -8, 0);
    textRect.right -= ScaleDPI(24); // Space for arrow
    DrawText(hdc, textRect, text, textColor, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Draw dropdown arrow
    int arrowSize = ScaleDPI(8);
    int arrowX = rect.right - ScaleDPI(16);
    int arrowY = (rect.top + rect.bottom) / 2;
    
    HPEN arrowPen = CreatePen(PS_SOLID, ScaleDPI(2), textColor);
    HGDIOBJ oldPen = SelectObject(hdc, arrowPen);
    
    MoveToEx(hdc, arrowX - arrowSize / 2, arrowY - arrowSize / 4, nullptr);
    LineTo(hdc, arrowX, arrowY + arrowSize / 4);
    LineTo(hdc, arrowX + arrowSize / 2, arrowY - arrowSize / 4);
    
    SelectObject(hdc, oldPen);
    DeleteObject(arrowPen);
}

void ModernControls::DrawSeparator(HDC hdc, const RECT& rect, bool isVertical) {
    const auto& colors = Theme::Instance().GetColors();
    
    HPEN separatorPen = CreatePen(PS_SOLID, 1, colors.border);
    HGDIOBJ oldPen = SelectObject(hdc, separatorPen);
    
    if (isVertical) {
        int x = (rect.left + rect.right) / 2;
        MoveToEx(hdc, x, rect.top, nullptr);
        LineTo(hdc, x, rect.bottom);
    } else {
        int y = (rect.top + rect.bottom) / 2;
        MoveToEx(hdc, rect.left, y, nullptr);
        LineTo(hdc, rect.right, y);
    }
    
    SelectObject(hdc, oldPen);
    DeleteObject(separatorPen);
}

void ModernControls::DrawPanel(HDC hdc, const RECT& rect, bool elevated) {
    const auto& colors = Theme::Instance().GetColors();
    
    COLORREF bgColor = elevated ? colors.surface : colors.background;
    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    
    if (elevated) {
        HPEN borderPen = CreatePen(PS_SOLID, 1, colors.border);
        DrawRoundedRect(hdc, rect, 8, bgBrush, borderPen);
        DeleteObject(borderPen);
    } else {
        FillRect(hdc, &rect, bgBrush);
    }
    
    DeleteObject(bgBrush);
}

void ModernControls::DrawProgressBar(HDC hdc, const RECT& rect, float progress) {
    const auto& colors = Theme::Instance().GetColors();
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(colors.surfaceVariant);
    DrawRoundedRect(hdc, rect, 4, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw progress
    if (progress > 0.0f) {
        RECT progressRect = rect;
        progressRect.right = progressRect.left + 
            static_cast<int>((progressRect.right - progressRect.left) * 
                           std::clamp(progress, 0.0f, 1.0f));
        
        HBRUSH progressBrush = CreateSolidBrush(colors.primary);
        DrawRoundedRect(hdc, progressRect, 4, progressBrush);
        DeleteObject(progressBrush);
    }
}

} // namespace violet
