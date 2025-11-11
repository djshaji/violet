#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include "violet/plugin_manager.h"
#include "violet/audio_buffer.h"
#include "violet/midi_handler.h"

namespace violet {

// Forward declarations
class AudioEngine;

// Audio processing node representing a plugin in the chain
class ProcessingNode {
public:
    ProcessingNode(std::unique_ptr<PluginInstance> plugin, uint32_t channels, uint32_t blockSize);
    ~ProcessingNode();
    
    // Processing
    void Process(float** inputBuffers, float** outputBuffers, uint32_t frames);
    void ProcessMidi(MidiBuffer* midiBuffer, uint32_t frames);
    
    // Plugin control
    PluginInstance* GetPlugin() { return plugin_.get(); }
    const PluginInstance* GetPlugin() const { return plugin_.get(); }
    
    // Bypass control
    void SetBypassed(bool bypassed) { bypassed_.store(bypassed); }
    bool IsBypassed() const { return bypassed_.load(); }
    
    // Node state
    bool IsActive() const;
    bool Activate();
    void Deactivate();
    
    // Audio routing
    void SetInputChannels(const std::vector<uint32_t>& channels);
    void SetOutputChannels(const std::vector<uint32_t>& channels);
    std::vector<uint32_t> GetInputChannels() const { return inputChannels_; }
    std::vector<uint32_t> GetOutputChannels() const { return outputChannels_; }
    
    // Parameter automation
    struct AutomationPoint {
        uint32_t sampleTime;
        uint32_t parameterIndex;
        float value;
    };
    
    void AddAutomationPoint(const AutomationPoint& point);
    void ClearAutomation();
    void ProcessAutomation(uint32_t currentSample, uint32_t frames);
    
private:
    void AllocateBuffers();
    void ConnectPorts();
    void ProcessParameterChanges();
    
    std::unique_ptr<PluginInstance> plugin_;
    uint32_t channels_;
    uint32_t blockSize_;
    
    // Audio buffers
    std::vector<std::vector<float>> inputBuffers_;
    std::vector<std::vector<float>> outputBuffers_;
    std::vector<float*> inputPtrs_;
    std::vector<float*> outputPtrs_;
    
    // Control parameters
    std::vector<float> controlValues_;
    std::vector<bool> parameterChanged_;
    std::vector<float> controlOutputDummy_;  // Dummy buffers for control output ports
    
    // Channel routing
    std::vector<uint32_t> inputChannels_;
    std::vector<uint32_t> outputChannels_;
    
    // State
    std::atomic<bool> bypassed_;
    
    // Automation
    std::vector<AutomationPoint> automationPoints_;
    std::mutex automationMutex_;
    
    // MIDI buffers (for future MIDI support)
    std::vector<uint8_t> midiBuffer_;
};

// Audio processing chain manager
class AudioProcessingChain {
public:
    AudioProcessingChain(AudioEngine* audioEngine);
    ~AudioProcessingChain();
    
    // Chain management
    uint32_t AddPlugin(const std::string& pluginUri, uint32_t position = UINT_MAX);
    bool RemovePlugin(uint32_t nodeId);
    bool MovePlugin(uint32_t nodeId, uint32_t newPosition);
    void ClearChain();
    
    // Node access
    ProcessingNode* GetNode(uint32_t nodeId);
    const ProcessingNode* GetNode(uint32_t nodeId) const;
    std::vector<uint32_t> GetNodeIds() const;
    uint32_t GetNodeCount() const;
    
    // Processing
    void Process(float** inputBuffers, float** outputBuffers, uint32_t channels, uint32_t frames);
    void ProcessMidi(MidiBuffer* midiBuffer, uint32_t frames);
    
    // Chain state
    void SetBypassed(bool bypassed) { bypassed_.store(bypassed); }
    bool IsBypassed() const { return bypassed_.load(); }
    void SetEnabled(bool enabled) { enabled_.store(enabled); }
    bool IsEnabled() const { return enabled_.load(); }
    
    // Parameter control
    bool SetParameter(uint32_t nodeId, uint32_t parameterIndex, float value);
    float GetParameter(uint32_t nodeId, uint32_t parameterIndex) const;
    
