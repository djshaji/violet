#include "violet/audio_processing_chain.h"
#include "violet/audio_engine.h"
#include "violet/utils.h"
#include <algorithm>
#include <chrono>
#include <iostream>

namespace violet {

// ProcessingNode implementation
ProcessingNode::ProcessingNode(std::unique_ptr<PluginInstance> plugin, uint32_t channels, uint32_t blockSize)
    : plugin_(std::move(plugin))
    , channels_(channels)
    , blockSize_(blockSize)
    , bypassed_(false) {
    
    if (plugin_) {
        const auto& info = plugin_->GetInfo();
        
        // Set up default channel routing
        for (uint32_t i = 0; i < info.audioInputs && i < channels_; ++i) {
            inputChannels_.push_back(i);
        }
        for (uint32_t i = 0; i < info.audioOutputs && i < channels_; ++i) {
            outputChannels_.push_back(i);
        }
        
        // Initialize control values - allocate for ALL control inputs
        controlValues_.resize(info.controlInputs, 0.0f);
        parameterChanged_.resize(info.controlInputs, false);
        
        // Set default values from parameter info
        auto parameters = plugin_->GetParameters();
        for (size_t i = 0; i < parameters.size() && i < controlValues_.size(); ++i) {
            controlValues_[i] = parameters[i].defaultValue;
        }
    }
    
    AllocateBuffers();
    ConnectPorts();
    
    // Activate plugin immediately after connecting ports
    // This is required by LV2 spec: all ports must be connected before activation
    if (plugin_) {
        std::cout << "Activating plugin in ProcessingNode constructor: " << plugin_->GetInfo().name << std::endl;
        plugin_->Activate();
    }
}

ProcessingNode::~ProcessingNode() {
    Deactivate();
}

void ProcessingNode::AllocateBuffers() {
    if (!plugin_) return;
    
    const auto& info = plugin_->GetInfo();
    
    std::cout << "Allocating buffers for plugin: inputs=" << info.audioInputs 
              << ", outputs=" << info.audioOutputs << ", blockSize=" << blockSize_ << std::endl;
    
    // Allocate input buffers
    inputBuffers_.resize(info.audioInputs);
    inputPtrs_.resize(info.audioInputs);
    for (uint32_t i = 0; i < info.audioInputs; ++i) {
        inputBuffers_[i].resize(blockSize_, 0.0f);
        inputPtrs_[i] = inputBuffers_[i].data();
        if (!inputPtrs_[i]) {
            std::cerr << "Error: Failed to allocate input buffer " << i << std::endl;
        }
    }
    
    // Allocate output buffers
    outputBuffers_.resize(info.audioOutputs);
    outputPtrs_.resize(info.audioOutputs);
    for (uint32_t i = 0; i < info.audioOutputs; ++i) {
        outputBuffers_[i].resize(blockSize_, 0.0f);
        outputPtrs_[i] = outputBuffers_[i].data();
        if (!outputPtrs_[i]) {
            std::cerr << "Error: Failed to allocate output buffer " << i << std::endl;
        }
    }
    
    // Allocate dummy buffers for control output ports (monitor ports)
    controlOutputDummy_.resize(info.controlOutputs, 0.0f);
    std::cout << "Allocated " << info.controlOutputs << " dummy control output ports" << std::endl;
}

void ProcessingNode::ConnectPorts() {
    if (!plugin_) return;
    
    const auto& info = plugin_->GetInfo();
    
    // Connect audio input ports
    for (uint32_t i = 0; i < info.audioInputs && i < inputPtrs_.size(); ++i) {
        plugin_->ConnectAudioInput(i, inputPtrs_[i]);
    }
    
    // Connect audio output ports
    for (uint32_t i = 0; i < info.audioOutputs && i < outputPtrs_.size(); ++i) {
        plugin_->ConnectAudioOutput(i, outputPtrs_[i]);
    }
    
    // Connect ALL control input ports
    for (uint32_t i = 0; i < info.controlInputs; ++i) {
        if (i < controlValues_.size()) {
            plugin_->ConnectControlInput(i, &controlValues_[i]);
        }
    }
    
    // Connect control output ports (monitor/meter ports) to dummy buffers
    for (uint32_t i = 0; i < info.controlOutputs && i < controlOutputDummy_.size(); ++i) {
        plugin_->ConnectControlOutput(i, &controlOutputDummy_[i]);
    }
}

bool ProcessingNode::IsActive() const {
    return plugin_ && plugin_->IsActive();
}

bool ProcessingNode::Activate() {
    if (plugin_) {
        return plugin_->Activate();
    }
    return false;
}

void ProcessingNode::Deactivate() {
    if (plugin_) {
        plugin_->Deactivate();
    }
}

void ProcessingNode::Process(float** inputBuffers, float** outputBuffers, uint32_t frames) {
    if (!plugin_ || bypassed_.load() || !IsActive()) {
        // Bypass: copy input to output
        for (uint32_t ch = 0; ch < std::min(inputChannels_.size(), outputChannels_.size()); ++ch) {
            if (inputChannels_[ch] < channels_ && outputChannels_[ch] < channels_) {
                if (inputBuffers && outputBuffers && inputBuffers[inputChannels_[ch]] && outputBuffers[outputChannels_[ch]]) {
                    memcpy(outputBuffers[outputChannels_[ch]], 
                           inputBuffers[inputChannels_[ch]], 
                           frames * sizeof(float));
                }
            }
        }
        return;
    }
    
    const auto& info = plugin_->GetInfo();
    
    // Process in chunks if frames exceeds blockSize
    uint32_t framesProcessed = 0;
    while (framesProcessed < frames) {
        uint32_t framesToProcess = std::min(frames - framesProcessed, blockSize_);
        
        // Copy input data to plugin input buffers with validation
        for (uint32_t i = 0; i < info.audioInputs && i < inputChannels_.size() && i < inputPtrs_.size(); ++i) {
            if (!inputPtrs_[i]) {
                std::cerr << "Error: inputPtrs_[" << i << "] is NULL!" << std::endl;
                continue;
            }
            
            uint32_t srcChannel = inputChannels_[i];
            if (srcChannel < channels_ && inputBuffers && inputBuffers[srcChannel]) {
                memcpy(inputPtrs_[i], inputBuffers[srcChannel] + framesProcessed, framesToProcess * sizeof(float));
            } else {
                // No input available, clear buffer
                memset(inputPtrs_[i], 0, framesToProcess * sizeof(float));
            }
        }
        
        // Process parameter changes
        ProcessParameterChanges();
        
        // Process automation
        ProcessAutomation(framesProcessed, framesToProcess);
        
        // Run the plugin
        plugin_->Process(framesToProcess);
        
        // Copy output data from plugin output buffers with validation
        for (uint32_t i = 0; i < info.audioOutputs && i < outputChannels_.size() && i < outputPtrs_.size(); ++i) {
            if (!outputPtrs_[i]) {
                std::cerr << "Error: outputPtrs_[" << i << "] is NULL!" << std::endl;
                continue;
            }
            
            uint32_t dstChannel = outputChannels_[i];
            if (dstChannel < channels_ && outputBuffers && outputBuffers[dstChannel]) {
                memcpy(outputBuffers[dstChannel] + framesProcessed, outputPtrs_[i], framesToProcess * sizeof(float));
            }
        }
        
        framesProcessed += framesToProcess;
    }
}

void ProcessingNode::ProcessMidi(MidiBuffer* midiBuffer, uint32_t frames) {
    // TODO: Implement MIDI processing for plugins that support it
    // This would involve converting MidiBuffer to LV2 Atom format
}

void ProcessingNode::ProcessParameterChanges() {
    for (size_t i = 0; i < parameterChanged_.size(); ++i) {
        if (parameterChanged_[i]) {
            plugin_->SetParameter(static_cast<uint32_t>(i), controlValues_[i]);
            parameterChanged_[i] = false;
        }
    }
}

void ProcessingNode::ProcessAutomation(uint32_t currentSample, uint32_t frames) {
    std::lock_guard<std::mutex> lock(automationMutex_);
    
    // Process automation points within the current frame range
    for (const auto& point : automationPoints_) {
        if (point.sampleTime >= currentSample && point.sampleTime < currentSample + frames) {
            if (point.parameterIndex < controlValues_.size()) {
                controlValues_[point.parameterIndex] = point.value;
                parameterChanged_[point.parameterIndex] = true;
            }
        }
    }
    
    // Remove processed automation points
    automationPoints_.erase(
        std::remove_if(automationPoints_.begin(), automationPoints_.end(),
                      [currentSample, frames](const AutomationPoint& point) {
                          return point.sampleTime < currentSample + frames;
                      }),
        automationPoints_.end());
}

void ProcessingNode::AddAutomationPoint(const AutomationPoint& point) {
    std::lock_guard<std::mutex> lock(automationMutex_);
    automationPoints_.push_back(point);
    
    // Keep automation points sorted by time
    std::sort(automationPoints_.begin(), automationPoints_.end(),
             [](const AutomationPoint& a, const AutomationPoint& b) {
                 return a.sampleTime < b.sampleTime;
             });
}

void ProcessingNode::ClearAutomation() {
    std::lock_guard<std::mutex> lock(automationMutex_);
    automationPoints_.clear();
}

void ProcessingNode::SetInputChannels(const std::vector<uint32_t>& channels) {
    inputChannels_ = channels;
}

void ProcessingNode::SetOutputChannels(const std::vector<uint32_t>& channels) {
    outputChannels_ = channels;
}

// AudioProcessingChain implementation
AudioProcessingChain::AudioProcessingChain(AudioEngine* audioEngine)
    : audioEngine_(audioEngine)
    , pluginManager_(nullptr)
    , sampleRate_(44100)
    , channels_(2)
    , blockSize_(256)
    , bypassed_(false)
    , enabled_(true)
    , cpuUsage_(0.0)
    , processedFrames_(0)
    , nextNodeId_(1) {
    
    // TODO: Get plugin manager from audio engine or create one
    pluginManager_ = new PluginManager();
    if (pluginManager_) {
        pluginManager_->Initialize();
    }
}

AudioProcessingChain::~AudioProcessingChain() {
    ClearChain();
    
    if (pluginManager_) {
        pluginManager_->Shutdown();
        delete pluginManager_;
    }
}

uint32_t AudioProcessingChain::AddPlugin(const std::string& pluginUri, uint32_t position) {
    if (!pluginManager_) {
        return 0;
    }
    
    // Create plugin instance
    auto pluginInstance = pluginManager_->CreatePlugin(pluginUri, sampleRate_, blockSize_);
    if (!pluginInstance) {
        std::cerr << "Failed to create plugin: " << pluginUri << std::endl;
        return 0;
    }
    
    // Create processing node
    auto node = std::make_unique<ProcessingNode>(std::move(pluginInstance), channels_, blockSize_);
    if (!node->Activate()) {
        std::cerr << "Failed to activate plugin: " << pluginUri << std::endl;
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    uint32_t nodeId = GetNextNodeId();
    
    // Determine position
    if (position == UINT_MAX) {
        position = static_cast<uint32_t>(nodes_.size());
    }
    
    NodeInfo nodeInfo;
    nodeInfo.nodeId = nodeId;
    nodeInfo.position = position;
    nodeInfo.node = std::move(node);
    
    nodes_.push_back(std::move(nodeInfo));
    
    // Reorder chain
    ReorderChain();
    
    return nodeId;
}

bool AudioProcessingChain::RemovePlugin(uint32_t nodeId) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
                          [nodeId](const NodeInfo& info) {
                              return info.nodeId == nodeId;
                          });
    
