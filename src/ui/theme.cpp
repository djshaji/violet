#include "violet/theme.h"
#include <algorithm>

namespace violet {

Theme::Theme()
    : mode_(ThemeMode::Dark)
    , currentColors_(nullptr)
    , backgroundBrush_(nullptr)
    , surfaceBrush_(nullptr)
    , primaryBrush_(nullptr)
    , borderPen_(nullptr)
    , borderHoverPen_(nullptr) {
    UpdateTheme();
}

Theme::~Theme() {
    DestroyResources();
}

Theme& Theme::Instance() {
    static Theme instance;
    return instance;
}

void Theme::SetMode(ThemeMode mode) {
    if (mode_ != mode) {
        mode_ = mode;
        UpdateTheme();
    }
}

ColorScheme Theme::LightTheme() {
    ColorScheme colors;
    colors.background = RGB(250, 250, 250);      // Almost white
    colors.surface = RGB(255, 255, 255);         // Pure white
    colors.surfaceVariant = RGB(245, 245, 245);  // Light gray
    colors.primary = RGB(98, 0, 238);            // Purple
    colors.primaryVariant = RGB(123, 31, 252);   // Lighter purple
    colors.secondary = RGB(3, 218, 198);         // Teal
    colors.onBackground = RGB(33, 33, 33);       // Almost black
    colors.onSurface = RGB(33, 33, 33);          // Almost black
    colors.onPrimary = RGB(255, 255, 255);       // White
    colors.border = RGB(224, 224, 224);          // Light gray
    colors.borderHover = RGB(189, 189, 189);     // Medium gray
    colors.shadow = RGB(0, 0, 0);                // Black (with alpha)
    colors.accent = RGB(98, 0, 238);             // Purple (same as primary)
    colors.error = RGB(211, 47, 47);             // Red
    colors.success = RGB(56, 142, 60);           // Green
    colors.warning = RGB(245, 124, 0);           // Orange
    return colors;
}

ColorScheme Theme::DarkTheme() {
    ColorScheme colors;
    colors.background = RGB(180, 180, 180);         // Very dark gray
    colors.surface = RGB(30, 30, 30);            // Dark gray
    colors.surfaceVariant = RGB(42, 42, 42);     // Lighter dark gray
    colors.primary = RGB(255, 0, 0);         // Light purple
    colors.primaryVariant = RGB(208, 170, 255);  // Lighter purple
    colors.secondary = RGB(3, 218, 198);         // Teal
    colors.onBackground = RGB(230, 230, 230);    // Light gray
    colors.onSurface = RGB(230, 230, 230);       // Light gray
    colors.onPrimary = RGB(18, 18, 18);          // Dark gray
    colors.border = RGB(60, 60, 60);             // Medium dark gray
    colors.borderHover = RGB(90, 90, 90);        // Lighter gray
    colors.shadow = RGB(0, 0, 0);                // Black (with alpha)
    colors.accent = RGB(255, 0, 0);          // Light purple (same as primary)
    colors.error = RGB(239, 83, 80);             // Light red
    colors.success = RGB(102, 187, 106);         // Light green
    colors.warning = RGB(255, 167, 38);          // Light orange
    return colors;
}

void Theme::UpdateTheme() {
    DestroyResources();
    
    // Determine which color scheme to use
    static ColorScheme lightColors = LightTheme();
    static ColorScheme darkColors = DarkTheme();
    
    switch (mode_) {
        case ThemeMode::Light:
            currentColors_ = &lightColors;
            break;
        case ThemeMode::Dark:
            currentColors_ = &darkColors;
            break;
        case ThemeMode::System:
            // TODO: Query Windows system theme preference
            currentColors_ = &darkColors; // Default to dark for now
            break;
    }
    
    CreateResources();
}

void Theme::CreateResources() {
    if (!currentColors_) return;
    
    // Create brushes
    backgroundBrush_ = CreateSolidBrush(currentColors_->background);
    surfaceBrush_ = CreateSolidBrush(currentColors_->surface);
    primaryBrush_ = CreateSolidBrush(currentColors_->primary);
    
    // Create pens
    borderPen_ = CreatePen(PS_SOLID, 1, currentColors_->border);
    borderHoverPen_ = CreatePen(PS_SOLID, 1, currentColors_->borderHover);
}

void Theme::DestroyResources() {
    if (backgroundBrush_) {
        DeleteObject(backgroundBrush_);
        backgroundBrush_ = nullptr;
    }
    if (surfaceBrush_) {
        DeleteObject(surfaceBrush_);
        surfaceBrush_ = nullptr;
    }
    if (primaryBrush_) {
        DeleteObject(primaryBrush_);
        primaryBrush_ = nullptr;
    }
    if (borderPen_) {
        DeleteObject(borderPen_);
        borderPen_ = nullptr;
    }
    if (borderHoverPen_) {
        DeleteObject(borderHoverPen_);
        borderHoverPen_ = nullptr;
    }
}

HFONT Theme::CreateScaledFont(int baseSize, int weight, bool italic) const {
    LOGFONT lf = {};
    lf.lfHeight = -baseSize;
    lf.lfWeight = weight;
    lf.lfItalic = italic ? TRUE : FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    
    return CreateFontIndirect(&lf);
}

} // namespace violet
