#include "violet/midi_handler.h"
#include "violet/utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace violet {

MidiHandler::MidiHandler()
    : inputHandle_(nullptr)
    , outputHandle_(nullptr)
    , currentInputDeviceId_(UINT_MAX)
    , currentOutputDeviceId_(UINT_MAX)
    , isInitialized_(false)
    , isInputRunning_(false)
    , isInputDeviceOpen_(false)
    , isOutputDeviceOpen_(false)
    , inputCallback_(nullptr)
    , callbackUserData_(nullptr)
    , inputBufferSize_(DEFAULT_BUFFER_SIZE)
    , inputMessageCount_(0)
    , outputMessageCount_(0)
    , droppedMessageCount_(0)
    , startTime_(0) {
    
    inputBuffer_ = std::make_unique<MidiBuffer>(inputBufferSize_);
}

MidiHandler::~MidiHandler() {
    Shutdown();
}

bool MidiHandler::Initialize() {
    if (isInitialized_) {
        return true;
    }
    
    startTime_ = GetHighResolutionTime();
    isInitialized_ = true;
    
    return true;
}

void MidiHandler::Shutdown() {
    if (!isInitialized_) {
        return;
    }
    
    StopInput();
    CloseInputDevice();
    CloseOutputDevice();
    
    isInitialized_ = false;
}

std::vector<MidiDevice> MidiHandler::EnumerateInputDevices() {
    std::vector<MidiDevice> devices;
    
    UINT numDevices = midiInGetNumDevs();
    for (UINT i = 0; i < numDevices; ++i) {
        devices.push_back(GetInputDeviceInfo(i));
    }
    
    return devices;
}

std::vector<MidiDevice> MidiHandler::EnumerateOutputDevices() {
    std::vector<MidiDevice> devices;
    
    UINT numDevices = midiOutGetNumDevs();
    for (UINT i = 0; i < numDevices; ++i) {
        devices.push_back(GetOutputDeviceInfo(i));
    }
    
    return devices;
}

MidiDevice MidiHandler::GetInputDeviceInfo(uint32_t deviceId) {
    MidiDevice device;
    device.id = deviceId;
    device.isInput = true;
    device.isOutput = false;
    
    MIDIINCAPS caps;
    if (midiInGetDevCaps(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
        device.name = utils::WStringToString(caps.szPname);
        device.manufacturer = ""; // Not available in MIDIINCAPS
    } else {
        device.name = "Unknown Input Device";
        device.manufacturer = "Unknown";
    }
    
    return device;
}

MidiDevice MidiHandler::GetOutputDeviceInfo(uint32_t deviceId) {
    MidiDevice device;
    device.id = deviceId;
    device.isInput = false;
    device.isOutput = true;
    
    MIDIOUTCAPS caps;
    if (midiOutGetDevCaps(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
        device.name = utils::WStringToString(caps.szPname);
        device.manufacturer = ""; // Not available in MIDIOUTCAPS
    } else {
        device.name = "Unknown Output Device";
        device.manufacturer = "Unknown";
    }
    
    return device;
}

bool MidiHandler::OpenInputDevice(uint32_t deviceId) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    if (isInputDeviceOpen_.load()) {
        CloseInputDevice();
    }
    
    MMRESULT result = midiInOpen(&inputHandle_, deviceId, 
                                (DWORD_PTR)MidiInputCallback, 
                                (DWORD_PTR)this, 
                                CALLBACK_FUNCTION);
    
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open MIDI input device " << deviceId << ": " << result << std::endl;
        return false;
    }
    
    currentInputDeviceId_ = deviceId;
    isInputDeviceOpen_.store(true);
    
    return true;
}

bool MidiHandler::OpenOutputDevice(uint32_t deviceId) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    if (isOutputDeviceOpen_.load()) {
        CloseOutputDevice();
    }
    
    MMRESULT result = midiOutOpen(&outputHandle_, deviceId, 0, 0, CALLBACK_NULL);
    
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to open MIDI output device " << deviceId << ": " << result << std::endl;
        return false;
    }
    
    currentOutputDeviceId_ = deviceId;
    isOutputDeviceOpen_.store(true);
    
    return true;
}

void MidiHandler::CloseInputDevice() {
    if (!isInputDeviceOpen_.load()) {
        return;
    }
    
    StopInput();
    
    if (inputHandle_) {
        midiInReset(inputHandle_);
        midiInClose(inputHandle_);
        inputHandle_ = nullptr;
    }
    
    currentInputDeviceId_ = UINT_MAX;
    isInputDeviceOpen_.store(false);
}

void MidiHandler::CloseOutputDevice() {
    if (!isOutputDeviceOpen_.load()) {
        return;
    }
    
    if (outputHandle_) {
        midiOutReset(outputHandle_);
        midiOutClose(outputHandle_);
        outputHandle_ = nullptr;
    }
    
    currentOutputDeviceId_ = UINT_MAX;
    isOutputDeviceOpen_.store(false);
}