    if (it != nodes_.end()) {
        nodes_.erase(it);
        ReorderChain();
        return true;
    }
    
    return false;
}

bool AudioProcessingChain::MovePlugin(uint32_t nodeId, uint32_t newPosition) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
                          [nodeId](NodeInfo& info) {
                              return info.nodeId == nodeId;
                          });
    
    if (it != nodes_.end()) {
        it->position = newPosition;
        ReorderChain();
        return true;
    }
    
    return false;
}

void AudioProcessingChain::ClearChain() {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    nodes_.clear();
}

ProcessingNode* AudioProcessingChain::GetNode(uint32_t nodeId) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
                          [nodeId](const NodeInfo& info) {
                              return info.nodeId == nodeId;
                          });
    
    return (it != nodes_.end()) ? it->node.get() : nullptr;
}

const ProcessingNode* AudioProcessingChain::GetNode(uint32_t nodeId) const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
                          [nodeId](const NodeInfo& info) {
                              return info.nodeId == nodeId;
                          });
    
    return (it != nodes_.end()) ? it->node.get() : nullptr;
}

std::vector<uint32_t> AudioProcessingChain::GetNodeIds() const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    std::vector<uint32_t> ids;
    for (const auto& info : nodes_) {
        ids.push_back(info.nodeId);
    }
    
    return ids;
}

