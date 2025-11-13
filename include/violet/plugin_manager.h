#pragma once

#include <lilv/lilv.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <windows.h>
#include "violet/audio_buffer.h"

namespace violet {

// Plugin information
struct PluginInfo {
    std::string uri;
    std::string name;
    std::string author;
    std::string category;
    std::string description;
    bool hasUI;
    uint32_t audioInputs;
    uint32_t audioOutputs;
    uint32_t controlInputs;
    uint32_t controlOutputs;
    uint32_t midiInputs;
    uint32_t midiOutputs;
};

// Plugin parameter information
struct ParameterInfo {
    uint32_t index = 0;        // Ordinal within control inputs
    uint32_t portIndex = 0;    // Actual LV2 port index
    std::string symbol;
    std::string name;
    float defaultValue = 0.0f;
    float minimum = 0.0f;
    float maximum = 1.0f;
    bool isToggle = false;
    bool isInteger = false;
    bool isEnum = false;
    std::vector<std::string> enumValues;
};

// Plugin instance
class PluginInstance {
public:
    PluginInstance(const LilvPlugin* plugin, LilvWorld* world, double sampleRate, uint32_t blockSize);
    ~PluginInstance();
    
    // Plugin control
    bool Activate();
    void Deactivate();
    bool IsActive() const { return isActive_; }
    
    // Audio processing
    void Process(uint32_t frames);
    
    // Port management
    void ConnectAudioInput(uint32_t port, float* buffer);
    void ConnectAudioOutput(uint32_t port, float* buffer);
    void ConnectControlInput(uint32_t port, float* value);
    void ConnectControlOutput(uint32_t port, float* value);
    void ConnectMidiInput(uint32_t port, LV2_Atom_Sequence* buffer);
    void ConnectMidiOutput(uint32_t port, LV2_Atom_Sequence* buffer);
    
    // Parameter control
    void SetParameter(uint32_t index, float value);
    float GetParameter(uint32_t index) const;
    std::vector<ParameterInfo> GetParameters() const;
    
    // State management
    bool SaveState(std::map<std::string, std::string>& state);
    bool LoadState(const std::map<std::string, std::string>& state);
    
    // Plugin info
    const PluginInfo& GetInfo() const { return info_; }
    const LilvPlugin* GetLilvPlugin() const { return plugin_; }
    
private:
    void InitializePorts();
    void InitializeFeatures();
    void CleanupFeatures();
    
    const LilvPlugin* plugin_;
    LilvWorld* world_;
    LilvInstance* instance_;
    PluginInfo info_;
    
    double sampleRate_;
    uint32_t blockSize_;
    bool isActive_;
    
    // Port information
    std::vector<uint32_t> audioInputPorts_;
    std::vector<uint32_t> audioOutputPorts_;
    std::vector<uint32_t> controlInputPorts_;
    std::vector<uint32_t> controlOutputPorts_;
    std::vector<uint32_t> midiInputPorts_;
    std::vector<uint32_t> midiOutputPorts_;
    
    // Control values
    std::vector<float> controlValues_;
    std::map<uint32_t, ParameterInfo> parameterInfo_;
    
    // LV2 features
    std::vector<const LV2_Feature*> features_;
    LV2_URID_Map uridMap_;
    LV2_URID_Unmap uridUnmap_;
    
    // URID mappings
    std::map<std::string, LV2_URID> uridMappings_;
    std::map<LV2_URID, std::string> uridReverseMappings_;
    LV2_URID nextUrid_;
    
    // Feature implementations
    static LV2_URID MapUrid(LV2_URID_Map_Handle handle, const char* uri);
    static const char* UnmapUrid(LV2_URID_Unmap_Handle handle, LV2_URID urid);
};

// Plugin manager for discovering and managing LV2 plugins
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Plugin discovery
    void ScanPlugins();
    void ScanDirectory(const std::string& directory);
    std::vector<PluginInfo> GetAvailablePlugins() const;
    std::vector<PluginInfo> GetPluginsByCategory(const std::string& category) const;
    std::vector<std::string> GetCategories() const;
    
    // Plugin instantiation
    std::unique_ptr<PluginInstance> CreatePlugin(const std::string& uri, double sampleRate, uint32_t blockSize);
    
    // Plugin information
    PluginInfo GetPluginInfo(const std::string& uri) const;
    bool IsPluginAvailable(const std::string& uri) const;
    
    // Utility
    std::vector<std::string> GetDefaultScanPaths() const;
    void AddScanPath(const std::string& path);
    void RemoveScanPath(const std::string& path);
    
private:
    void InitializeLilv();
    void ShutdownLilv();
    PluginInfo ExtractPluginInfo(const LilvPlugin* plugin);
    std::string GetPluginCategory(const LilvPlugin* plugin);
    
    LilvWorld* world_;
    const LilvPlugins* plugins_;
    
    std::vector<PluginInfo> availablePlugins_;
    std::map<std::string, const LilvPlugin*> pluginMap_;
    std::vector<std::string> scanPaths_;
    std::vector<std::string> categories_;
    
    bool isInitialized_;
};

} // namespace violet