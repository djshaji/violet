#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ks.h>
#include <ksmedia.h>

#include "violet/audio_engine.h"
#include "violet/utils.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace violet {

// Supported audio formats
const uint32_t AudioEngine::SUPPORTED_SAMPLE_RATES[] = {44100, 48000, 88200, 96000, 192000};
const uint32_t AudioEngine::SUPPORTED_BUFFER_SIZES[] = {64, 128, 256, 512, 1024, 2048};

AudioEngine::AudioEngine()
    : deviceEnumerator_(nullptr)
    , inputDevice_(nullptr)
    , outputDevice_(nullptr)
    , inputClient_(nullptr)
    , outputClient_(nullptr)
    , renderClient_(nullptr)
    , captureClient_(nullptr)
    , volumeControl_(nullptr)
    , isRunning_(false)
    , shouldStop_(false)
    , audioEvent_(nullptr)
    , inputEventCallbackMode_(false)
    , outputEventCallbackMode_(false)
    , audioCallback_(nullptr)
    , callbackUserData_(nullptr)
    , cpuUsage_(0.0)
    , dropoutCount_(0) {
}

AudioEngine::~AudioEngine() {
    Shutdown();
}

bool AudioEngine::Initialize() {
    std::cout << "Initializing audio engine..." << std::endl;
    
    if (!InitializeWASAPI()) {
        std::cerr << "Failed to initialize WASAPI" << std::endl;
        return false;
    }
    
    // Create audio processing event
    audioEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!audioEvent_) {
        std::cerr << "Failed to create audio event: " << GetLastError() << std::endl;
        Shutdown();
        return false;
    }
    
    std::cout << "Audio engine initialized successfully" << std::endl;
    return true;
}

void AudioEngine::Shutdown() {
    Stop();
    ShutdownWASAPI();
    
    if (audioEvent_) {
        CloseHandle(audioEvent_);
        audioEvent_ = nullptr;
    }
}

bool AudioEngine::InitializeWASAPI() {
    // Initialize COM for this thread if not already initialized
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        std::cerr << "Failed to initialize COM: 0x" << std::hex << hr << std::endl;
        // Continue anyway as COM might already be initialized
    }
    
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator_
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create device enumerator: 0x" << std::hex << hr << std::endl;
        if (hr == REGDB_E_CLASSNOTREG) {
            std::cerr << "MMDeviceEnumerator class not registered (Wine may need winepulse)" << std::endl;
        } else if (hr == CLASS_E_NOAGGREGATION) {
            std::cerr << "Class cannot be aggregated" << std::endl;
        } else if (hr == E_NOINTERFACE) {
            std::cerr << "Interface not supported" << std::endl;
        } else if (hr == CO_E_NOTINITIALIZED) {
            std::cerr << "COM not initialized" << std::endl;
        }
        return false;
    }
    
    std::cout << "WASAPI device enumerator created successfully" << std::endl;
    return true;
}

void AudioEngine::ShutdownWASAPI() {
    if (renderClient_) {
        renderClient_->Release();
        renderClient_ = nullptr;
    }
    
    if (captureClient_) {
        captureClient_->Release();
        captureClient_ = nullptr;
    }
    
    if (volumeControl_) {
        volumeControl_->Release();
        volumeControl_ = nullptr;
    }
    
    if (inputClient_) {
        inputClient_->Release();
        inputClient_ = nullptr;
        inputEventCallbackMode_ = false;
    }
    
    if (outputClient_) {
        outputClient_->Release();
        outputClient_ = nullptr;
        outputEventCallbackMode_ = false;
    }
    
    if (inputDevice_) {
        inputDevice_->Release();
        inputDevice_ = nullptr;
    }
    
    if (outputDevice_) {
        outputDevice_->Release();
        outputDevice_ = nullptr;
    }
    
    if (deviceEnumerator_) {
        deviceEnumerator_->Release();
        deviceEnumerator_ = nullptr;
    }
}

