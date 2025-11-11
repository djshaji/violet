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
    }
    
    if (outputClient_) {
        outputClient_->Release();
        outputClient_ = nullptr;
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
    
    // Create audio clients
    std::cout << "Creating audio client for output device: " << currentOutputDeviceId_ << std::endl;
    if (!CreateAudioClient(false)) { // Output
        std::cerr << "Failed to create audio client. Cannot start audio engine." << std::endl;
        return false;
    }
    
    std::cout << "Audio engine starting with:" << std::endl;
    std::cout << "  Sample rate: " << currentFormat_.sampleRate << " Hz" << std::endl;
    std::cout << "  Channels: " << currentFormat_.channels << std::endl;
    std::cout << "  Bit depth: " << currentFormat_.bitsPerSample << " bits" << std::endl;
    std::cout << "  Buffer size: " << currentFormat_.bufferSize << " samples" << std::endl;
    
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

bool AudioEngine::CreateAudioClient(bool isInput) {
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
    
    // Set up format
    if (!SetupFormat(*client, currentFormat_)) {
        (*client)->Release();
        *client = nullptr;
        return false;
    }
    
    // Get render/capture client
    if (!isInput) {
        hr = (*client)->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get render client: 0x" << std::hex << hr << std::endl;
            return false;
        }
        
        // Get volume control
        hr = (*client)->GetService(__uuidof(ISimpleAudioVolume), (void**)&volumeControl_);
        // Volume control is optional, don't fail if not available
    } else {
        hr = (*client)->GetService(__uuidof(IAudioCaptureClient), (void**)&captureClient_);
        if (FAILED(hr)) {
            std::cerr << "Failed to get capture client: 0x" << std::hex << hr << std::endl;
            return false;
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
    
    // Start the audio client
    if (outputClient_) {
        hr = outputClient_->Start();
        if (FAILED(hr)) {
            std::cerr << "Failed to start output client: 0x" << std::hex << hr << std::endl;
            if (comInitialized) CoUninitialize();
            return;
        }
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (!shouldStop_.load()) {
        // Wait for audio event
        DWORD waitResult = WaitForSingleObject(audioEvent_, 1000); // 1 second timeout
        
        if (waitResult != WAIT_OBJECT_0) {
            if (waitResult == WAIT_TIMEOUT) {
                continue;
            } else {
                break; // Error occurred
            }
        }
        
        if (shouldStop_.load()) {
            break;
        }
        
        // Process audio
        if (renderClient_ && audioCallback_) {
            UINT32 bufferFrameCount;
            if (SUCCEEDED(outputClient_->GetBufferSize(&bufferFrameCount))) {
                UINT32 numFramesPadding;
                if (SUCCEEDED(outputClient_->GetCurrentPadding(&numFramesPadding))) {
                    UINT32 numFramesAvailable = bufferFrameCount - numFramesPadding;
                    
                    if (numFramesAvailable > 0) {
                        BYTE* data;
                        if (SUCCEEDED(renderClient_->GetBuffer(numFramesAvailable, &data))) {
                            // Prepare buffers
                            inputBuffer_.resize(numFramesAvailable * currentFormat_.channels);
                            outputBuffer_.resize(numFramesAvailable * currentFormat_.channels);
                            
                            // Clear input buffer (no input capture implemented yet)
                            std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
                            
                            // Call user callback
                            audioCallback_(inputBuffer_.data(), outputBuffer_.data(), numFramesAvailable, callbackUserData_);
                            
                            // Convert float to target format and copy to WASAPI buffer
                            if (currentFormat_.bitsPerSample == 32) {
                                // 32-bit float
                                memcpy(data, outputBuffer_.data(), numFramesAvailable * currentFormat_.channels * sizeof(float));
                            } else {
                                // TODO: Implement 16-bit and 24-bit conversions
                                memset(data, 0, numFramesAvailable * currentFormat_.channels * (currentFormat_.bitsPerSample / 8));
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
    
    // Stop the audio client
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