uint32_t AudioProcessingChain::GetNodeCount() const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    return static_cast<uint32_t>(nodes_.size());
}

void AudioProcessingChain::Process(float** inputBuffers, float** outputBuffers, uint32_t channels, uint32_t frames) {
    if (!enabled_.load() || bypassed_.load()) {
        // Bypass: copy input to output
        for (uint32_t ch = 0; ch < channels; ++ch) {
            memcpy(outputBuffers[ch], inputBuffers[ch], frames * sizeof(float));
        }
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    if (nodes_.empty()) {
        // No plugins: copy input to output
        for (uint32_t ch = 0; ch < channels; ++ch) {
            memcpy(outputBuffers[ch], inputBuffers[ch], frames * sizeof(float));
        }
        return;
    }
    
    // Allocate chain buffers if needed
    if (chainBuffers_.size() != channels || chainBuffers_[0].size() != frames) {
        chainBuffers_.resize(channels);
        chainBufferPtrs_.resize(channels);
        for (uint32_t ch = 0; ch < channels; ++ch) {
            chainBuffers_[ch].resize(frames);
            chainBufferPtrs_[ch] = chainBuffers_[ch].data();
        }
    }
    
    // Copy input to chain buffers
    for (uint32_t ch = 0; ch < channels; ++ch) {
        memcpy(chainBufferPtrs_[ch], inputBuffers[ch], frames * sizeof(float));
    }
    
    // Process through chain
    for (const auto& nodeInfo : nodes_) {
        if (nodeInfo.node && nodeInfo.node->IsActive()) {
            nodeInfo.node->Process(chainBufferPtrs_.data(), chainBufferPtrs_.data(), frames);
        }
    }
    
    // Copy chain buffers to output
    for (uint32_t ch = 0; ch < channels; ++ch) {
        memcpy(outputBuffers[ch], chainBufferPtrs_[ch], frames * sizeof(float));
    }
    
    processedFrames_.fetch_add(frames);
    
    // Update CPU usage periodically
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(endTime - lastCpuMeasurement_).count();
    if (elapsed >= CPU_MEASUREMENT_INTERVAL) {
        auto processingTime = std::chrono::duration<double>(endTime - startTime).count();
        double frameTime = static_cast<double>(frames) / sampleRate_;
        cpuUsage_.store((processingTime / frameTime) * 100.0);
        lastCpuMeasurement_ = endTime;
    }
}

void AudioProcessingChain::ProcessMidi(MidiBuffer* midiBuffer, uint32_t frames) {
    if (!enabled_.load() || !midiBuffer) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    // Process MIDI through each node
    for (const auto& nodeInfo : nodes_) {
        if (nodeInfo.node && nodeInfo.node->IsActive()) {
            nodeInfo.node->ProcessMidi(midiBuffer, frames);
        }
    }
}

bool AudioProcessingChain::SetParameter(uint32_t nodeId, uint32_t parameterIndex, float value) {
    ProcessingNode* node = GetNode(nodeId);
    if (node && node->GetPlugin()) {
        node->GetPlugin()->SetParameter(parameterIndex, value);
        return true;
    }
    return false;
}

float AudioProcessingChain::GetParameter(uint32_t nodeId, uint32_t parameterIndex) const {
    const ProcessingNode* node = GetNode(nodeId);
    if (node && node->GetPlugin()) {
        return node->GetPlugin()->GetParameter(parameterIndex);
    }
    return 0.0f;
}

void AudioProcessingChain::SetMidiParameterMapper(std::shared_ptr<MidiParameterMapper> mapper) {
    midiMapper_ = mapper;
}

void AudioProcessingChain::ProcessMidiParameterControl(const MidiMessage& message) {
    if (!midiMapper_ || !message.IsControlChange()) {
        return;
    }
    
    MidiParameterMapper::ParameterMapping mapping;
    if (midiMapper_->FindMapping(message.GetChannel(), message.data1, mapping)) {
        float value = midiMapper_->ControlChangeToParameter(message, mapping);
        
        // Find the node that contains this parameter
        // For now, we'll assume parameter indices are global across all nodes
        // TODO: Implement proper node-specific parameter mapping
        for (const auto& nodeId : GetNodeIds()) {
            if (SetParameter(nodeId, mapping.parameterIndex, value)) {
                break; // Parameter was found and set
            }
        }
    }
}

bool AudioProcessingChain::SetFormat(uint32_t sampleRate, uint32_t channels, uint32_t blockSize) {
    std::lock_guard<std::mutex> lock(formatMutex_);
    
    if (sampleRate_ == sampleRate && channels_ == channels && blockSize_ == blockSize) {
        return true; // No change needed
    }
    
    sampleRate_ = sampleRate;
    channels_ = channels;
    blockSize_ = blockSize;
    
    UpdateAudioFormat();
    return true;
}

void AudioProcessingChain::GetFormat(uint32_t& sampleRate, uint32_t& channels, uint32_t& blockSize) const {
    std::lock_guard<std::mutex> lock(formatMutex_);
    sampleRate = sampleRate_;
    channels = channels_;
    blockSize = blockSize_;
}

void AudioProcessingChain::ReorderChain() {
    // Sort nodes by position
    std::sort(nodes_.begin(), nodes_.end(),
             [](const NodeInfo& a, const NodeInfo& b) {
                 return a.position < b.position;
             });
}

void AudioProcessingChain::UpdateAudioFormat() {
    // TODO: Recreate nodes with new format
    // This would require stopping audio, recreating all plugin instances,
    // and restarting audio
}

uint32_t AudioProcessingChain::GetNextNodeId() {
    return nextNodeId_.fetch_add(1);
}

void AudioProcessingChain::ResetPerformanceCounters() {
    cpuUsage_.store(0.0);
    processedFrames_.store(0);
    lastCpuMeasurement_ = std::chrono::high_resolution_clock::now();
}

// Simplified preset manager implementation
ChainPresetManager::ChainPresetManager() {
    presetsDirectory_ = utils::GetExecutableDirectory() + "\\presets";
    CreatePresetsDirectory();
}

ChainPresetManager::~ChainPresetManager() = default;

bool ChainPresetManager::SavePreset(const std::string& name, const AudioProcessingChain::ChainState& state, 
                                   const std::string& description, const std::string& author) {
    // TODO: Implement preset serialization
    // For now, return true as placeholder
    return true;
}

bool ChainPresetManager::LoadPreset(const std::string& name, AudioProcessingChain::ChainState& state) {
    // TODO: Implement preset deserialization
    // For now, return false as placeholder
    return false;
}

std::vector<std::string> ChainPresetManager::GetPresetNames() const {
    // TODO: Implement preset discovery
    return std::vector<std::string>();
}

std::string ChainPresetManager::GetPresetsDirectory() const {
    return presetsDirectory_;
}

bool ChainPresetManager::CreatePresetsDirectory() const {
    return CreateDirectoryA(presetsDirectory_.c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

} // namespace violet