#include "violet/plugin_manager.h"
#include "violet/utils.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace violet {

// PluginInstance implementation
PluginInstance::PluginInstance(const LilvPlugin* plugin, LilvWorld* world, double sampleRate, uint32_t blockSize)
    : plugin_(plugin)
    , world_(world)
    , instance_(nullptr)
    , sampleRate_(sampleRate)
    , blockSize_(blockSize)
    , isActive_(false)
    , nextUrid_(1) {
    
    // Extract plugin info
    LilvNode* nameNode = const_cast<LilvNode*>(lilv_plugin_get_name(plugin_));
    info_.name = nameNode ? lilv_node_as_string(nameNode) : "Unknown";
    if (nameNode) lilv_node_free(nameNode);
    
    info_.uri = lilv_node_as_string(lilv_plugin_get_uri(plugin_));
    
    LilvNode* authorNode = const_cast<LilvNode*>(lilv_plugin_get_author_name(plugin_));
    info_.author = authorNode ? lilv_node_as_string(authorNode) : "Unknown";
    if (authorNode) lilv_node_free(authorNode);
    
    // Initialize features and ports
    InitializeFeatures();
    InitializePorts();
    
    // Create instance
    instance_ = lilv_plugin_instantiate(plugin_, sampleRate_, features_.data());
    if (!instance_) {
        std::cerr << "Failed to instantiate plugin: " << info_.name << std::endl;
    }
}

PluginInstance::~PluginInstance() {
    Deactivate();
    
    if (instance_) {
        lilv_instance_free(instance_);
    }
    
    CleanupFeatures();
}

void PluginInstance::InitializeFeatures() {
    // Set up URID map/unmap
    uridMap_.handle = this;
    uridMap_.map = MapUrid;
    
    uridUnmap_.handle = this;
    uridUnmap_.unmap = UnmapUrid;
    
    // Create feature list
    features_.clear();
    
    static LV2_Feature uridMapFeature = { LV2_URID__map, &uridMap_ };
    static LV2_Feature uridUnmapFeature = { LV2_URID__unmap, &uridUnmap_ };
    
    features_.push_back(&uridMapFeature);
    features_.push_back(&uridUnmapFeature);
    features_.push_back(nullptr); // Null terminator
}

void PluginInstance::CleanupFeatures() {
    features_.clear();
}

LV2_URID PluginInstance::MapUrid(LV2_URID_Map_Handle handle, const char* uri) {
    PluginInstance* instance = static_cast<PluginInstance*>(handle);
    
    auto it = instance->uridMappings_.find(uri);
    if (it != instance->uridMappings_.end()) {
        return it->second;
    }
    
    LV2_URID urid = instance->nextUrid_++;
    instance->uridMappings_[uri] = urid;
    instance->uridReverseMappings_[urid] = uri;
    
    return urid;
}

const char* PluginInstance::UnmapUrid(LV2_URID_Unmap_Handle handle, LV2_URID urid) {
    PluginInstance* instance = static_cast<PluginInstance*>(handle);
    
    auto it = instance->uridReverseMappings_.find(urid);
    if (it != instance->uridReverseMappings_.end()) {
        return it->second.c_str();
    }
    
    return nullptr;
}

