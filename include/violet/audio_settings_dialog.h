#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace violet {

class AudioEngine;
struct AudioDevice;

class AudioSettingsDialog {
public:
    AudioSettingsDialog();
    ~AudioSettingsDialog();
    
    // Show the dialog modally
    // Returns true if user clicked OK, false if cancelled
    bool Show(HWND parentWindow, AudioEngine* audioEngine);
    
private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Message handlers
    void OnInitDialog(HWND hwnd);
    void OnCommand(WPARAM wParam);
    void OnOK();
    void OnCancel();
    
    // UI updates
    void PopulateDeviceLists();
    void PopulateSampleRates();
    void PopulateBufferSizes();
    void UpdateCurrentSettings();
    void SelectDeviceInList(HWND listbox, const std::string& deviceId, bool isInput);
    
    // Apply settings
    bool ApplySettings();
    
    // Error handling
    void ShowError(const std::wstring& message);
    
    // Dialog window
    HWND hwnd_;
    HWND parentWindow_;
    AudioEngine* audioEngine_;
    
    // Control handles
    HWND hInputDeviceList_;
    HWND hOutputDeviceList_;
    HWND hSampleRateCombo_;
    HWND hBufferSizeCombo_;
    
    // Device lists
    std::vector<AudioDevice> inputDevices_;
    std::vector<AudioDevice> outputDevices_;
    
    // Selected settings
    std::string selectedInputDeviceId_;
    std::string selectedOutputDeviceId_;
    uint32_t selectedSampleRate_;
    uint32_t selectedBufferSize_;
    
    // Control IDs
    static constexpr int IDC_INPUT_DEVICE_LIST = 1001;
    static constexpr int IDC_OUTPUT_DEVICE_LIST = 1002;
    static constexpr int IDC_SAMPLE_RATE_COMBO = 1003;
    static constexpr int IDC_BUFFER_SIZE_COMBO = 1004;
    static constexpr int IDC_OK_BUTTON = IDOK;
    static constexpr int IDC_CANCEL_BUTTON = IDCANCEL;
    
    // Dialog result
    bool dialogResult_;
};

} // namespace violet