    // MIDI parameter mapping
    void SetMidiParameterMapper(std::shared_ptr<MidiParameterMapper> mapper);
    void ProcessMidiParameterControl(const MidiMessage& message);
    
    // Performance monitoring
    double GetCpuUsage() const { return cpuUsage_.load(); }
    uint32_t GetProcessedFrames() const { return processedFrames_.load(); }
    void ResetPerformanceCounters();
    
    // Audio format
    bool SetFormat(uint32_t sampleRate, uint32_t channels, uint32_t blockSize);
    void GetFormat(uint32_t& sampleRate, uint32_t& channels, uint32_t& blockSize) const;
    
    // Session management
    struct ChainState {
        struct NodeState {
            uint32_t nodeId;
            std::string pluginUri;
            uint32_t position;
            bool bypassed;
            std::map<std::string, std::string> pluginState;
            std::vector<uint32_t> inputChannels;
            std::vector<uint32_t> outputChannels;
        };
        
        std::vector<NodeState> nodes;
        bool bypassed;
        bool enabled;
    };
    
    ChainState SaveState() const;
    bool LoadState(const ChainState& state);
    
private:
    void ReorderChain();
    void UpdateAudioFormat();
    uint32_t GetNextNodeId();
    
    AudioEngine* audioEngine_;
    PluginManager* pluginManager_;
    
    // Chain nodes
    struct NodeInfo {
        uint32_t nodeId;
        uint32_t position;
        std::unique_ptr<ProcessingNode> node;
    };
    
    std::vector<NodeInfo> nodes_;
    mutable std::mutex nodesMutex_;
    
    // Audio format
    uint32_t sampleRate_;
    uint32_t channels_;
    uint32_t blockSize_;
    mutable std::mutex formatMutex_;
    
    // Chain state
    std::atomic<bool> bypassed_;
    std::atomic<bool> enabled_;
    
    // Performance monitoring
    std::atomic<double> cpuUsage_;
    std::atomic<uint32_t> processedFrames_;
    std::chrono::high_resolution_clock::time_point lastCpuMeasurement_;
    
    // MIDI parameter mapping
    std::shared_ptr<MidiParameterMapper> midiMapper_;
    
    // Internal buffers for chain processing
    std::vector<std::vector<float>> chainBuffers_;
    std::vector<float*> chainBufferPtrs_;
    
    // ID generation
    std::atomic<uint32_t> nextNodeId_;
    
    // Constants
    static constexpr double CPU_MEASUREMENT_INTERVAL = 1.0; // seconds
};

// Preset management for processing chains
class ChainPresetManager {
public:
    struct Preset {
        std::string name;
        std::string description;
        std::string author;
        std::string tags;
        AudioProcessingChain::ChainState chainState;
        std::chrono::system_clock::time_point createdTime;
        std::chrono::system_clock::time_point modifiedTime;
    };
    
    ChainPresetManager();
    ~ChainPresetManager();
    
    // Preset management
    bool SavePreset(const std::string& name, const AudioProcessingChain::ChainState& state, 
                   const std::string& description = "", const std::string& author = "");
    bool LoadPreset(const std::string& name, AudioProcessingChain::ChainState& state);
    bool DeletePreset(const std::string& name);
    bool RenamePreset(const std::string& oldName, const std::string& newName);
    
    // Preset discovery
    std::vector<std::string> GetPresetNames() const;
    std::vector<Preset> GetAllPresets() const;
    bool GetPresetInfo(const std::string& name, Preset& preset) const;
    
    // File operations
    bool ExportPreset(const std::string& name, const std::string& filePath);
    bool ImportPreset(const std::string& filePath, const std::string& newName = "");
    
    // Preset organization
    std::vector<std::string> GetTags() const;
    std::vector<std::string> GetPresetsByTag(const std::string& tag) const;
    
    // Utility
    std::string GetPresetsDirectory() const;
    bool CreatePresetsDirectory() const;
    
private:
    std::string GetPresetFilePath(const std::string& name) const;
    bool SerializePreset(const Preset& preset, const std::string& filePath);
    bool DeserializePreset(const std::string& filePath, Preset& preset);
    
    mutable std::mutex presetsMutex_;
    std::string presetsDirectory_;
};

} // namespace violet