void PluginInstance::InitializePorts() {
    if (!plugin_) return;
    
    uint32_t numPorts = lilv_plugin_get_num_ports(plugin_);
    controlValues_.resize(numPorts, 0.0f);
    
    info_.audioInputs = 0;
    info_.audioOutputs = 0;
    info_.controlInputs = 0;
    info_.controlOutputs = 0;
    info_.midiInputs = 0;
    info_.midiOutputs = 0;
    
    std::cout << "=== Enumerating ports for plugin: " << info_.name << " (total ports: " << numPorts << ") ===" << std::endl;
    
    for (uint32_t i = 0; i < numPorts; ++i) {
        const LilvPort* port = lilv_plugin_get_port_by_index(plugin_, i);
        
        // Check port type
        if (lilv_port_is_a(plugin_, port, lilv_new_uri(world_, LV2_CORE__AudioPort))) {
            if (lilv_port_is_a(plugin_, port, lilv_new_uri(world_, LV2_CORE__InputPort))) {
                std::cout << "  Port " << i << ": Audio Input" << std::endl;
                audioInputPorts_.push_back(i);
                info_.audioInputs++;
            } else if (lilv_port_is_a(plugin_, port, lilv_new_uri(world_, LV2_CORE__OutputPort))) {
                std::cout << "  Port " << i << ": Audio Output" << std::endl;
                audioOutputPorts_.push_back(i);
                info_.audioOutputs++;
            }
        } else if (lilv_port_is_a(plugin_, port, lilv_new_uri(world_, LV2_CORE__ControlPort))) {
            if (lilv_port_is_a(plugin_, port, lilv_new_uri(world_, LV2_CORE__InputPort))) {
                std::cout << "  Port " << i << ": Control Input" << std::endl;
                controlInputPorts_.push_back(i);
                info_.controlInputs++;
                
                // Extract parameter info
                ParameterInfo paramInfo;
                paramInfo.index = static_cast<uint32_t>(controlInputPorts_.size() - 1); // ordinal index
                paramInfo.portIndex = i;
                
                const LilvNode* symbolNode = lilv_port_get_symbol(plugin_, port);
                paramInfo.symbol = symbolNode ? lilv_node_as_string(symbolNode) : "";
                
                LilvNode* nameNode = const_cast<LilvNode*>(lilv_port_get_name(plugin_, port));
                paramInfo.name = nameNode ? lilv_node_as_string(nameNode) : paramInfo.symbol;
                if (nameNode) lilv_node_free(nameNode);
                
                std::cout << "    Name: " << paramInfo.name << ", Symbol: " << paramInfo.symbol << std::endl;
                
                // Get default, min, max values from TTL file
                LilvNode* defaultNode = nullptr;
                LilvNode* minNode = nullptr;
                LilvNode* maxNode = nullptr;
                
                lilv_port_get_range(plugin_, port, &defaultNode, &minNode, &maxNode);
                
                paramInfo.defaultValue = defaultNode ? lilv_node_as_float(defaultNode) : 0.0f;
                paramInfo.minimum = minNode ? lilv_node_as_float(minNode) : 0.0f;
                paramInfo.maximum = maxNode ? lilv_node_as_float(maxNode) : 1.0f;
                
                std::cout << "    Range: min=" << paramInfo.minimum 
                          << ", max=" << paramInfo.maximum 
                          << ", default=" << paramInfo.defaultValue << std::endl;
                
                if (defaultNode) lilv_node_free(defaultNode);
                if (minNode) lilv_node_free(minNode);
                if (maxNode) lilv_node_free(maxNode);
                
                // Check for toggle or integer properties
                paramInfo.isToggle = lilv_port_has_property(plugin_, port, 
                    lilv_new_uri(world_, LV2_CORE__toggled));
                paramInfo.isInteger = lilv_port_has_property(plugin_, port, 
                    lilv_new_uri(world_, LV2_CORE__integer));
                paramInfo.isEnum = false; // TODO: Implement enumeration detection
                
                parameterInfo_[i] = paramInfo;
                controlValues_[i] = paramInfo.defaultValue;
                
            } else if (lilv_port_is_a(plugin_, port, lilv_new_uri(world_, LV2_CORE__OutputPort))) {
                std::cout << "  Port " << i << ": Control Output" << std::endl;
                controlOutputPorts_.push_back(i);
                info_.controlOutputs++;
            }
        }
        // TODO: Add MIDI port detection when we implement MIDI support
    }
}

