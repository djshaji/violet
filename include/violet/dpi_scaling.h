#pragma once

#include <windows.h>

namespace violet {

class DpiScaling {
public:
    static DpiScaling& Instance();
    
    // Initialize DPI awareness
    void Initialize(HWND hwnd = nullptr);
    
    // Get current DPI
    UINT GetDpiForWindow(HWND hwnd) const;
    UINT GetSystemDpi() const { return systemDpi_; }
    
    // Scale values
    int Scale(int value, HWND hwnd = nullptr) const;
    float ScaleF(float value, HWND hwnd = nullptr) const;
    
    // Unscale values (useful for mouse coordinates)
    int Unscale(int value, HWND hwnd = nullptr) const;
    float UnscaleF(float value, HWND hwnd = nullptr) const;
    
    // Get scaling factor
    float GetScaleFactor(HWND hwnd = nullptr) const;
    
    // DPI change handling
    void OnDpiChanged(HWND hwnd, UINT dpi, const RECT* rect);
    
    // Helper for LOGFONT
    void ScaleLogFont(LOGFONT& lf, HWND hwnd = nullptr) const;
    
private:
    DpiScaling();
    ~DpiScaling() = default;
    DpiScaling(const DpiScaling&) = delete;
    DpiScaling& operator=(const DpiScaling&) = delete;
    
    UINT systemDpi_;
    bool perMonitorAware_;
    
    // Function pointers for Windows 10+ DPI APIs
    typedef UINT(WINAPI* GetDpiForWindowFunc)(HWND);
    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
    typedef BOOL(WINAPI* AdjustWindowRectExForDpiFunc)(LPRECT, DWORD, BOOL, DWORD, UINT);
    
    GetDpiForWindowFunc getDpiForWindow_;
    SetProcessDpiAwarenessContextFunc setProcessDpiAwarenessContext_;
    AdjustWindowRectExForDpiFunc adjustWindowRectExForDpi_;
};

// Convenience macros
#define DPI_SCALE(value) violet::DpiScaling::Instance().Scale(value)
#define DPI_SCALE_F(value) violet::DpiScaling::Instance().ScaleF(value)
#define DPI_SCALE_WINDOW(value, hwnd) violet::DpiScaling::Instance().Scale(value, hwnd)

} // namespace violet
