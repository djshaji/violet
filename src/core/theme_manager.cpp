#include "violet/theme_manager.h"
#include "violet/config_manager.h"
#include <dwmapi.h>
#include <algorithm>

namespace violet {

// Helper to create RGB color
constexpr COLORREF RGB_COLOR(int r, int g, int b) {
    return RGB(r, g, b);
}

ThemeManager& ThemeManager::GetInstance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : currentTheme_(ThemeType::System) {
    InitializeThemes();
    UpdateFromSystem();
}

ThemeManager::~ThemeManager() {
    DestroyBrushes();
    DestroyPens();
}

void ThemeManager::InitializeThemes() {
    // Light theme colors
    lightTheme_.windowBackground = RGB_COLOR(240, 240, 240);
    lightTheme_.panelBackground = RGB_COLOR(255, 255, 255);
    lightTheme_.controlBackground = RGB_COLOR(250, 250, 250);
    
    lightTheme_.textPrimary = RGB_COLOR(0, 0, 0);
    lightTheme_.textSecondary = RGB_COLOR(96, 96, 96);
    lightTheme_.textDisabled = RGB_COLOR(160, 160, 160);
    
    lightTheme_.border = RGB_COLOR(200, 200, 200);
    lightTheme_.borderLight = RGB_COLOR(220, 220, 220);
    lightTheme_.borderDark = RGB_COLOR(160, 160, 160);
    
    lightTheme_.buttonFace = RGB_COLOR(225, 225, 225);
    lightTheme_.buttonHover = RGB_COLOR(210, 210, 210);
    lightTheme_.buttonPressed = RGB_COLOR(190, 190, 190);
    lightTheme_.buttonText = RGB_COLOR(0, 0, 0);
    
    lightTheme_.pluginHeader = RGB_COLOR(230, 230, 250);
    lightTheme_.pluginHeaderText = RGB_COLOR(0, 0, 0);
    lightTheme_.pluginBackground = RGB_COLOR(250, 250, 255);
    lightTheme_.pluginBorder = RGB_COLOR(180, 180, 200);
    
    lightTheme_.statusActive = RGB_COLOR(0, 180, 0);
    lightTheme_.statusBypassed = RGB_COLOR(255, 140, 0);
    lightTheme_.statusError = RGB_COLOR(220, 20, 20);
    lightTheme_.statusWarning = RGB_COLOR(255, 200, 0);
    
    lightTheme_.selectionBackground = RGB_COLOR(0, 120, 215);
    lightTheme_.selectionText = RGB_COLOR(255, 255, 255);
    lightTheme_.highlight = RGB_COLOR(100, 150, 255);
    
    lightTheme_.scrollbarBackground = RGB_COLOR(240, 240, 240);
    lightTheme_.scrollbarThumb = RGB_COLOR(200, 200, 200);
    lightTheme_.scrollbarThumbHover = RGB_COLOR(160, 160, 160);
    
    // Dark theme colors
    darkTheme_.windowBackground = RGB_COLOR(32, 32, 32);
    darkTheme_.panelBackground = RGB_COLOR(45, 45, 48);
    darkTheme_.controlBackground = RGB_COLOR(37, 37, 38);
    
    darkTheme_.textPrimary = RGB_COLOR(255, 255, 255);
    darkTheme_.textSecondary = RGB_COLOR(180, 180, 180);
    darkTheme_.textDisabled = RGB_COLOR(120, 120, 120);
    
    darkTheme_.border = RGB_COLOR(60, 60, 60);
    darkTheme_.borderLight = RGB_COLOR(80, 80, 80);
    darkTheme_.borderDark = RGB_COLOR(40, 40, 40);
    
    darkTheme_.buttonFace = RGB_COLOR(55, 55, 55);
    darkTheme_.buttonHover = RGB_COLOR(70, 70, 70);
    darkTheme_.buttonPressed = RGB_COLOR(85, 85, 85);
    darkTheme_.buttonText = RGB_COLOR(255, 255, 255);
    
    darkTheme_.pluginHeader = RGB_COLOR(50, 50, 70);
    darkTheme_.pluginHeaderText = RGB_COLOR(220, 220, 255);
    darkTheme_.pluginBackground = RGB_COLOR(40, 40, 45);
    darkTheme_.pluginBorder = RGB_COLOR(60, 60, 80);
    
    darkTheme_.statusActive = RGB_COLOR(0, 220, 0);
    darkTheme_.statusBypassed = RGB_COLOR(255, 160, 0);
    darkTheme_.statusError = RGB_COLOR(255, 60, 60);
    darkTheme_.statusWarning = RGB_COLOR(255, 220, 0);
    
    darkTheme_.selectionBackground = RGB_COLOR(0, 120, 215);
    darkTheme_.selectionText = RGB_COLOR(255, 255, 255);
    darkTheme_.highlight = RGB_COLOR(80, 120, 200);
    
    darkTheme_.scrollbarBackground = RGB_COLOR(32, 32, 32);
    darkTheme_.scrollbarThumb = RGB_COLOR(80, 80, 80);
    darkTheme_.scrollbarThumbHover = RGB_COLOR(120, 120, 120);
}

void ThemeManager::SetTheme(ThemeType theme) {
    currentTheme_ = theme;
    UpdateColors();
    CreateBrushes();
    CreatePens();
    ApplyToAllWindows();
    SaveToConfig();
}

void ThemeManager::UpdateColors() {
    DestroyBrushes();
    DestroyPens();
    
    if (currentTheme_ == ThemeType::System) {
        currentColors_ = IsSystemDarkMode() ? darkTheme_ : lightTheme_;
    } else if (currentTheme_ == ThemeType::Dark) {
        currentColors_ = darkTheme_;
    } else {
        currentColors_ = lightTheme_;
    }
}

void ThemeManager::CreateBrushes() {
    brushes_["windowBackground"] = CreateSolidBrush(currentColors_.windowBackground);
    brushes_["panelBackground"] = CreateSolidBrush(currentColors_.panelBackground);
    brushes_["controlBackground"] = CreateSolidBrush(currentColors_.controlBackground);
    brushes_["buttonFace"] = CreateSolidBrush(currentColors_.buttonFace);
    brushes_["buttonHover"] = CreateSolidBrush(currentColors_.buttonHover);
    brushes_["buttonPressed"] = CreateSolidBrush(currentColors_.buttonPressed);
    brushes_["pluginHeader"] = CreateSolidBrush(currentColors_.pluginHeader);
    brushes_["pluginBackground"] = CreateSolidBrush(currentColors_.pluginBackground);
    brushes_["selectionBackground"] = CreateSolidBrush(currentColors_.selectionBackground);
    brushes_["scrollbarBackground"] = CreateSolidBrush(currentColors_.scrollbarBackground);
    brushes_["scrollbarThumb"] = CreateSolidBrush(currentColors_.scrollbarThumb);
    brushes_["scrollbarThumbHover"] = CreateSolidBrush(currentColors_.scrollbarThumbHover);
}

void ThemeManager::CreatePens() {
    pens_["border"] = CreatePen(PS_SOLID, 1, currentColors_.border);
    pens_["borderLight"] = CreatePen(PS_SOLID, 1, currentColors_.borderLight);
    pens_["borderDark"] = CreatePen(PS_SOLID, 1, currentColors_.borderDark);
    pens_["pluginBorder"] = CreatePen(PS_SOLID, 1, currentColors_.pluginBorder);
    pens_["highlight"] = CreatePen(PS_SOLID, 2, currentColors_.highlight);
}

void ThemeManager::DestroyBrushes() {
    for (auto& pair : brushes_) {
        if (pair.second) {
            DeleteObject(pair.second);
        }
    }
    brushes_.clear();
}

void ThemeManager::DestroyPens() {
    for (auto& pair : pens_) {
        if (pair.second) {
            DeleteObject(pair.second);
        }
    }
    pens_.clear();
}

COLORREF ThemeManager::GetColor(const std::string& colorName) const {
    if (colorName == "windowBackground") return currentColors_.windowBackground;
    if (colorName == "panelBackground") return currentColors_.panelBackground;
    if (colorName == "controlBackground") return currentColors_.controlBackground;
    if (colorName == "textPrimary") return currentColors_.textPrimary;
    if (colorName == "textSecondary") return currentColors_.textSecondary;
    if (colorName == "textDisabled") return currentColors_.textDisabled;
    if (colorName == "border") return currentColors_.border;
    if (colorName == "buttonFace") return currentColors_.buttonFace;
    if (colorName == "buttonText") return currentColors_.buttonText;
    if (colorName == "pluginHeader") return currentColors_.pluginHeader;
    if (colorName == "pluginHeaderText") return currentColors_.pluginHeaderText;
    if (colorName == "selectionBackground") return currentColors_.selectionBackground;
    
    return currentColors_.textPrimary;  // Default
}

HBRUSH ThemeManager::GetBrush(const std::string& colorName) {
    auto it = brushes_.find(colorName);
    if (it != brushes_.end()) {
        return it->second;
    }
    return GetBackgroundBrush();  // Fallback
}

HPEN ThemeManager::GetPen(const std::string& colorName, int width) {
    auto it = pens_.find(colorName);
    if (it != pens_.end() && width == 1) {
        return it->second;
    }
    
    // Create on-demand pen with custom width
    COLORREF color = GetColor(colorName);
    return CreatePen(PS_SOLID, width, color);
}

bool ThemeManager::IsSystemDarkMode() const {
    // Check Windows 10/11 dark mode setting
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(value);
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueEx(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                       reinterpret_cast<LPBYTE>(&value), &size);
        RegCloseKey(hKey);
        return value == 0;  // 0 = dark mode, 1 = light mode
    }
    
