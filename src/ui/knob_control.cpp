#include "violet/knob_control.h"
#include "violet/theme.h"
#include "violet/dpi_scaling.h"
#include <windowsx.h>
#include <commctrl.h>
#include <cmath>
#include <algorithm>

#ifndef TB_ENDTRACK
#define TB_ENDTRACK 8
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace violet {

const wchar_t* KnobControl::CLASS_NAME = L"VioletKnobControl";

KnobControl::KnobControl()
    : hwnd_(nullptr)
    , hInstance_(nullptr)
    , minValue_(0.0f)
    , maxValue_(1.0f)
    , value_(0.0f)
    , isDragging_(false)
    , dragStartY_(0)
    , dragStartValue_(0.0f)
    , size_(50) {
}

KnobControl::~KnobControl() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
}

bool KnobControl::RegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = CLASS_NAME;
    
    return RegisterClassEx(&wc) != 0;
}

HWND KnobControl::Create(HWND parent, HINSTANCE hInstance, int x, int y, int size, int id) {
    hInstance_ = hInstance;
    size_ = size;
    
    hwnd_ = CreateWindowEx(
        0, CLASS_NAME, L"",
        WS_CHILD | WS_VISIBLE,
        x, y, size, size,
        parent, (HMENU)(INT_PTR)id, hInstance, this
    );

    if (hwnd_) {
        HRGN region = CreateEllipticRgn(0, 0, size, size);
        if (region) {
            if (!SetWindowRgn(hwnd_, region, TRUE)) {
                DeleteObject(region);
            }
        }
    }
    
    return hwnd_;
}