bool PluginInstance::Activate() {
    if (!instance_ || isActive_) {
        return isActive_;
    }
    
    lilv_instance_activate(instance_);
    isActive_ = true;
    return true;
}

void PluginInstance::Deactivate() {
    if (!instance_ || !isActive_) {
        return;
    }
    
    lilv_instance_deactivate(instance_);
    isActive_ = false;
}

void PluginInstance::Process(uint32_t frames) {
    if (!instance_ || !isActive_) {
        return;
    }
    
    lilv_instance_run(instance_, frames);
}

void PluginInstance::ConnectAudioInput(uint32_t port, float* buffer) {
    if (!instance_) {
        std::cerr << "Error: instance_ is NULL in ConnectAudioInput" << std::endl;
        return;
    }
    
    if (port >= audioInputPorts_.size()) {
        std::cerr << "Error: audio input port " << port << " out of range (size=" << audioInputPorts_.size() << ")" << std::endl;
        return;
    }
    
    if (!buffer) {
        std::cerr << "Error: NULL buffer passed to ConnectAudioInput port " << port << std::endl;
        return;
    }
    
    uint32_t actualPortIndex = audioInputPorts_[port];
    std::cout << "Connecting audio input port " << port << " (actual LV2 port " << actualPortIndex << ") to buffer " << (void*)buffer << std::endl;
    lilv_instance_connect_port(instance_, actualPortIndex, buffer);
}

void PluginInstance::ConnectAudioOutput(uint32_t port, float* buffer) {
    if (!instance_) {
        std::cerr << "Error: instance_ is NULL in ConnectAudioOutput" << std::endl;
        return;
    }
    
    if (port >= audioOutputPorts_.size()) {
        std::cerr << "Error: audio output port " << port << " out of range (size=" << audioOutputPorts_.size() << ")" << std::endl;
        return;
    }
    
    if (!buffer) {
        std::cerr << "Error: NULL buffer passed to ConnectAudioOutput port " << port << std::endl;
        return;
    }
    
    uint32_t actualPortIndex = audioOutputPorts_[port];
    std::cout << "Connecting audio output port " << port << " (actual LV2 port " << actualPortIndex << ") to buffer " << (void*)buffer << std::endl;
    lilv_instance_connect_port(instance_, actualPortIndex, buffer);
}

void PluginInstance::ConnectControlInput(uint32_t port, float* value) {
    if (!instance_) {
        std::cerr << "Error: instance_ is NULL in ConnectControlInput" << std::endl;
        return;
    }
    
    if (port >= controlInputPorts_.size()) {
        std::cerr << "Error: control input port " << port << " out of range (size=" << controlInputPorts_.size() << ")" << std::endl;
        return;
    }
    
    if (!value) {
        std::cerr << "Error: NULL value passed to ConnectControlInput port " << port << std::endl;
        return;
    }
    
    uint32_t actualPortIndex = controlInputPorts_[port];
    std::cout << "Connecting control input port " << port << " (actual LV2 port " << actualPortIndex << ") to value " << (void*)value << std::endl;
    lilv_instance_connect_port(instance_, actualPortIndex, value);
}

void PluginInstance::ConnectControlOutput(uint32_t port, float* value) {
    if (!instance_) {
        std::cerr << "Error: instance_ is NULL in ConnectControlOutput" << std::endl;
        return;
    }
    
    if (port >= controlOutputPorts_.size()) {
        std::cerr << "Error: control output port " << port << " out of range (size=" << controlOutputPorts_.size() << ")" << std::endl;
        return;
    }
    
    if (!value) {
        std::cerr << "Error: NULL value passed to ConnectControlOutput port " << port << std::endl;
        return;
    }
    
    uint32_t actualPortIndex = controlOutputPorts_[port];
    std::cout << "Connecting control output port " << port << " (actual LV2 port " << actualPortIndex << ") to dummy buffer " << (void*)value << std::endl;
    lilv_instance_connect_port(instance_, actualPortIndex, value);
}

