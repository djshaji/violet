#include "violet/dpi_scaling.h"
#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")

namespace violet {

DpiScaling::DpiScaling()
    : systemDpi_(96)
    , perMonitorAware_(false)
    , getDpiForWindow_(nullptr)
    , setProcessDpiAwarenessContext_(nullptr)
    , adjustWindowRectExForDpi_(nullptr) {
}

DpiScaling& DpiScaling::Instance() {
    static DpiScaling instance;
    return instance;
}

void DpiScaling::Initialize(HWND hwnd) {
    // Try to load Windows 10+ DPI functions
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        getDpiForWindow_ = reinterpret_cast<GetDpiForWindowFunc>(
            GetProcAddress(user32, "GetDpiForWindow"));
        setProcessDpiAwarenessContext_ = reinterpret_cast<SetProcessDpiAwarenessContextFunc>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        adjustWindowRectExForDpi_ = reinterpret_cast<AdjustWindowRectExForDpiFunc>(
            GetProcAddress(user32, "AdjustWindowRectExForDpi"));
    }
    
    // Set process DPI awareness
    if (setProcessDpiAwarenessContext_) {
        // Windows 10 1703+: Per-monitor V2 (best)
        if (setProcessDpiAwarenessContext_(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            perMonitorAware_ = true;
        } else if (setProcessDpiAwarenessContext_(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) {
            perMonitorAware_ = true;
        } else {
            setProcessDpiAwarenessContext_(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
        }
    } else {
        // Fallback to Windows 8.1 API
        HMODULE shcore = LoadLibraryW(L"shcore.dll");
        if (shcore) {
            typedef HRESULT(WINAPI* SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);
            auto setProcessDpiAwareness = reinterpret_cast<SetProcessDpiAwarenessFunc>(
                GetProcAddress(shcore, "SetProcessDpiAwareness"));
            
            if (setProcessDpiAwareness) {
                if (SUCCEEDED(setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE))) {
                    perMonitorAware_ = true;
                } else {
                    setProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
                }
            }
        }
    }
    
    // Get system DPI
    if (hwnd && getDpiForWindow_) {
        systemDpi_ = getDpiForWindow_(hwnd);
    } else {
        HDC screen = GetDC(nullptr);
        if (screen) {
            systemDpi_ = GetDeviceCaps(screen, LOGPIXELSX);
            ReleaseDC(nullptr, screen);
        }
    }
}

UINT DpiScaling::GetDpiForWindow(HWND hwnd) const {
    if (hwnd && getDpiForWindow_) {
        return getDpiForWindow_(hwnd);
    }
    return systemDpi_;
}

int DpiScaling::Scale(int value, HWND hwnd) const {
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(value, dpi, 96);
}

float DpiScaling::ScaleF(float value, HWND hwnd) const {
    UINT dpi = GetDpiForWindow(hwnd);
    return value * dpi / 96.0f;
}

int DpiScaling::Unscale(int value, HWND hwnd) const {
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(value, 96, dpi);
}

float DpiScaling::UnscaleF(float value, HWND hwnd) const {
    UINT dpi = GetDpiForWindow(hwnd);
    return value * 96.0f / dpi;
}

float DpiScaling::GetScaleFactor(HWND hwnd) const {
    UINT dpi = GetDpiForWindow(hwnd);
    return dpi / 96.0f;
}

void DpiScaling::OnDpiChanged(HWND hwnd, UINT dpi, const RECT* rect) {
    if (rect) {
        SetWindowPos(hwnd, nullptr,
            rect->left, rect->top,
            rect->right - rect->left,
            rect->bottom - rect->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void DpiScaling::ScaleLogFont(LOGFONT& lf, HWND hwnd) const {
    lf.lfHeight = Scale(lf.lfHeight, hwnd);
    lf.lfWidth = Scale(lf.lfWidth, hwnd);
}

} // namespace violet
