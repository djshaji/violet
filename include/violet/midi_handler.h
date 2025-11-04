#pragma once

#include <windows.h>
#include <mmeapi.h>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <string>
#include "violet/audio_buffer.h"

namespace violet {

// MIDI device information
struct MidiDevice {
    uint32_t id;
    std::string name;
    std::string manufacturer;
    bool isInput;
    bool isOutput;
};

// MIDI message structure
struct MidiMessage {
    uint32_t timestamp;    // High-resolution timestamp
    uint8_t status;        // Status byte (includes channel)
    uint8_t data1;         // First data byte
    uint8_t data2;         // Second data byte
    uint32_t deltaTime;    // Delta time from previous message
    
    MidiMessage() : timestamp(0), status(0), data1(0), data2(0), deltaTime(0) {}
    
    MidiMessage(uint32_t ts, uint8_t st, uint8_t d1, uint8_t d2)
        : timestamp(ts), status(st), data1(d1), data2(d2), deltaTime(0) {}
    
    // MIDI message type helpers
    uint8_t GetMessageType() const { return status & 0xF0; }
    uint8_t GetChannel() const { return status & 0x0F; }
    bool IsNoteOn() const { return GetMessageType() == 0x90 && data2 > 0; }
    bool IsNoteOff() const { return GetMessageType() == 0x80 || (GetMessageType() == 0x90 && data2 == 0); }
    bool IsControlChange() const { return GetMessageType() == 0xB0; }
    bool IsPitchBend() const { return GetMessageType() == 0xE0; }
    bool IsChannelPressure() const { return GetMessageType() == 0xD0; }
    bool IsPolyphonicPressure() const { return GetMessageType() == 0xA0; }
    bool IsProgramChange() const { return GetMessageType() == 0xC0; }
    
    // Get 14-bit pitch bend value (-8192 to +8191)
    int16_t GetPitchBendValue() const {
        if (!IsPitchBend()) return 0;
        return ((data2 << 7) | data1) - 8192;
    }
    
    // Get normalized pitch bend value (-1.0 to +1.0)
    float GetNormalizedPitchBend() const {
        return GetPitchBendValue() / 8192.0f;
    }
};

// MIDI callback function type
using MidiCallback = std::function<void(const MidiMessage& message, void* userData)>;

// MIDI input handler
class MidiHandler {
public:
    MidiHandler();
    ~MidiHandler();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Device management
    std::vector<MidiDevice> EnumerateInputDevices();
    std::vector<MidiDevice> EnumerateOutputDevices();
    bool OpenInputDevice(uint32_t deviceId);
    bool OpenOutputDevice(uint32_t deviceId);
    void CloseInputDevice();
    void CloseOutputDevice();
    bool IsInputDeviceOpen() const;
    bool IsOutputDeviceOpen() const;
    
    // MIDI input
    bool StartInput();
    void StopInput();
    bool IsInputRunning() const;
    void SetInputCallback(MidiCallback callback, void* userData = nullptr);
    
    // MIDI output
    bool SendMessage(const MidiMessage& message);
    bool SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    bool SendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    bool SendControlChange(uint8_t channel, uint8_t controller, uint8_t value);
    bool SendPitchBend(uint8_t channel, int16_t value);
    bool SendProgramChange(uint8_t channel, uint8_t program);
    
    // Buffer management
    void SetInputBufferSize(size_t size);
    size_t GetInputBufferSize() const;
    MidiBuffer* GetInputBuffer();
    
    // Performance monitoring
    uint32_t GetInputMessageCount() const;
    uint32_t GetOutputMessageCount() const;
    uint32_t GetDroppedMessageCount() const;
    
    // Utility functions
    static std::string MessageTypeToString(uint8_t messageType);
    static std::string MessageToString(const MidiMessage& message);
    
private:
    // Windows MIDI callback
    static void CALLBACK MidiInputCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void ProcessMidiInput(DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    
    // Device helpers
    MidiDevice GetInputDeviceInfo(uint32_t deviceId);
    MidiDevice GetOutputDeviceInfo(uint32_t deviceId);
    
    // Timing
    uint32_t GetHighResolutionTime();
    
    // MIDI handles
    HMIDIIN inputHandle_;
    HMIDIOUT outputHandle_;
    
    // Device information
    uint32_t currentInputDeviceId_;
    uint32_t currentOutputDeviceId_;
    
    // State
    bool isInitialized_;
    bool isInputRunning_;
    std::atomic<bool> isInputDeviceOpen_;
    std::atomic<bool> isOutputDeviceOpen_;
    
    // Callback
    MidiCallback inputCallback_;
    void* callbackUserData_;
    
    // Input buffer
    std::unique_ptr<MidiBuffer> inputBuffer_;
    size_t inputBufferSize_;
    
    // Performance counters
    std::atomic<uint32_t> inputMessageCount_;
    std::atomic<uint32_t> outputMessageCount_;
    std::atomic<uint32_t> droppedMessageCount_;
    
    // Thread safety
    mutable std::mutex deviceMutex_;
    mutable std::mutex callbackMutex_;
    
    // Timing reference
    uint32_t startTime_;
    
    // Constants
    static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;
    static constexpr uint32_t SYSEX_BUFFER_SIZE = 1024;
};

// MIDI parameter mapping for plugin control
class MidiParameterMapper {
public:
    struct ParameterMapping {
        uint8_t channel;        // MIDI channel (0-15)
        uint8_t controller;     // CC number (0-127)
        uint32_t parameterIndex; // Plugin parameter index
        float minValue;         // Minimum parameter value
        float maxValue;         // Maximum parameter value
        bool isToggle;          // Toggle on/off with any value > 0
        
        ParameterMapping()
            : channel(0), controller(0), parameterIndex(0)
            , minValue(0.0f), maxValue(1.0f), isToggle(false) {}
    };
    
    MidiParameterMapper();
    ~MidiParameterMapper();
    
    // Mapping management
    void AddMapping(const ParameterMapping& mapping);
    void RemoveMapping(uint8_t channel, uint8_t controller);
    void ClearMappings();
    std::vector<ParameterMapping> GetMappings() const;
    
    // Parameter conversion
    float ControlChangeToParameter(const MidiMessage& message, const ParameterMapping& mapping) const;
    bool FindMapping(uint8_t channel, uint8_t controller, ParameterMapping& mapping) const;
    
    // Learn mode
    void SetLearnMode(bool enabled);
    bool IsLearnModeEnabled() const;
    void SetLearnTarget(uint32_t parameterIndex, float minValue, float maxValue, bool isToggle = false);
    bool ProcessLearnMessage(const MidiMessage& message); // Returns true if mapping was created
    
private:
    std::vector<ParameterMapping> mappings_;
    mutable std::mutex mappingsMutex_;
    
    // Learn mode state
    bool learnModeEnabled_;
    bool hasLearnTarget_;
    uint32_t learnParameterIndex_;
    float learnMinValue_;
    float learnMaxValue_;
    bool learnIsToggle_;
};

} // namespace violet