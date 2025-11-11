#include "violet/audio_settings_dialog.h"
#include "violet/audio_engine.h"
#include "violet/utils.h"
#include <commctrl.h>
#include <windowsx.h>

namespace violet {

AudioSettingsDialog::AudioSettingsDialog()
    : hwnd_(nullptr)
    , parentWindow_(nullptr)
    , audioEngine_(nullptr)
    , hInputDeviceList_(nullptr)
    , hOutputDeviceList_(nullptr)
    , hSampleRateCombo_(nullptr)
    , hBufferSizeCombo_(nullptr)
    , selectedSampleRate_(44100)
    , selectedBufferSize_(256)
    , dialogResult_(false) {
}

AudioSettingsDialog::~AudioSettingsDialog() {
}

bool AudioSettingsDialog::Show(HWND parentWindow, AudioEngine* audioEngine) {
    parentWindow_ = parentWindow;
    audioEngine_ = audioEngine;
    dialogResult_ = false;
    
    if (!audioEngine_) {
        return false;
    }
    
    // Register window class for dialog
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = StaticDialogProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"AudioSettingsDialog";
    
    static bool classRegistered = false;
    if (!classRegistered) {
        if (!RegisterClassEx(&wc)) {
            MessageBox(parentWindow, L"Failed to register dialog class", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        classRegistered = true;
    }
    
    // Create the dialog window
    hwnd_ = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
        L"AudioSettingsDialog",
        L"Audio Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 400,
        parentWindow,
        nullptr,
        GetModuleHandle(nullptr),
        this  // Pass this pointer via CreateParams
    );
    
    if (!hwnd_) {
        DWORD error = GetLastError();
        wchar_t buf[256];
        wsprintf(buf, L"Failed to create dialog window. Error: %d", error);
        MessageBox(parentWindow, buf, L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    // Create controls
    OnInitDialog(hwnd_);
    
    // Center dialog
    RECT rcParent, rcDialog;
    GetWindowRect(parentWindow, &rcParent);
    GetWindowRect(hwnd_, &rcDialog);
    int x = rcParent.left + (rcParent.right - rcParent.left - (rcDialog.right - rcDialog.left)) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDialog.bottom - rcDialog.top)) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    // Show the window
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    
    // Run modal message loop
    MSG msg;
    EnableWindow(parentWindow, FALSE);
    
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsWindow(hwnd_)) {
            break;
        }
        
        if (!IsDialogMessage(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(parentWindow, TRUE);
    SetForegroundWindow(parentWindow);
    
    return dialogResult_;
}

void AudioSettingsDialog::OnInitDialog(HWND hwnd) {
    hwnd_ = hwnd;
    
    // Create labels and controls
    int yPos = 20;
    int labelWidth = 120;
    int controlWidth = 340;
    int controlHeight = 24;
    int spacing = 10;
    
    // Input Device section
    CreateWindow(L"STATIC", L"Input Device:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        10, yPos, labelWidth, controlHeight,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hInputDeviceList_ = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"LISTBOX",
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        labelWidth + 20, yPos, controlWidth, 80,
        hwnd_, (HMENU)IDC_INPUT_DEVICE_LIST, GetModuleHandle(nullptr), nullptr);
    
    yPos += 80 + spacing;
    
    // Output Device section
    CreateWindow(L"STATIC", L"Output Device:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        10, yPos, labelWidth, controlHeight,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hOutputDeviceList_ = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"LISTBOX",
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        labelWidth + 20, yPos, controlWidth, 80,
        hwnd_, (HMENU)IDC_OUTPUT_DEVICE_LIST, GetModuleHandle(nullptr), nullptr);
    
    yPos += 80 + spacing;
    
    // Sample Rate section
    CreateWindow(L"STATIC", L"Sample Rate:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        10, yPos, labelWidth, controlHeight,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hSampleRateCombo_ = CreateWindow(
        L"COMBOBOX",
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        labelWidth + 20, yPos, 150, 200,
        hwnd_, (HMENU)IDC_SAMPLE_RATE_COMBO, GetModuleHandle(nullptr), nullptr);
    
    yPos += controlHeight + spacing;
    
    // Buffer Size section
    CreateWindow(L"STATIC", L"Buffer Size:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        10, yPos, labelWidth, controlHeight,
        hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
    
    hBufferSizeCombo_ = CreateWindow(
        L"COMBOBOX",
        nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        labelWidth + 20, yPos, 150, 200,
        hwnd_, (HMENU)IDC_BUFFER_SIZE_COMBO, GetModuleHandle(nullptr), nullptr);
    
    yPos += controlHeight + spacing * 2;
    
    // OK and Cancel buttons
    CreateWindow(L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        labelWidth + 20 + controlWidth - 180, yPos, 80, 30,
        hwnd_, (HMENU)IDC_OK_BUTTON, GetModuleHandle(nullptr), nullptr);
    
    CreateWindow(L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        labelWidth + 20 + controlWidth - 90, yPos, 80, 30,
        hwnd_, (HMENU)IDC_CANCEL_BUTTON, GetModuleHandle(nullptr), nullptr);
    
    // Populate controls
    PopulateDeviceLists();
    PopulateSampleRates();
    PopulateBufferSizes();
    UpdateCurrentSettings();
}

void AudioSettingsDialog::PopulateDeviceLists() {
    if (!audioEngine_) {
        return;
    }
    
    // Clear existing items
    SendMessage(hInputDeviceList_, LB_RESETCONTENT, 0, 0);
    SendMessage(hOutputDeviceList_, LB_RESETCONTENT, 0, 0);
    
    inputDevices_.clear();
    outputDevices_.clear();
    
    // Get all available devices
    std::vector<AudioDevice> allDevices = audioEngine_->EnumerateDevices();
    
    // Separate input and output devices
    for (const auto& device : allDevices) {
        if (device.isInput) {
            inputDevices_.push_back(device);
            std::wstring displayName = utils::StringToWString(device.name);
            if (device.isDefault) {
                displayName += L" (Default)";
            }
            int index = SendMessage(hInputDeviceList_, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());
            SendMessage(hInputDeviceList_, LB_SETITEMDATA, index, (LPARAM)inputDevices_.size() - 1);
        }
        
        if (device.isOutput) {
            outputDevices_.push_back(device);
            std::wstring displayName = utils::StringToWString(device.name);
            if (device.isDefault) {
                displayName += L" (Default)";
            }
            int index = SendMessage(hOutputDeviceList_, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());
            SendMessage(hOutputDeviceList_, LB_SETITEMDATA, index, (LPARAM)outputDevices_.size() - 1);
        }
    }
}

void AudioSettingsDialog::PopulateSampleRates() {
    if (!audioEngine_) {
        return;
    }
    
    SendMessage(hSampleRateCombo_, CB_RESETCONTENT, 0, 0);
    
    std::vector<uint32_t> sampleRates = audioEngine_->GetSupportedSampleRates();
    for (uint32_t sampleRate : sampleRates) {
        std::wstring text = std::to_wstring(sampleRate) + L" Hz";
        int index = SendMessage(hSampleRateCombo_, CB_ADDSTRING, 0, (LPARAM)text.c_str());
        SendMessage(hSampleRateCombo_, CB_SETITEMDATA, index, (LPARAM)sampleRate);
    }
}

void AudioSettingsDialog::PopulateBufferSizes() {
    if (!audioEngine_) {
        return;
    }
    
    SendMessage(hBufferSizeCombo_, CB_RESETCONTENT, 0, 0);
    
    std::vector<uint32_t> bufferSizes = audioEngine_->GetSupportedBufferSizes();
    for (uint32_t bufferSize : bufferSizes) {
        std::wstring text = std::to_wstring(bufferSize) + L" samples";
        int index = SendMessage(hBufferSizeCombo_, CB_ADDSTRING, 0, (LPARAM)text.c_str());
        SendMessage(hBufferSizeCombo_, CB_SETITEMDATA, index, (LPARAM)bufferSize);
    }
}

void AudioSettingsDialog::UpdateCurrentSettings() {
    if (!audioEngine_) {
        return;
    }
    
    // Get current settings
    std::string currentInputId = audioEngine_->GetCurrentInputDevice();
    std::string currentOutputId = audioEngine_->GetCurrentOutputDevice();
    AudioFormat currentFormat = audioEngine_->GetFormat();
    
    // Select current input device
    SelectDeviceInList(hInputDeviceList_, currentInputId, true);
    
    // Select current output device
    SelectDeviceInList(hOutputDeviceList_, currentOutputId, false);
    
    // Select current sample rate
    int count = SendMessage(hSampleRateCombo_, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i) {
        uint32_t sampleRate = (uint32_t)SendMessage(hSampleRateCombo_, CB_GETITEMDATA, i, 0);
        if (sampleRate == currentFormat.sampleRate) {
            SendMessage(hSampleRateCombo_, CB_SETCURSEL, i, 0);
            break;
        }
    }
    
    // Select current buffer size
    count = SendMessage(hBufferSizeCombo_, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i) {
        uint32_t bufferSize = (uint32_t)SendMessage(hBufferSizeCombo_, CB_GETITEMDATA, i, 0);
        if (bufferSize == currentFormat.bufferSize) {
            SendMessage(hBufferSizeCombo_, CB_SETCURSEL, i, 0);
            break;
        }
    }
}

void AudioSettingsDialog::SelectDeviceInList(HWND listbox, const std::string& deviceId, bool isInput) {
    const std::vector<AudioDevice>& devices = isInput ? inputDevices_ : outputDevices_;
    
    int count = SendMessage(listbox, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i) {
        int deviceIndex = (int)SendMessage(listbox, LB_GETITEMDATA, i, 0);
        if (deviceIndex >= 0 && deviceIndex < (int)devices.size()) {
            if (devices[deviceIndex].id == deviceId) {
                SendMessage(listbox, LB_SETCURSEL, i, 0);
                return;
            }
        }
    }
    
    // If not found, select first item (usually default)
    if (count > 0) {
        SendMessage(listbox, LB_SETCURSEL, 0, 0);
    }
}

LRESULT CALLBACK AudioSettingsDialog::StaticDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AudioSettingsDialog* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        // Get the this pointer from CreateParams
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<AudioSettingsDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<AudioSettingsDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT AudioSettingsDialog::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        OnCommand(wParam);
        return 0;
        
    case WM_CLOSE:
        OnCancel();
        return 0;
        
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void AudioSettingsDialog::OnCommand(WPARAM wParam) {
    int wmId = LOWORD(wParam);
    
    switch (wmId) {
    case IDC_OK_BUTTON:
        OnOK();
        break;
        
    case IDC_CANCEL_BUTTON:
        OnCancel();
        break;
    }
}

void AudioSettingsDialog::OnOK() {
    if (ApplySettings()) {
        dialogResult_ = true;
        DestroyWindow(hwnd_);
    }
}

void AudioSettingsDialog::OnCancel() {
    dialogResult_ = false;
    DestroyWindow(hwnd_);
}

bool AudioSettingsDialog::ApplySettings() {
    if (!audioEngine_) {
        return false;
    }
    
    // Get selected input device
    int inputSel = SendMessage(hInputDeviceList_, LB_GETCURSEL, 0, 0);
    if (inputSel != LB_ERR) {
        int deviceIndex = (int)SendMessage(hInputDeviceList_, LB_GETITEMDATA, inputSel, 0);
        if (deviceIndex >= 0 && deviceIndex < (int)inputDevices_.size()) {
            selectedInputDeviceId_ = inputDevices_[deviceIndex].id;
        }
    }
    
    // Get selected output device
    int outputSel = SendMessage(hOutputDeviceList_, LB_GETCURSEL, 0, 0);
    if (outputSel != LB_ERR) {
        int deviceIndex = (int)SendMessage(hOutputDeviceList_, LB_GETITEMDATA, outputSel, 0);
        if (deviceIndex >= 0 && deviceIndex < (int)outputDevices_.size()) {
            selectedOutputDeviceId_ = outputDevices_[deviceIndex].id;
        }
    }
    
    // Get selected sample rate
    int sampleRateSel = SendMessage(hSampleRateCombo_, CB_GETCURSEL, 0, 0);
    if (sampleRateSel != CB_ERR) {
        selectedSampleRate_ = (uint32_t)SendMessage(hSampleRateCombo_, CB_GETITEMDATA, sampleRateSel, 0);
    }
    
    // Get selected buffer size
    int bufferSizeSel = SendMessage(hBufferSizeCombo_, CB_GETCURSEL, 0, 0);
    if (bufferSizeSel != CB_ERR) {
        selectedBufferSize_ = (uint32_t)SendMessage(hBufferSizeCombo_, CB_GETITEMDATA, bufferSizeSel, 0);
    }
    
    // Check if audio engine is running
    bool wasRunning = audioEngine_->IsRunning();
    
    // Stop audio engine if running
    if (wasRunning) {
        if (!audioEngine_->Stop()) {
            ShowError(L"Failed to stop audio engine");
            return false;
        }
    }
    
    // Apply device selections
    if (!selectedInputDeviceId_.empty()) {
        if (!audioEngine_->SetInputDevice(selectedInputDeviceId_)) {
            ShowError(L"Failed to set input device");
            if (wasRunning) {
                audioEngine_->Start(); // Try to restart with old settings
            }
            return false;
        }
    }
    
    if (!selectedOutputDeviceId_.empty()) {
        if (!audioEngine_->SetOutputDevice(selectedOutputDeviceId_)) {
            ShowError(L"Failed to set output device");
            if (wasRunning) {
                audioEngine_->Start(); // Try to restart with old settings
            }
            return false;
        }
    }
    
    // Apply format
    AudioFormat newFormat;
    newFormat.sampleRate = selectedSampleRate_;
    newFormat.channels = 2; // Stereo
    newFormat.bufferSize = selectedBufferSize_;
    newFormat.bitsPerSample = 32; // 32-bit float
    
    if (!audioEngine_->SetFormat(newFormat)) {
        ShowError(L"Failed to set audio format");
        if (wasRunning) {
            audioEngine_->Start(); // Try to restart with old settings
        }
        return false;
    }
    
    // Restart audio engine if it was running
    if (wasRunning) {
        if (!audioEngine_->Start()) {
            ShowError(L"Failed to restart audio engine with new settings.\nPlease check the Audio menu to start manually.");
            return false;
        }
    }
    
    return true;
}

void AudioSettingsDialog::ShowError(const std::wstring& message) {
    MessageBox(hwnd_, message.c_str(), L"Audio Settings Error", MB_OK | MB_ICONERROR);
}

} // namespace violet
