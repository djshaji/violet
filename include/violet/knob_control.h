#pragma once

#include <windows.h>
#include <string>

namespace violet {

// Custom rotary knob control for parameter adjustment
class KnobControl {
public:
    KnobControl();
    ~KnobControl();
    
    // Create the knob window
    static bool RegisterClass(HINSTANCE hInstance);
    HWND Create(HWND parent, HINSTANCE hInstance, int x, int y, int size, int id);
    
    // Value management
    void SetRange(float min, float max);
    void SetValue(float value);
    float GetValue() const { return value_; }
    
    // Get window handle
    HWND GetHandle() const { return hwnd_; }
    
private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnPaint();
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int delta);
    
    // Drawing
    void DrawKnob(HDC hdc);
    
    // Helper functions
    float PixelToValue(int pixelDelta);
    float AngleToValue(float angle);
    float ValueToAngle(float value);
    
    HWND hwnd_;
    HINSTANCE hInstance_;
    
    // Value range
    float minValue_;
    float maxValue_;
    float value_;
    
    // Mouse tracking
    bool isDragging_;
    int dragStartY_;
    float dragStartValue_;
    
    // Appearance
    int size_;
    
    // Class name
    static const wchar_t* CLASS_NAME;
};

} // namespace violet