void PluginInstance::ConnectMidiInput(uint32_t port, LV2_Atom_Sequence* buffer) {
    if (instance_) {
        lilv_instance_connect_port(instance_, port, buffer);
    }
}

void PluginInstance::ConnectMidiOutput(uint32_t port, LV2_Atom_Sequence* buffer) {
    if (instance_) {
        lilv_instance_connect_port(instance_, port, buffer);
    }
}

void PluginInstance::SetParameter(uint32_t index, float value) {
    if (index >= controlInputPorts_.size()) {
        return;
    }

    uint32_t portIndex = controlInputPorts_[index];

    auto it = parameterInfo_.find(portIndex);
    if (it != parameterInfo_.end()) {
        const ParameterInfo& info = it->second;
        value = std::max(info.minimum, std::min(info.maximum, value));
        if (info.isInteger) {
            value = std::round(value);
        }
    }

    if (portIndex < controlValues_.size()) {
        controlValues_[portIndex] = value;
    }
}

float PluginInstance::GetParameter(uint32_t index) const {
    if (index >= controlInputPorts_.size()) {
        return 0.0f;
    }

    uint32_t portIndex = controlInputPorts_[index];
    if (portIndex < controlValues_.size()) {
        return controlValues_[portIndex];
    }
    return 0.0f;
}

std::vector<ParameterInfo> PluginInstance::GetParameters() const {
    std::vector<ParameterInfo> params;
    params.reserve(controlInputPorts_.size());
    for (size_t ordinal = 0; ordinal < controlInputPorts_.size(); ++ordinal) {
        uint32_t portIndex = controlInputPorts_[ordinal];
        auto it = parameterInfo_.find(portIndex);
        if (it != parameterInfo_.end()) {
            ParameterInfo info = it->second;
            info.index = static_cast<uint32_t>(ordinal);
            info.portIndex = portIndex;
            params.push_back(std::move(info));
        }
    }
    return params;
}

bool PluginInstance::SaveState(std::map<std::string, std::string>& state) {
    // Save control parameter values
    for (const auto& pair : parameterInfo_) {
        uint32_t index = pair.first;
        const ParameterInfo& info = pair.second;
        state[info.symbol] = std::to_string(controlValues_[index]);
    }
    
    // TODO: Implement LV2 state extension support
    return true;
}

bool PluginInstance::LoadState(const std::map<std::string, std::string>& state) {
    // Load control parameter values
    for (const auto& pair : parameterInfo_) {
        uint32_t index = pair.first;
        const ParameterInfo& info = pair.second;
        
        auto it = state.find(info.symbol);
        if (it != state.end()) {
            try {
                float value = std::stof(it->second);
                SetParameter(index, value);
            } catch (const std::exception&) {
                // Ignore invalid values
            }
        }
    }
    
    // TODO: Implement LV2 state extension support
    return true;
}

// PluginManager implementation
PluginManager::PluginManager()
    : world_(nullptr)
    , plugins_(nullptr)
    , isInitialized_(false) {
}

PluginManager::~PluginManager() {
    Shutdown();
}

bool PluginManager::Initialize() {
    if (isInitialized_) {
        return true;
    }
    
    InitializeLilv();
    
    if (!world_) {
        return false;
    }
    
    ScanPlugins();
    isInitialized_ = true;
    
    return true;
}

void PluginManager::Shutdown() {
    if (!isInitialized_) {
        return;
    }
    
    availablePlugins_.clear();
    pluginMap_.clear();
    categories_.clear();
    
    ShutdownLilv();
    isInitialized_ = false;
}