std::vector<AudioDevice> AudioEngine::EnumerateDevices() {
    std::vector<AudioDevice> devices;
    
    if (!deviceEnumerator_) {
        return devices;
    }
    
    // Enumerate output devices
    IMMDeviceCollection* outputCollection = nullptr;
    HRESULT hr = deviceEnumerator_->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &outputCollection);
    if (SUCCEEDED(hr)) {
        UINT count = 0;
        outputCollection->GetCount(&count);
        
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* device = nullptr;
            if (SUCCEEDED(outputCollection->Item(i, &device))) {
                AudioDevice audioDevice;
                audioDevice.id = GetDeviceId(device);
                audioDevice.name = GetDeviceName(device);
                audioDevice.isInput = false;
                audioDevice.isOutput = true;
                audioDevice.isDefault = false; // Will be set later
                
                devices.push_back(audioDevice);
                device->Release();
            }
        }
        outputCollection->Release();
    }
    
    // Enumerate input devices
    IMMDeviceCollection* inputCollection = nullptr;
    hr = deviceEnumerator_->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &inputCollection);
    if (SUCCEEDED(hr)) {
        UINT count = 0;
        inputCollection->GetCount(&count);
        
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* device = nullptr;
            if (SUCCEEDED(inputCollection->Item(i, &device))) {
                AudioDevice audioDevice;
                audioDevice.id = GetDeviceId(device);
                audioDevice.name = GetDeviceName(device);
                audioDevice.isInput = true;
                audioDevice.isOutput = false;
                audioDevice.isDefault = false; // Will be set later
                
                devices.push_back(audioDevice);
                device->Release();
            }
        }
        inputCollection->Release();
    }
    
    // Mark default devices
    IMMDevice* defaultOutput = nullptr;
    if (SUCCEEDED(deviceEnumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &defaultOutput))) {
        std::string defaultOutputId = GetDeviceId(defaultOutput);
        for (auto& device : devices) {
            if (device.id == defaultOutputId && device.isOutput) {
                device.isDefault = true;
                break;
            }
        }
        defaultOutput->Release();
    }
    
    IMMDevice* defaultInput = nullptr;
    if (SUCCEEDED(deviceEnumerator_->GetDefaultAudioEndpoint(eCapture, eConsole, &defaultInput))) {
        std::string defaultInputId = GetDeviceId(defaultInput);
        for (auto& device : devices) {
            if (device.id == defaultInputId && device.isInput) {
                device.isDefault = true;
                break;
            }
        }
        defaultInput->Release();
    }
    
    return devices;
}

std::string AudioEngine::GetDeviceId(IMMDevice* device) {
    LPWSTR deviceId = nullptr;
    if (SUCCEEDED(device->GetId(&deviceId))) {
        std::string result = utils::WStringToString(deviceId);
        CoTaskMemFree(deviceId);
        return result;
    }
    return "";
}

std::string AudioEngine::GetDeviceName(IMMDevice* device) {
    IPropertyStore* props = nullptr;
    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &props))) {
        PROPVARIANT varName;
        PropVariantInit(&varName);
        
        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName))) {
            std::string result = utils::WStringToString(varName.pwszVal);
            PropVariantClear(&varName);
            props->Release();
            return result;
        }
        
        PropVariantClear(&varName);
        props->Release();
    }
    return "Unknown Device";
}

bool AudioEngine::SetInputDevice(const std::string& deviceId) {
    return SelectDevice(deviceId, true);
}

bool AudioEngine::SetOutputDevice(const std::string& deviceId) {
    return SelectDevice(deviceId, false);
}

