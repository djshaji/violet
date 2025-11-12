#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>

namespace violet {

// Audio device information
struct AudioDevice {
    std::string id;
    std::string name;
    bool isDefault;
    bool isInput;
    bool isOutput;
};

// Audio format specification
struct AudioFormat {
    uint32_t sampleRate;      // 44100, 48000, 88200, 96000, 192000
    uint32_t channels;        // 1 (mono), 2 (stereo), etc.
    uint32_t bitsPerSample;   // 16, 24, 32
    uint32_t bufferSize;      // 64, 128, 256, 512, 1024, 2048 samples
    
    AudioFormat() : sampleRate(44100), channels(2), bitsPerSample(32), bufferSize(256) {}
};

// Audio callback function type
// Parameters: input buffer, output buffer, frame count, user data
using AudioCallback = std::function<void(float* input, float* output, uint32_t frames, void* userData)>;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    
    // Initialization and cleanup
    bool Initialize();
    void Shutdown();
    
    // Device management
    std::vector<AudioDevice> EnumerateDevices();
    bool SetInputDevice(const std::string& deviceId);
    bool SetOutputDevice(const std::string& deviceId);
    std::string GetCurrentInputDevice() const;
    std::string GetCurrentOutputDevice() const;
    
    // Audio format
    bool SetFormat(const AudioFormat& format);
    AudioFormat GetFormat() const;
    std::vector<uint32_t> GetSupportedSampleRates() const;
    std::vector<uint32_t> GetSupportedBufferSizes() const;
    
    // Audio processing
    bool Start();
    bool Stop();
    bool IsRunning() const;
    
    // Callback management
    void SetAudioCallback(AudioCallback callback, void* userData = nullptr);
    
    // Performance monitoring
    double GetCpuUsage() const;
    double GetLatency() const;
    uint32_t GetDropouts() const;
    
    // Volume control
    bool SetMasterVolume(float volume); // 0.0 to 1.0
    float GetMasterVolume() const;
    bool SetMuted(bool muted);
    bool IsMuted() const;
    
private:
    // WASAPI implementation
    bool InitializeWASAPI();
    void ShutdownWASAPI();
    bool CreateAudioClient(bool isInput, AudioFormat* actualFormat = nullptr);
    void AudioThreadProc();
    
    // Device helpers
    bool SelectDevice(const std::string& deviceId, bool isInput);
    std::string GetDeviceName(IMMDevice* device);
    std::string GetDeviceId(IMMDevice* device);
    
    // Format helpers
    bool SetupFormat(IAudioClient* client, const AudioFormat& format);
    WAVEFORMATEX* CreateWaveFormat(const AudioFormat& format);
    
    // COM interfaces
    IMMDeviceEnumerator* deviceEnumerator_;
    IMMDevice* inputDevice_;
    IMMDevice* outputDevice_;
    IAudioClient* inputClient_;
    IAudioClient* outputClient_;
    IAudioRenderClient* renderClient_;
    IAudioCaptureClient* captureClient_;
    ISimpleAudioVolume* volumeControl_;
    
    // Audio thread management
    std::thread audioThread_;
    std::atomic<bool> isRunning_;
    std::atomic<bool> shouldStop_;
    HANDLE audioEvent_;
    bool inputEventCallbackMode_;
    bool outputEventCallbackMode_;
    
    // Configuration
    AudioFormat currentFormat_;        // Requested format
    AudioFormat actualInputFormat_;    // Actual input device format
    AudioFormat actualOutputFormat_;   // Actual output device format
    std::string currentInputDeviceId_;
    std::string currentOutputDeviceId_;
    
    // Callback
    AudioCallback audioCallback_;
    void* callbackUserData_;
    
    // Performance monitoring
    std::atomic<double> cpuUsage_;
    std::atomic<uint32_t> dropoutCount_;
    std::chrono::high_resolution_clock::time_point lastCpuMeasurement_;
    
    // Internal buffers
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    
    // Thread safety
    mutable std::mutex deviceMutex_;
    mutable std::mutex formatMutex_;
    
    // Constants
    static const uint32_t SUPPORTED_SAMPLE_RATES[];
    static const uint32_t SUPPORTED_BUFFER_SIZES[];
    static constexpr uint32_t MAX_CHANNELS = 8;
    static constexpr double CPU_MEASUREMENT_INTERVAL = 1.0; // seconds
};

} // namespace violet