bool MidiHandler::IsInputDeviceOpen() const {
    return isInputDeviceOpen_.load();
}

bool MidiHandler::IsOutputDeviceOpen() const {
    return isOutputDeviceOpen_.load();
}

bool MidiHandler::StartInput() {
    if (!isInputDeviceOpen_.load() || isInputRunning_) {
        return isInputRunning_;
    }
    
    MMRESULT result = midiInStart(inputHandle_);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "Failed to start MIDI input: " << result << std::endl;
        return false;
    }
    
    isInputRunning_ = true;
    return true;
}

void MidiHandler::StopInput() {
    if (!isInputRunning_) {
        return;
    }
    
    if (inputHandle_) {
        midiInStop(inputHandle_);
    }
    
    isInputRunning_ = false;
}

bool MidiHandler::IsInputRunning() const {
    return isInputRunning_;
}

void MidiHandler::SetInputCallback(MidiCallback callback, void* userData) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    inputCallback_ = callback;
    callbackUserData_ = userData;
}

void CALLBACK MidiHandler::MidiInputCallback(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    MidiHandler* handler = reinterpret_cast<MidiHandler*>(dwInstance);
    if (handler) {
        handler->ProcessMidiInput(dwParam1, dwParam2);
    }
}

void MidiHandler::ProcessMidiInput(DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    uint32_t timestamp = GetHighResolutionTime();
    
    // Parse short MIDI message
    DWORD midiData = static_cast<DWORD>(dwParam1);
    uint8_t status = midiData & 0xFF;
    uint8_t data1 = (midiData >> 8) & 0xFF;
    uint8_t data2 = (midiData >> 16) & 0xFF;
    
    MidiMessage message(timestamp, status, data1, data2);
    
    // Add to buffer
    MidiEvent event;
    event.timestamp = timestamp;
    event.data[0] = status;
    event.data[1] = data1;
    event.data[2] = data2;
    event.data[3] = 0;
    event.size = 3; // Most MIDI messages are 3 bytes
    
    // Adjust size for different message types
    if (message.IsProgramChange() || message.IsChannelPressure()) {
        event.size = 2;
    }
    
    if (inputBuffer_->Write(&event, 1) == 0) {
        droppedMessageCount_.fetch_add(1);
    } else {
        inputMessageCount_.fetch_add(1);
    }
    
    // Call user callback
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (inputCallback_) {
            inputCallback_(message, callbackUserData_);
        }
    }
}

bool MidiHandler::SendMessage(const MidiMessage& message) {
    if (!isOutputDeviceOpen_.load()) {
        return false;
    }
    
    DWORD midiData = message.status | (message.data1 << 8) | (message.data2 << 16);
    MMRESULT result = midiOutShortMsg(outputHandle_, midiData);
    
    if (result == MMSYSERR_NOERROR) {
        outputMessageCount_.fetch_add(1);
        return true;
    }
    
    return false;
}

bool MidiHandler::SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    return SendMessage(MidiMessage(GetHighResolutionTime(), 0x90 | (channel & 0x0F), note & 0x7F, velocity & 0x7F));
}

bool MidiHandler::SendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    return SendMessage(MidiMessage(GetHighResolutionTime(), 0x80 | (channel & 0x0F), note & 0x7F, velocity & 0x7F));
}

bool MidiHandler::SendControlChange(uint8_t channel, uint8_t controller, uint8_t value) {
    return SendMessage(MidiMessage(GetHighResolutionTime(), 0xB0 | (channel & 0x0F), controller & 0x7F, value & 0x7F));
}

bool MidiHandler::SendPitchBend(uint8_t channel, int16_t value) {
    // Convert -8192 to +8191 range to 0-16383
    uint16_t bendValue = static_cast<uint16_t>(value + 8192);
    uint8_t lsb = bendValue & 0x7F;
    uint8_t msb = (bendValue >> 7) & 0x7F;
    
    return SendMessage(MidiMessage(GetHighResolutionTime(), 0xE0 | (channel & 0x0F), lsb, msb));
}

bool MidiHandler::SendProgramChange(uint8_t channel, uint8_t program) {
    return SendMessage(MidiMessage(GetHighResolutionTime(), 0xC0 | (channel & 0x0F), program & 0x7F, 0));
}

void MidiHandler::SetInputBufferSize(size_t size) {
    inputBufferSize_ = size;
    inputBuffer_ = std::make_unique<MidiBuffer>(size);
}

size_t MidiHandler::GetInputBufferSize() const {
    return inputBufferSize_;
}

MidiBuffer* MidiHandler::GetInputBuffer() {
    return inputBuffer_.get();
}

uint32_t MidiHandler::GetInputMessageCount() const {
    return inputMessageCount_.load();
}

