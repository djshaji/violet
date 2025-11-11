#pragma once

#include <windows.h>
#include <string>
#include <map>
#include <vector>

namespace violet {

// Theme color definitions
struct ThemeColors {
    // Window backgrounds
    COLORREF windowBackground;
    COLORREF panelBackground;
    COLORREF controlBackground;
    
    // Text colors
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF textDisabled;
    
    // UI element colors
    COLORREF border;
    COLORREF borderLight;
    COLORREF borderDark;
    
    // Interactive elements
    COLORREF buttonFace;
    COLORREF buttonHover;
    COLORREF buttonPressed;
    COLORREF buttonText;
    
    // Plugin panel specific
    COLORREF pluginHeader;
    COLORREF pluginHeaderText;
    COLORREF pluginBackground;
    COLORREF pluginBorder;
    
    // Status indicators
    COLORREF statusActive;
    COLORREF statusBypassed;
    COLORREF statusError;
    COLORREF statusWarning;
    
    // Selection and highlights
    COLORREF selectionBackground;
    COLORREF selectionText;
    COLORREF highlight;
    
    // Scrollbar
    COLORREF scrollbarBackground;
    COLORREF scrollbarThumb;
    COLORREF scrollbarThumbHover;
};

// Theme types
enum class ThemeType {
    Light,
    Dark,
    System  // Follow system theme
};

// Theme manager for application-wide theming
class ThemeManager {
public:
    static ThemeManager& GetInstance();
    
    // Theme management
    void SetTheme(ThemeType theme);
    ThemeType GetCurrentTheme() const { return currentTheme_; }
    const ThemeColors& GetColors() const { return currentColors_; }
    
    // Color access
    COLORREF GetColor(const std::string& colorName) const;
    
    // Brush management
    HBRUSH GetBrush(const std::string& colorName);
    HBRUSH GetBackgroundBrush() { return GetBrush("windowBackground"); }
    HBRUSH GetPanelBrush() { return GetBrush("panelBackground"); }
    HBRUSH GetControlBrush() { return GetBrush("controlBackground"); }
    
    // Pen management
    HPEN GetPen(const std::string& colorName, int width = 1);
    HPEN GetBorderPen() { return GetPen("border", 1); }
    
    // System integration
    bool IsSystemDarkMode() const;
    void UpdateFromSystem();
    
    // Configuration
    void LoadFromConfig();
    void SaveToConfig();
    
    // Apply theme to existing controls
    void ApplyToWindow(HWND hwnd);
    void ApplyToAllWindows();
    
private:
    ThemeManager();
    ~ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    
    void InitializeThemes();
    void CreateBrushes();
    void CreatePens();
    void DestroyBrushes();
    void DestroyPens();
    void UpdateColors();
    
    ThemeType currentTheme_;
    ThemeColors currentColors_;
    
    // Predefined themes
    ThemeColors lightTheme_;
    ThemeColors darkTheme_;
    
    // GDI objects cache
    std::map<std::string, HBRUSH> brushes_;
    std::map<std::string, HPEN> pens_;
    
    // Window tracking for theme updates
    std::vector<HWND> trackedWindows_;
};

} // namespace violet
