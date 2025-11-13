#pragma once

#include <windows.h>
#include <string>

namespace violet {

// Modern color schemes
struct ColorScheme {
    COLORREF background;
    COLORREF surface;
    COLORREF surfaceVariant;
    COLORREF primary;
    COLORREF primaryVariant;
    COLORREF secondary;
    COLORREF onBackground;
    COLORREF onSurface;
    COLORREF onPrimary;
    COLORREF border;
    COLORREF borderHover;
    COLORREF shadow;
    COLORREF accent;
    COLORREF error;
    COLORREF success;
    COLORREF warning;
};

enum class ThemeMode {
    Light,
    Dark,
    System
};

class Theme {
public:
    static Theme& Instance();
    
    // Theme management
    void SetMode(ThemeMode mode);
    ThemeMode GetMode() const { return mode_; }
    
    // Color access
    const ColorScheme& GetColors() const { return *currentColors_; }
    
    // Brushes (cached for performance)
    HBRUSH GetBackgroundBrush() const { return backgroundBrush_; }
    HBRUSH GetSurfaceBrush() const { return surfaceBrush_; }
    HBRUSH GetPrimaryBrush() const { return primaryBrush_; }
    
    // Pens (cached for performance)
    HPEN GetBorderPen() const { return borderPen_; }
    HPEN GetBorderHoverPen() const { return borderHoverPen_; }
    
    // Font creation
    HFONT CreateScaledFont(int baseSize, int weight = FW_NORMAL, bool italic = false) const;
    
    // Colors
    static ColorScheme LightTheme();
    static ColorScheme DarkTheme();
    
private:
    Theme();
    ~Theme();
    Theme(const Theme&) = delete;
    Theme& operator=(const Theme&) = delete;
    
    void UpdateTheme();
    void CreateResources();
    void DestroyResources();
    
    ThemeMode mode_;
    const ColorScheme* currentColors_;
    
    // GDI resources
    HBRUSH backgroundBrush_;
    HBRUSH surfaceBrush_;
    HBRUSH primaryBrush_;
    HPEN borderPen_;
    HPEN borderHoverPen_;
};

} // namespace violet