uint32_t MidiHandler::GetOutputMessageCount() const {
    return outputMessageCount_.load();
}

uint32_t MidiHandler::GetDroppedMessageCount() const {
    return droppedMessageCount_.load();
}

uint32_t MidiHandler::GetHighResolutionTime() {
    return timeGetTime() - startTime_;
}

std::string MidiHandler::MessageTypeToString(uint8_t messageType) {
    switch (messageType) {
    case 0x80: return "Note Off";
    case 0x90: return "Note On";
    case 0xA0: return "Polyphonic Pressure";
    case 0xB0: return "Control Change";
    case 0xC0: return "Program Change";
    case 0xD0: return "Channel Pressure";
    case 0xE0: return "Pitch Bend";
    case 0xF0: return "System";
    default: return "Unknown";
    }
}

std::string MidiHandler::MessageToString(const MidiMessage& message) {
    std::ostringstream oss;
    oss << MessageTypeToString(message.GetMessageType()) 
        << " Ch:" << (message.GetChannel() + 1)
        << " D1:" << static_cast<int>(message.data1)
        << " D2:" << static_cast<int>(message.data2);
    return oss.str();
}

// MidiParameterMapper implementation
MidiParameterMapper::MidiParameterMapper()
    : learnModeEnabled_(false)
    , hasLearnTarget_(false)
    , learnParameterIndex_(0)
    , learnMinValue_(0.0f)
    , learnMaxValue_(1.0f)
    , learnIsToggle_(false) {
}

MidiParameterMapper::~MidiParameterMapper() = default;

void MidiParameterMapper::AddMapping(const ParameterMapping& mapping) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    // Remove existing mapping for the same CC
    auto it = std::find_if(mappings_.begin(), mappings_.end(),
                          [&mapping](const ParameterMapping& m) {
                              return m.channel == mapping.channel && m.controller == mapping.controller;
                          });
    
    if (it != mappings_.end()) {
        mappings_.erase(it);
    }
    
    mappings_.push_back(mapping);
}

void MidiParameterMapper::RemoveMapping(uint8_t channel, uint8_t controller) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    mappings_.erase(std::remove_if(mappings_.begin(), mappings_.end(),
                                  [channel, controller](const ParameterMapping& m) {
                                      return m.channel == channel && m.controller == controller;
                                  }),
                   mappings_.end());
}

void MidiParameterMapper::ClearMappings() {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    mappings_.clear();
}

std::vector<MidiParameterMapper::ParameterMapping> MidiParameterMapper::GetMappings() const {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    return mappings_;
}

float MidiParameterMapper::ControlChangeToParameter(const MidiMessage& message, const ParameterMapping& mapping) const {
    if (!message.IsControlChange()) {
        return mapping.minValue;
    }
    
    if (mapping.isToggle) {
        return message.data2 > 0 ? mapping.maxValue : mapping.minValue;
    }
    
    // Convert 0-127 MIDI range to parameter range
    float normalized = message.data2 / 127.0f;
    return mapping.minValue + normalized * (mapping.maxValue - mapping.minValue);
}

bool MidiParameterMapper::FindMapping(uint8_t channel, uint8_t controller, ParameterMapping& mapping) const {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto it = std::find_if(mappings_.begin(), mappings_.end(),
                          [channel, controller](const ParameterMapping& m) {
                              return m.channel == channel && m.controller == controller;
                          });
    
    if (it != mappings_.end()) {
        mapping = *it;
        return true;
    }
    
    return false;
}

void MidiParameterMapper::SetLearnMode(bool enabled) {
    learnModeEnabled_ = enabled;
    if (!enabled) {
        hasLearnTarget_ = false;
    }
}

bool MidiParameterMapper::IsLearnModeEnabled() const {
    return learnModeEnabled_;
}

void MidiParameterMapper::SetLearnTarget(uint32_t parameterIndex, float minValue, float maxValue, bool isToggle) {
    learnParameterIndex_ = parameterIndex;
    learnMinValue_ = minValue;
    learnMaxValue_ = maxValue;
    learnIsToggle_ = isToggle;
    hasLearnTarget_ = true;
}

bool MidiParameterMapper::ProcessLearnMessage(const MidiMessage& message) {
    if (!learnModeEnabled_ || !hasLearnTarget_ || !message.IsControlChange()) {
        return false;
    }
    
    ParameterMapping mapping;
    mapping.channel = message.GetChannel();
    mapping.controller = message.data1;
    mapping.parameterIndex = learnParameterIndex_;
    mapping.minValue = learnMinValue_;
    mapping.maxValue = learnMaxValue_;
    mapping.isToggle = learnIsToggle_;
    
    AddMapping(mapping);
    
    hasLearnTarget_ = false;
    learnModeEnabled_ = false;
    
    return true;
}

} // namespace violet