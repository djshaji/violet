#pragma once

#include <windows.h>

namespace violet {

class AboutDialog {
public:
    AboutDialog();
    ~AboutDialog();
    
    // Show the about dialog modally
    void Show(HWND parentWindow);
    
private:
    // Dialog procedure
    static LRESULT CALLBACK StaticDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnInitDialog(HWND hwnd);
    void OnCommand(WPARAM wParam);
    void OnPaint();
    
    // Drawing
    void DrawLogo(HDC hdc, const RECT& rect);
    void DrawAppText(HDC hdc, const RECT& rect);
    
    // Dialog window
    HWND hwnd_;
    HWND parentWindow_;
    
    // Control IDs
    static constexpr int IDC_OK_BUTTON = IDOK;
    static constexpr int IDC_LOGO_STATIC = 1001;
    
    // Application info
    static constexpr const wchar_t* APP_NAME = L"Violet";
    static constexpr const wchar_t* APP_VERSION = L"0.78";
    static constexpr const wchar_t* APP_COPYRIGHT = L"Copyright \u00a9 2025";
    static constexpr const wchar_t* APP_DESCRIPTION = L"Lightweight LV2 Plugin Host for Windows";
    static constexpr const wchar_t* APP_LICENSE = L"Released under the MIT License";
    static constexpr const wchar_t* APP_WEBSITE = L"https://github.com/djshaji/violet";
};

} // namespace violet