    return false;  // Default to light if can't determine
}

void ThemeManager::UpdateFromSystem() {
    if (currentTheme_ == ThemeType::System) {
        UpdateColors();
        CreateBrushes();
        CreatePens();
        ApplyToAllWindows();
    }
}

void ThemeManager::LoadFromConfig() {
    ConfigManager config;
    if (!config.Load()) {
        // Config doesn't exist yet, use default
        SetTheme(ThemeType::System);
        return;
    }
    
    std::string themeStr = config.GetString("ui.theme", "system");
    
    if (themeStr == "light") {
        SetTheme(ThemeType::Light);
    } else if (themeStr == "dark") {
        SetTheme(ThemeType::Dark);
    } else {
        SetTheme(ThemeType::System);
    }
}

void ThemeManager::SaveToConfig() {
    ConfigManager config;
    config.Load();  // Load existing config first
    
    std::string themeStr;
    
    switch (currentTheme_) {
    case ThemeType::Light:
        themeStr = "light";
        break;
    case ThemeType::Dark:
        themeStr = "dark";
        break;
    default:
        themeStr = "system";
        break;
    }
    
    config.SetString("ui.theme", themeStr);
    config.Save();
}

void ThemeManager::ApplyToWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    
    // Enable dark mode for title bar on Windows 10/11
    if (currentColors_.windowBackground == darkTheme_.windowBackground) {
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode)); // DWMWA_USE_IMMERSIVE_DARK_MODE
    } else {
        BOOL useDarkMode = FALSE;
        DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
    }
    
    // Track window for future updates
    if (std::find(trackedWindows_.begin(), trackedWindows_.end(), hwnd) == trackedWindows_.end()) {
        trackedWindows_.push_back(hwnd);
    }
    
    // Force redraw
    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
    
    // Apply to child windows
    EnumChildWindows(hwnd, [](HWND child, LPARAM lParam) -> BOOL {
        InvalidateRect(child, nullptr, TRUE);
        return TRUE;
    }, 0);
}

void ThemeManager::ApplyToAllWindows() {
    for (HWND hwnd : trackedWindows_) {
        if (IsWindow(hwnd)) {
            ApplyToWindow(hwnd);
        }
    }
    
    // Remove invalid windows
    trackedWindows_.erase(
        std::remove_if(trackedWindows_.begin(), trackedWindows_.end(),
                      [](HWND hwnd) { return !IsWindow(hwnd); }),
        trackedWindows_.end()
    );
}

} // namespace violet