bool AudioEngine::SelectDevice(const std::string& deviceId, bool isInput) {
    if (!deviceEnumerator_) {
        std::cerr << "Device enumerator not initialized" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    IMMDevice** targetDevice = isInput ? &inputDevice_ : &outputDevice_;
    std::string* targetDeviceId = isInput ? &currentInputDeviceId_ : &currentOutputDeviceId_;
    
    // Release current device
    if (*targetDevice) {
        (*targetDevice)->Release();
        *targetDevice = nullptr;
    }
    
    // Get new device
    if (deviceId.empty()) {
        // Use default device
        EDataFlow flow = isInput ? eCapture : eRender;
        HRESULT hr = deviceEnumerator_->GetDefaultAudioEndpoint(flow, eConsole, targetDevice);
        if (FAILED(hr)) {
            std::cerr << "Failed to get default " << (isInput ? "input" : "output") 
                      << " device: 0x" << std::hex << hr << std::endl;
            if (hr == E_NOTFOUND) {
                std::cerr << "No " << (isInput ? "input" : "output") << " devices found" << std::endl;
            } else if (hr == E_OUTOFMEMORY) {
                std::cerr << "Out of memory" << std::endl;
            }
            return false;
        }
        *targetDeviceId = GetDeviceId(*targetDevice);
        std::cout << "Selected default " << (isInput ? "input" : "output") 
                  << " device: " << *targetDeviceId << std::endl;
    } else {
        // Use specific device
        std::wstring wDeviceId = utils::StringToWString(deviceId);
        HRESULT hr = deviceEnumerator_->GetDevice(wDeviceId.c_str(), targetDevice);
        if (FAILED(hr)) {
            std::cerr << "Failed to get device '" << deviceId << "': 0x" << std::hex << hr << std::endl;
            if (hr == E_NOTFOUND) {
                std::cerr << "Device not found" << std::endl;
            } else if (hr == E_INVALIDARG) {
                std::cerr << "Invalid device ID" << std::endl;
            }
            return false;
        }
        *targetDeviceId = deviceId;
        std::cout << "Selected " << (isInput ? "input" : "output") 
                  << " device: " << deviceId << std::endl;
    }
    
    return true;
}

std::string AudioEngine::GetCurrentInputDevice() const {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    return currentInputDeviceId_;
}

std::string AudioEngine::GetCurrentOutputDevice() const {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    return currentOutputDeviceId_;
}

bool AudioEngine::SetFormat(const AudioFormat& format) {
    std::lock_guard<std::mutex> lock(formatMutex_);
    
    // Validate format
    auto sampleRateIt = std::find(std::begin(SUPPORTED_SAMPLE_RATES), std::end(SUPPORTED_SAMPLE_RATES), format.sampleRate);
    if (sampleRateIt == std::end(SUPPORTED_SAMPLE_RATES)) {
        return false;
    }
    
    auto bufferSizeIt = std::find(std::begin(SUPPORTED_BUFFER_SIZES), std::end(SUPPORTED_BUFFER_SIZES), format.bufferSize);
    if (bufferSizeIt == std::end(SUPPORTED_BUFFER_SIZES)) {
        return false;
    }
    
    if (format.channels == 0 || format.channels > MAX_CHANNELS) {
        return false;
    }
    
    if (format.bitsPerSample != 16 && format.bitsPerSample != 24 && format.bitsPerSample != 32) {
        return false;
    }
    
    currentFormat_ = format;
    return true;
}

AudioFormat AudioEngine::GetFormat() const {
    std::lock_guard<std::mutex> lock(formatMutex_);
    return currentFormat_;
}

std::vector<uint32_t> AudioEngine::GetSupportedSampleRates() const {
    return std::vector<uint32_t>(std::begin(SUPPORTED_SAMPLE_RATES), std::end(SUPPORTED_SAMPLE_RATES));
}

std::vector<uint32_t> AudioEngine::GetSupportedBufferSizes() const {
    return std::vector<uint32_t>(std::begin(SUPPORTED_BUFFER_SIZES), std::end(SUPPORTED_BUFFER_SIZES));
}

bool AudioEngine::Start() {
    if (isRunning_.load()) {
        return true;
    }
    
    // Ensure we have devices selected
    if (currentOutputDeviceId_.empty()) {
        std::cout << "Selecting default output device..." << std::endl;
        if (!SetOutputDevice("")) { // Use default
            std::cerr << "Failed to select default output device" << std::endl;
            return false;
        }
    }
    
    // Select default input device for microphone capture
    if (currentInputDeviceId_.empty()) {
        std::cout << "Selecting default input device..." << std::endl;
        if (!SetInputDevice("")) { // Use default
            std::cerr << "Warning: Failed to select default input device, audio input will be silent" << std::endl;
            // Don't fail - we can still output audio even without input
        }
    }
    
    // Create input client first to get its native format
    if (!currentInputDeviceId_.empty()) {
        std::cout << "Creating audio client for input device: " << currentInputDeviceId_ << std::endl;
        if (CreateAudioClient(true, &actualInputFormat_)) { // Input
            std::cout << "Input initialized: " << actualInputFormat_.sampleRate << " Hz, "
                      << actualInputFormat_.channels << " channels, "
                      << actualInputFormat_.bitsPerSample << " bits" << std::endl;
            
            // Use input device's format for processing chain, but check compatibility with output
            currentFormat_ = actualInputFormat_;
            std::cout << "Input format will be used for processing chain" << std::endl;
        } else {
            std::cerr << "Warning: Failed to create input audio client, continuing without audio input" << std::endl;
        }
    }
    
    // Create audio client for output device using same format as input
    std::cout << "Creating audio client for output device: " << currentOutputDeviceId_ << std::endl;
    if (!CreateAudioClient(false, &actualOutputFormat_)) { // Output
        std::cerr << "Failed to create audio client. Cannot start audio engine." << std::endl;
        return false;
    }
    
    std::cout << "Output initialized: " << actualOutputFormat_.sampleRate << " Hz, "
              << actualOutputFormat_.channels << " channels, "
              << actualOutputFormat_.bitsPerSample << " bits" << std::endl;
    
    std::cout << "Audio engine starting with:" << std::endl;
    if (actualInputFormat_.sampleRate > 0) {
        std::cout << "  Input:  " << actualInputFormat_.sampleRate << " Hz, "
                  << actualInputFormat_.channels << " ch, "
                  << actualInputFormat_.bitsPerSample << " bits" << std::endl;
    }
    std::cout << "  Output: " << actualOutputFormat_.sampleRate << " Hz, "
              << actualOutputFormat_.channels << " ch, "
              << actualOutputFormat_.bitsPerSample << " bits" << std::endl;
    std::cout << "  Buffer size: " << currentFormat_.bufferSize << " samples" << std::endl;
    
    // Handle sample rate mismatch by using output format as the common format
    if (actualInputFormat_.sampleRate != actualOutputFormat_.sampleRate) {
        std::cout << "Sample rate mismatch detected. Using output format (" 
                  << actualOutputFormat_.sampleRate << " Hz) for processing" << std::endl;
        currentFormat_ = actualOutputFormat_;
    }
    
    // Start audio thread
    shouldStop_.store(false);
    audioThread_ = std::thread(&AudioEngine::AudioThreadProc, this);
    
    isRunning_.store(true);
    return true;
}

bool AudioEngine::Stop() {
    if (!isRunning_.load()) {
        return true;
    }
    
    shouldStop_.store(true);
    
    // Signal audio thread to stop
    if (audioEvent_) {
        SetEvent(audioEvent_);
    }
    
    // Wait for audio thread to finish
    if (audioThread_.joinable()) {
        audioThread_.join();
    }
    
    isRunning_.store(false);
    return true;
}

bool AudioEngine::IsRunning() const {
    return isRunning_.load();
}

bool AudioEngine::CreateAudioClient(bool isInput, AudioFormat* actualFormat) {
    IMMDevice* device = isInput ? inputDevice_ : outputDevice_;
    IAudioClient** client = isInput ? &inputClient_ : &outputClient_;
    
    if (!device) {
        std::cerr << "No " << (isInput ? "input" : "output") << " device selected" << std::endl;
        return false;
    }
    
    // Release existing client
    if (*client) {
        (*client)->Release();
        *client = nullptr;
    }
    
    // Create new audio client
    HRESULT hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client);
    if (FAILED(hr)) {
        std::cerr << "Failed to activate audio client for " << (isInput ? "input" : "output") 
                  << ": 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // Get the device's native mix format (for both input and output)
    WAVEFORMATEX* mixFormat = nullptr;
    hr = (*client)->GetMixFormat(&mixFormat);
    if (FAILED(hr)) {
        std::cerr << "Failed to get mix format for " << (isInput ? "input" : "output") << " device: 0x" << std::hex << hr << std::endl;
        (*client)->Release();
        *client = nullptr;
        return false;
    }
    
    std::cout << (isInput ? "Input" : "Output") << " device native format: "
              << mixFormat->nSamplesPerSec << " Hz, "
              << mixFormat->nChannels << " channels, "
              << mixFormat->wBitsPerSample << " bits" << std::endl;
    
    // For output: if input format is already set, try to use matching format
    // Otherwise use device's native format
    if (!isInput && actualInputFormat_.sampleRate > 0) {
        std::cout << "Attempting to match output format to input: "
                  << actualInputFormat_.sampleRate << " Hz" << std::endl;
        
        // Create format matching input
        WAVEFORMATEXTENSIBLE* desiredFormat = (WAVEFORMATEXTENSIBLE*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
        memcpy(desiredFormat, mixFormat, sizeof(WAVEFORMATEXTENSIBLE));
        desiredFormat->Format.nSamplesPerSec = actualInputFormat_.sampleRate;
        desiredFormat->Format.nAvgBytesPerSec = actualInputFormat_.sampleRate * desiredFormat->Format.nBlockAlign;
        
        // Check if this format is supported
        WAVEFORMATEX* closestMatch = nullptr;
        hr = (*client)->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX*)desiredFormat, &closestMatch);
        
        if (hr == S_OK) {
            // Exact match supported - use it
            CoTaskMemFree(mixFormat);
            mixFormat = (WAVEFORMATEX*)desiredFormat;
            std::cout << "Output device supports input sample rate" << std::endl;
        } else if (hr == S_FALSE && closestMatch) {
            // Close match available
            std::cout << "Using closest match: " << closestMatch->nSamplesPerSec << " Hz" << std::endl;
            CoTaskMemFree(mixFormat);
            CoTaskMemFree(desiredFormat);
            mixFormat = closestMatch;
        } else {
            // Not supported, use device default
            std::cout << "Output device doesn't support input sample rate, using native format" << std::endl;
            CoTaskMemFree(desiredFormat);
        }
    }
    
    // Store the actual format being used
    if (actualFormat) {
        actualFormat->sampleRate = mixFormat->nSamplesPerSec;
        actualFormat->channels = mixFormat->nChannels;
        actualFormat->bitsPerSample = mixFormat->wBitsPerSample;
        actualFormat->bufferSize = currentFormat_.bufferSize; // Keep requested buffer size
    }
    
    // Calculate buffer duration - use 10ms for low latency
    REFERENCE_TIME bufferDuration = 100000;  // 10ms
    
    // Try event callback mode first (for Windows), fall back to polling if it fails
    hr = (*client)->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        bufferDuration,
        0,
        mixFormat,
        nullptr
    );
    
    bool useEventCallback = SUCCEEDED(hr);
    
    if (FAILED(hr)) {
        std::cout << "Event callback mode failed for " << (isInput ? "input" : "output") << ", trying polling mode" << std::endl;
        // Release and recreate client
        (*client)->Release();
        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client);
        if (FAILED(hr)) {
            std::cerr << "Failed to reactivate audio client: 0x" << std::hex << hr << std::endl;
            CoTaskMemFree(mixFormat);
            *client = nullptr;
            return false;
        }
        
        // Get mix format again
        WAVEFORMATEX* mixFormat2 = nullptr;
        hr = (*client)->GetMixFormat(&mixFormat2);
        if (SUCCEEDED(hr)) {
            CoTaskMemFree(mixFormat);
            mixFormat = mixFormat2;
        }
        
        // Try without event callback (polling mode)
        hr = (*client)->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,  // No event callback
            bufferDuration,
            0,
            mixFormat,
            nullptr
        );
    }
    
    CoTaskMemFree(mixFormat);
    
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize " << (isInput ? "input" : "output") << " audio client: 0x" << std::hex << hr << std::endl;
        if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
            std::cerr << "  Format not supported" << std::endl;
        } else if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
            std::cerr << "  Buffer size not aligned" << std::endl;
        } else if (hr == AUDCLNT_E_DEVICE_IN_USE) {
            std::cerr << "  Device already in use" << std::endl;
        }
        (*client)->Release();
        *client = nullptr;
        return false;
    }
    
    if (isInput) {
        // Get the device's preferred format
        WAVEFORMATEX* mixFormat = nullptr;
        hr = (*client)->GetMixFormat(&mixFormat);
        if (FAILED(hr)) {
            std::cerr << "Failed to get mix format for input device: 0x" << std::hex << hr << std::endl;
            (*client)->Release();
            *client = nullptr;
            return false;
        }
        
        std::cout << "Input device native format: "
                  << mixFormat->nSamplesPerSec << " Hz, "
                  << mixFormat->nChannels << " channels, "
                  << mixFormat->wBitsPerSample << " bits" << std::endl;
        
        // Calculate buffer duration - use same duration as output for better sync
        // Default to 10ms (100000 hundred-nanosecond units)
        REFERENCE_TIME bufferDuration = 100000;  // 10ms
        
        // Try event callback mode first (for Windows), fall back to polling if it fails
        hr = (*client)->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            bufferDuration,
            0,
            mixFormat,
            nullptr
        );
        
        bool useEventCallback = SUCCEEDED(hr);
        
        if (FAILED(hr)) {
            std::cout << "Event callback mode failed, trying polling mode for input" << std::endl;
            // Release and recreate client
            (*client)->Release();
            hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client);
            if (FAILED(hr)) {
                std::cerr << "Failed to reactivate audio client for input: 0x" << std::hex << hr << std::endl;
                CoTaskMemFree(mixFormat);
                *client = nullptr;
                return false;
            }
            
            // Get mix format again
            WAVEFORMATEX* mixFormat2 = nullptr;
            hr = (*client)->GetMixFormat(&mixFormat2);
            if (SUCCEEDED(hr)) {
                CoTaskMemFree(mixFormat);
                mixFormat = mixFormat2;
            }
            
            // Try without event callback (polling mode)
            hr = (*client)->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                0,  // No event callback
                bufferDuration,
                0,
                mixFormat,
                nullptr
            );
        }
        
        CoTaskMemFree(mixFormat);
        
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize input audio client: 0x" << std::hex << hr << std::endl;
            if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
                std::cerr << "  Format not supported" << std::endl;
            } else if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
                std::cerr << "  Buffer size not aligned" << std::endl;
            } else if (hr == AUDCLNT_E_DEVICE_IN_USE) {
                std::cerr << "  Device already in use" << std::endl;
            }
            (*client)->Release();
            *client = nullptr;
            return false;
        }
        
        // Get capture client
        hr = (*client)->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get capture client: 0x" << std::hex << hr << std::endl;
            (*client)->Release();
            *client = nullptr;
            return false;
        }
        
        // Set event handle if using event callback mode
        if (useEventCallback) {
            hr = (*client)->SetEventHandle(audioEvent_);
            if (FAILED(hr)) {
                std::cerr << "Failed to set event handle for input: 0x" << std::hex << hr << std::endl;
                (*client)->Release();
                *client = nullptr;
                return false;
            }
            inputEventCallbackMode_ = true;
            std::cout << "Input audio client initialized (event callback mode)" << std::endl;
        } else {
            inputEventCallbackMode_ = false;
            std::cout << "Input audio client initialized (polling mode)" << std::endl;
        }
    } else {
        // Output device
        hr = (*client)->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get render client: 0x" << std::hex << hr << std::endl;
            (*client)->Release();
            *client = nullptr;
            return false;
        }
        
        // Get volume control (optional)
        hr = (*client)->GetService(__uuidof(ISimpleAudioVolume), (void**)&volumeControl_);
        // Volume control is optional, don't fail if not available
        
        // Set event handle if using event callback mode
        if (useEventCallback) {
            hr = (*client)->SetEventHandle(audioEvent_);
            if (FAILED(hr)) {
                std::cerr << "Failed to set event handle for output: 0x" << std::hex << hr << std::endl;
                (*client)->Release();
                *client = nullptr;
                return false;
            }
            outputEventCallbackMode_ = true;
            std::cout << "Output audio client initialized (event callback mode)" << std::endl;
        } else {
            outputEventCallbackMode_ = false;
            std::cout << "Output audio client initialized (polling mode)" << std::endl;
        }
    }
    
    return true;
}