void KnobControl::SetRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
    value_ = std::max(min, std::min(max, value_));
    
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void KnobControl::SetValue(float value) {
    value_ = std::max(minValue_, std::min(maxValue_, value));
    
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

float KnobControl::PixelToValue(int pixelDelta) {
    // Vertical drag sensitivity: 100 pixels = full range
    float range = maxValue_ - minValue_;
    return -pixelDelta * range / 100.0f;
}

float KnobControl::ValueToAngle(float value) {
    // Map value to angle (270 degrees of rotation, starting at 135 degrees)
    float normalized = (value - minValue_) / (maxValue_ - minValue_);
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    return 135.0f + normalized * 270.0f;
}

float KnobControl::AngleToValue(float angle) {
    // Map angle back to value
    float normalized = (angle - 135.0f) / 270.0f;
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    return minValue_ + normalized * (maxValue_ - minValue_);
}

LRESULT CALLBACK KnobControl::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    KnobControl* knob = nullptr;
    
    if (uMsg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        knob = reinterpret_cast<KnobControl*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(knob));
    } else {
        knob = reinterpret_cast<KnobControl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (knob) {
        return knob->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT KnobControl::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT:
        OnPaint();
        return 0;
        
    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
        
    case WM_LBUTTONUP:
        OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
        
    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
        
    case WM_MOUSEWHEEL:
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
        
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void KnobControl::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    
    DrawKnob(hdc);
    
    EndPaint(hwnd_, &ps);
}

void KnobControl::DrawKnob(HDC hdc) {
    const auto& colors = Theme::Instance().GetColors();
    
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    // Fill background using circular region so the control stays round
    HRGN clipRegion = CreateEllipticRgn(rect.left, rect.top, rect.right, rect.bottom);
    if (clipRegion) {
        HBRUSH bgBrush = CreateSolidBrush(colors.background);
        FillRgn(hdc, clipRegion, bgBrush);
        DeleteObject(bgBrush);
        DeleteObject(clipRegion);
    }
    
    // Calculate knob circle
    int centerX = (rect.right - rect.left) / 2;
    int centerY = (rect.bottom - rect.top) / 2;
    int radius = std::min(centerX, centerY) - DPI_SCALE(4);
    
    // Draw outer circle (track)
    HBRUSH trackBrush = CreateSolidBrush(colors.surfaceVariant);
    HPEN trackPen = CreatePen(PS_SOLID, DPI_SCALE(2), colors.border);
    HGDIOBJ oldBrush = SelectObject(hdc, trackBrush);
    HGDIOBJ oldPen = SelectObject(hdc, trackPen);
    
    Ellipse(hdc, 
        centerX - radius, centerY - radius,
        centerX + radius, centerY + radius);
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(trackBrush);
    DeleteObject(trackPen);
    
    // Draw value indicator (arc)
    float angle = ValueToAngle(value_);
    float angleRad = angle * M_PI / 180.0f;
    
    // Draw indicator line
    int indicatorRadius = radius - DPI_SCALE(8);
    int indicatorX = centerX + (int)(indicatorRadius * cos(angleRad));
    int indicatorY = centerY + (int)(indicatorRadius * sin(angleRad));
    
    HPEN indicatorPen = CreatePen(PS_SOLID, DPI_SCALE(3), colors.primary);
    oldPen = SelectObject(hdc, indicatorPen);
    
    MoveToEx(hdc, centerX, centerY, nullptr);
    LineTo(hdc, indicatorX, indicatorY);
    
    SelectObject(hdc, oldPen);
    DeleteObject(indicatorPen);
    
    // Draw center dot
    int dotRadius = DPI_SCALE(4);
    HBRUSH dotBrush = CreateSolidBrush(colors.primary);
    HPEN dotPen = CreatePen(PS_SOLID, 1, colors.primary);
    oldBrush = SelectObject(hdc, dotBrush);
    oldPen = SelectObject(hdc, dotPen);
    
    Ellipse(hdc,
        centerX - dotRadius, centerY - dotRadius,
        centerX + dotRadius, centerY + dotRadius);
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(dotBrush);
    DeleteObject(dotPen);
}

void KnobControl::OnLButtonDown(int x, int y) {
    isDragging_ = true;
    dragStartY_ = y;
    dragStartValue_ = value_;
    SetCapture(hwnd_);
}

void KnobControl::OnLButtonUp(int x, int y) {
    if (isDragging_) {
        isDragging_ = false;
        ReleaseCapture();
        
        // Notify parent that dragging has ended
        HWND parent = GetParent(hwnd_);
        if (parent) {
            // Send WM_HSCROLL with TB_ENDTRACK to signal end of interaction
            SendMessage(parent, WM_HSCROLL, 
                MAKEWPARAM(TB_ENDTRACK, 0), 
                (LPARAM)hwnd_);
        }
    }
}

void KnobControl::OnMouseMove(int x, int y) {
    if (isDragging_) {
        int deltaY = y - dragStartY_;
        float deltaValue = PixelToValue(deltaY);
        
        value_ = dragStartValue_ + deltaValue;
        value_ = std::max(minValue_, std::min(maxValue_, value_));
        
        InvalidateRect(hwnd_, nullptr, TRUE);
        
        // Notify parent of value change (continuous)
        HWND parent = GetParent(hwnd_);
        if (parent) {
            SendMessage(parent, WM_HSCROLL, 
                MAKEWPARAM(0, 0), 
                (LPARAM)hwnd_);
        }
    }
}

void KnobControl::OnMouseWheel(int delta) {
    // Scroll wheel adjustment
    float range = maxValue_ - minValue_;
    float step = range * 0.01f; // 1% per wheel notch
    
    value_ += (delta > 0 ? step : -step);
    value_ = std::max(minValue_, std::min(maxValue_, value_));
    
    InvalidateRect(hwnd_, nullptr, TRUE);
    
    // Notify parent
    HWND parent = GetParent(hwnd_);
    if (parent) {
        SendMessage(parent, WM_HSCROLL, 
            MAKEWPARAM(0, 0), 
            (LPARAM)hwnd_);
    }
}

} // namespace violet