void PluginManager::InitializeLilv() {
    world_ = lilv_world_new();
    if (!world_) {
        std::cerr << "Failed to create LILV world" << std::endl;
        return;
    }
    
    // Set LV2_PATH environment variable to current directory
    char currentDir[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, currentDir)) {
        std::string lv2Path = "LV2_PATH=" + std::string(currentDir) + "/lv2";
        putenv(const_cast<char*>(lv2Path.c_str()));
        std::cout << "LV2_PATH set to: " << currentDir << std::endl;
    } else {
        std::cerr << "Failed to get current directory" << std::endl;
    }

    lilv_world_load_all(world_);
    plugins_ = lilv_world_get_all_plugins(world_);
    
    // Print all found plugins
    if (plugins_) {
        unsigned int pluginCount = lilv_plugins_size(plugins_);
        std::cout << "Found " << pluginCount << " LV2 plugins:" << std::endl;
        
        LILV_FOREACH(plugins, iter, plugins_) {
            const LilvPlugin* plugin = lilv_plugins_get(plugins_, iter);
            const LilvNode* nameNode = lilv_plugin_get_name(plugin);
            const LilvNode* uriNode = lilv_plugin_get_uri(plugin);
            
            std::string name = nameNode ? lilv_node_as_string(nameNode) : "Unknown";
            std::string uri = uriNode ? lilv_node_as_string(uriNode) : "Unknown";
            
            std::cout << "  - " << name << " (" << uri << ")" << std::endl;
            
            if (nameNode) lilv_node_free(const_cast<LilvNode*>(nameNode));
        }
    } else {
        std::cout << "No LV2 plugins found" << std::endl;
    }
}

void PluginManager::ShutdownLilv() {
    if (world_) {
        lilv_world_free(world_);
        world_ = nullptr;
        plugins_ = nullptr;
    }
}

void PluginManager::ScanPlugins() {
    if (!plugins_) {
        return;
    }
    
    availablePlugins_.clear();
    pluginMap_.clear();
    categories_.clear();
    
    LILV_FOREACH(plugins, iter, plugins_) {
        const LilvPlugin* plugin = lilv_plugins_get(plugins_, iter);
        PluginInfo info = ExtractPluginInfo(plugin);
        
        availablePlugins_.push_back(info);
        pluginMap_[info.uri] = plugin;
        
        // Add category if not already present
        if (std::find(categories_.begin(), categories_.end(), info.category) == categories_.end()) {
            categories_.push_back(info.category);
        }
    }
    
    std::sort(categories_.begin(), categories_.end());
}

void PluginManager::ScanDirectory(const std::string& directory) {
    // Add directory to scan paths and rescan
    AddScanPath(directory);
    
    // Reload world with new paths
    if (world_) {
        lilv_world_load_all(world_);
        plugins_ = lilv_world_get_all_plugins(world_);
        ScanPlugins();
    }
}

PluginInfo PluginManager::ExtractPluginInfo(const LilvPlugin* plugin) {
    PluginInfo info;
    
    info.uri = lilv_node_as_string(lilv_plugin_get_uri(plugin));
    
    LilvNode* nameNode = const_cast<LilvNode*>(lilv_plugin_get_name(plugin));
    info.name = nameNode ? lilv_node_as_string(nameNode) : "Unknown";
    if (nameNode) lilv_node_free(nameNode);
    
    LilvNode* authorNode = const_cast<LilvNode*>(lilv_plugin_get_author_name(plugin));
    info.author = authorNode ? lilv_node_as_string(authorNode) : "Unknown";
    if (authorNode) lilv_node_free(authorNode);
    
    info.category = GetPluginCategory(plugin);
    info.description = ""; // TODO: Extract description if available
    info.hasUI = false; // TODO: Check for UI extension
    
    // Count ports
    info.audioInputs = 0;
    info.audioOutputs = 0;
    info.controlInputs = 0;
    info.controlOutputs = 0;
    info.midiInputs = 0;
    info.midiOutputs = 0;
    
    uint32_t numPorts = lilv_plugin_get_num_ports(plugin);
    for (uint32_t i = 0; i < numPorts; ++i) {
        const LilvPort* port = lilv_plugin_get_port_by_index(plugin, i);
        
        if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__AudioPort))) {
            if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__InputPort))) {
                info.audioInputs++;
            } else if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__OutputPort))) {
                info.audioOutputs++;
            }
        } else if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__ControlPort))) {
            if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__InputPort))) {
                info.controlInputs++;
            } else if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__OutputPort))) {
                info.controlOutputs++;
            }
        }
        // TODO: Add MIDI port counting
    }
    
    return info;
}