bool AudioEngine::SetupFormat(IAudioClient* client, const AudioFormat& format) {
    WAVEFORMATEX* waveFormat = CreateWaveFormat(format);
    if (!waveFormat) {
        std::cerr << "Failed to create wave format" << std::endl;
        return false;
    }
    
    // Calculate buffer duration in 100-nanosecond units
    REFERENCE_TIME bufferDuration = (REFERENCE_TIME)((double)format.bufferSize / format.sampleRate * 10000000.0);
    
    HRESULT hr = client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        bufferDuration,
        0,
        waveFormat,
        nullptr
    );
    
    CoTaskMemFree(waveFormat);
    
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize audio client: 0x" << std::hex << hr << std::endl;
        if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
            std::cerr << "Format not supported by device" << std::endl;
        } else if (hr == AUDCLNT_E_DEVICE_IN_USE) {
            std::cerr << "Device is already in use" << std::endl;
        } else if (hr == E_INVALIDARG) {
            std::cerr << "Invalid argument in Initialize" << std::endl;
        }
        return false;
    }
    
    // Set event handle
    hr = client->SetEventHandle(audioEvent_);
    if (FAILED(hr)) {
        std::cerr << "Failed to set event handle: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    return true;
}

WAVEFORMATEX* AudioEngine::CreateWaveFormat(const AudioFormat& format) {
    // Use WAVEFORMATEXTENSIBLE for proper float format support
    WAVEFORMATEXTENSIBLE* waveFormatEx = (WAVEFORMATEXTENSIBLE*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if (!waveFormatEx) {
        return nullptr;
    }
    
    memset(waveFormatEx, 0, sizeof(WAVEFORMATEXTENSIBLE));
    
    waveFormatEx->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    waveFormatEx->Format.nChannels = format.channels;
    waveFormatEx->Format.nSamplesPerSec = format.sampleRate;
    waveFormatEx->Format.wBitsPerSample = format.bitsPerSample;
    waveFormatEx->Format.nBlockAlign = (format.channels * format.bitsPerSample) / 8;
    waveFormatEx->Format.nAvgBytesPerSec = format.sampleRate * waveFormatEx->Format.nBlockAlign;
    waveFormatEx->Format.cbSize = 22; // Size of the extension
    
    waveFormatEx->Samples.wValidBitsPerSample = format.bitsPerSample;
    
    // Set channel mask for stereo (front left + front right)
    waveFormatEx->dwChannelMask = (format.channels == 2) ? (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT) : 0;
    
    // Use IEEE float format for 32-bit, PCM for others
    if (format.bitsPerSample == 32) {
        waveFormatEx->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    } else {
        waveFormatEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
    
    return (WAVEFORMATEX*)waveFormatEx;
}

void AudioEngine::AudioThreadProc() {
    // Initialize COM for this thread (required for WASAPI)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hr);
    
    // Start the audio clients
    if (outputClient_) {
        hr = outputClient_->Start();
        if (FAILED(hr)) {
            std::cerr << "Failed to start output client: 0x" << std::hex << hr << std::endl;
            if (comInitialized) CoUninitialize();
            return;
        }
    }
    
    if (inputClient_) {
        hr = inputClient_->Start();
        if (FAILED(hr)) {
            std::cerr << "Warning: Failed to start input client: 0x" << std::hex << hr << std::endl;
            // Continue without input
        }
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (!shouldStop_.load()) {
        // Handle different modes: event callback vs polling
        if (outputEventCallbackMode_) {
            // Event callback mode: wait for audio event
            DWORD waitResult = WaitForSingleObject(audioEvent_, 1000); // 1 second timeout
            
            if (waitResult != WAIT_OBJECT_0) {
                if (waitResult == WAIT_TIMEOUT) {
                    continue;
                } else {
                    break; // Error occurred
                }
            }
        } else {
            // Polling mode: sleep for a short period and check buffers
            Sleep(5); // 5ms sleep for polling mode
        }
        
        if (shouldStop_.load()) {
            break;
        }
        
        // Process audio
        if (renderClient_ && audioCallback_) {
            static bool firstCallback = true;
            if (firstCallback) {
                std::cout << "Audio processing callbacks started" << std::endl;
                firstCallback = false;
            }
            
            UINT32 bufferFrameCount;
            if (SUCCEEDED(outputClient_->GetBufferSize(&bufferFrameCount))) {
                UINT32 numFramesPadding;
                if (SUCCEEDED(outputClient_->GetCurrentPadding(&numFramesPadding))) {
                    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
                    
                    if (numFramesAvailable > 0) {
                        BYTE* outputData;
                        if (SUCCEEDED(renderClient_->GetBuffer(numFramesAvailable, &outputData))) {
                            // Prepare buffers
                            inputBuffer_.resize(numFramesAvailable * currentFormat_.channels);
                            outputBuffer_.resize(numFramesAvailable * currentFormat_.channels);
                            
                            // Capture audio input from microphone if available
                            bool capturedInput = false;
                            if (captureClient_) {
                                UINT32 packetLength = 0;
                                HRESULT hr = captureClient_->GetNextPacketSize(&packetLength);
                                
                                // Check for available packets (polling mode)
                                if (SUCCEEDED(hr) && packetLength > 0) {
                                    BYTE* inputData;
                                    UINT32 framesAvailable;
                                    DWORD flags;
                                    
                                    HRESULT captureHr = captureClient_->GetBuffer(&inputData, &framesAvailable, &flags, nullptr, nullptr);
                                    
                                    if (SUCCEEDED(captureHr) && framesAvailable > 0) {
                                        // Copy captured audio to input buffer
                                        UINT32 framesToCopy = std::min(framesAvailable, numFramesAvailable);
                                        
                                        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                                            // Silence - fill with zeros
                                            std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
                                        } else {
                                            // Copy audio data (assuming 32-bit float format)
                                            if (actualInputFormat_.bitsPerSample == 32) {
                                                memcpy(inputBuffer_.data(), inputData, framesToCopy * actualInputFormat_.channels * sizeof(float));
                                            } else {
                                                // TODO: Implement conversion from other formats
                                                std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
                                            }
                                            
                                            // If captured frames < needed frames, zero the rest
                                            if (framesToCopy < numFramesAvailable) {
                                                std::fill(inputBuffer_.begin() + (framesToCopy * actualInputFormat_.channels),
                                                         inputBuffer_.end(), 0.0f);
                                            }
                                        }
                                        
                                        captureClient_->ReleaseBuffer(framesAvailable);
                                        capturedInput = true;
                                        
                                        // Log successful captures occasionally
                                        static int successCount = 0;
                                        successCount++;
                                        if (successCount == 1) {
                                            std::cout << "SUCCESS: Microphone input is working in polling mode" << std::endl;
                                        }
                                    }
                                }
                            }
                            
                            // If no input captured, use silence instead of test tone
                            if (!capturedInput) {
                                // Fill input buffer with silence
                                std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
                                
                                // Log silence usage (reduced frequency)
                                static int silenceCounter = 0;
                                silenceCounter++;
                                if (silenceCounter == 1) {
                                    std::cout << "INFO: Using silence when microphone data not available" << std::endl;
                                }
                            }
                            
                            // Call user callback to process audio
                            audioCallback_(inputBuffer_.data(), outputBuffer_.data(), numFramesAvailable, callbackUserData_);
                            
                            // Convert float to target format and copy to WASAPI buffer
                            if (currentFormat_.bitsPerSample == 32) {
                                // 32-bit float
                                memcpy(outputData, outputBuffer_.data(), numFramesAvailable * currentFormat_.channels * sizeof(float));
                            } else {
                                // TODO: Implement 16-bit and 24-bit conversions
                                memset(outputData, 0, numFramesAvailable * currentFormat_.channels * (currentFormat_.bitsPerSample / 8));
                            }
                            
                            renderClient_->ReleaseBuffer(numFramesAvailable, 0);
                        }
                    }
                }
            }
        }
        
        // Update CPU usage periodically
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(now - startTime).count();
        if (elapsed >= CPU_MEASUREMENT_INTERVAL) {
            // Simple CPU usage calculation (this is a placeholder)
            cpuUsage_.store(5.0); // TODO: Implement actual CPU measurement
            startTime = now;
        }
    }
    
    // Stop the audio clients
    if (inputClient_) {
        inputClient_->Stop();
    }
    
    if (outputClient_) {
        outputClient_->Stop();
    }
    
    // Uninitialize COM for this thread
    if (comInitialized) {
        CoUninitialize();
    }
}

void AudioEngine::SetAudioCallback(AudioCallback callback, void* userData) {
    audioCallback_ = callback;
    callbackUserData_ = userData;
}

double AudioEngine::GetCpuUsage() const {
    return cpuUsage_.load();
}

double AudioEngine::GetLatency() const {
    if (!outputClient_) {
        return 0.0;
    }
    
    REFERENCE_TIME latency;
    if (SUCCEEDED(outputClient_->GetStreamLatency(&latency))) {
        return latency / 10000.0; // Convert to milliseconds
    }
    
    return 0.0;
}

uint32_t AudioEngine::GetDropouts() const {
    return dropoutCount_.load();
}

bool AudioEngine::SetMasterVolume(float volume) {
    if (!volumeControl_) {
        return false;
    }
    
    volume = std::max(0.0f, std::min(1.0f, volume));
    return SUCCEEDED(volumeControl_->SetMasterVolume(volume, nullptr));
}

float AudioEngine::GetMasterVolume() const {
    if (!volumeControl_) {
        return 1.0f;
    }
    
    float volume = 1.0f;
    volumeControl_->GetMasterVolume(&volume);
    return volume;
}

bool AudioEngine::SetMuted(bool muted) {
    if (!volumeControl_) {
        return false;
    }
    
    return SUCCEEDED(volumeControl_->SetMute(muted, nullptr));
}

bool AudioEngine::IsMuted() const {
    if (!volumeControl_) {
        return false;
    }
    
    BOOL muted = FALSE;
    volumeControl_->GetMute(&muted);
    return muted != FALSE;
}

} // namespace violet