std::string PluginManager::GetPluginCategory(const LilvPlugin* plugin) {
    // TODO: Implement proper category detection based on LV2 classes
    // For now, return a generic category based on port configuration
    
    uint32_t numPorts = lilv_plugin_get_num_ports(plugin);
    uint32_t audioInputs = 0, audioOutputs = 0;
    
    for (uint32_t i = 0; i < numPorts; ++i) {
        const LilvPort* port = lilv_plugin_get_port_by_index(plugin, i);
        
        if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__AudioPort))) {
            if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__InputPort))) {
                audioInputs++;
            } else if (lilv_port_is_a(plugin, port, lilv_new_uri(world_, LV2_CORE__OutputPort))) {
                audioOutputs++;
            }
        }
    }
    
    if (audioInputs == 0 && audioOutputs > 0) {
        return "Generator";
    } else if (audioInputs > 0 && audioOutputs > 0) {
        return "Effect";
    } else if (audioInputs > 0 && audioOutputs == 0) {
        return "Analyzer";
    }
    
    return "Utility";
}

std::vector<PluginInfo> PluginManager::GetAvailablePlugins() const {
    return availablePlugins_;
}

std::vector<PluginInfo> PluginManager::GetPluginsByCategory(const std::string& category) const {
    std::vector<PluginInfo> result;
    
    std::copy_if(availablePlugins_.begin(), availablePlugins_.end(),
                 std::back_inserter(result),
                 [&category](const PluginInfo& info) {
                     return info.category == category;
                 });
    
    return result;
}

std::vector<std::string> PluginManager::GetCategories() const {
    return categories_;
}

std::unique_ptr<PluginInstance> PluginManager::CreatePlugin(const std::string& uri, double sampleRate, uint32_t blockSize) {
    auto it = pluginMap_.find(uri);
    if (it == pluginMap_.end()) {
        return nullptr;
    }
    
    return std::make_unique<PluginInstance>(it->second, world_, sampleRate, blockSize);
}

PluginInfo PluginManager::GetPluginInfo(const std::string& uri) const {
    auto it = std::find_if(availablePlugins_.begin(), availablePlugins_.end(),
                          [&uri](const PluginInfo& info) {
                              return info.uri == uri;
                          });
    
    if (it != availablePlugins_.end()) {
        return *it;
    }
    
    return PluginInfo(); // Return empty info if not found
}

bool PluginManager::IsPluginAvailable(const std::string& uri) const {
    return pluginMap_.find(uri) != pluginMap_.end();
}

std::vector<std::string> PluginManager::GetDefaultScanPaths() const {
    std::vector<std::string> paths;
    
    // Common LV2 plugin directories on Windows
    paths.push_back("C:\\Program Files\\LV2");
    paths.push_back("C:\\Program Files (x86)\\LV2");
    
    // User directory
    char* appData = getenv("APPDATA");
    if (appData) {
        paths.push_back(std::string(appData) + "\\LV2");
    }
    
    return paths;
}

void PluginManager::AddScanPath(const std::string& path) {
    if (std::find(scanPaths_.begin(), scanPaths_.end(), path) == scanPaths_.end()) {
        scanPaths_.push_back(path);
    }
}

void PluginManager::RemoveScanPath(const std::string& path) {
    scanPaths_.erase(std::remove(scanPaths_.begin(), scanPaths_.end(), path), scanPaths_.end());
}

} // namespace violet