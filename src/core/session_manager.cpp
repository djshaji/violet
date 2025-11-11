#include "violet/session_manager.h"
#include "violet/audio_processing_chain.h"
#include "violet/plugin_manager.h"
#include "violet/config_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace violet {

const char* SessionManager::SESSION_VERSION = "1.0";
const char* SessionManager::SESSION_EXTENSION = ".violet";

SessionManager::SessionManager()
    : hasUnsavedChanges_(false) {
    LoadRecentSessions();
}

SessionManager::~SessionManager() {
    SaveRecentSessions();
}

bool SessionManager::NewSession() {
    currentSessionPath_.clear();
    hasUnsavedChanges_ = false;
    return true;
}

bool SessionManager::SaveSession(const std::string& filePath, AudioProcessingChain* chain) {
    if (!chain) {
        return false;
    }
    
    SessionData data = CreateSessionFromChain(chain);
    data.path = filePath;
    
    // Extract name from path
    size_t lastSlash = filePath.find_last_of("/\\");
    size_t lastDot = filePath.find_last_of('.');
    if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash) {
        data.name = filePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else {
        data.name = "Untitled";
    }
    
    if (SerializeSession(data, filePath)) {
        currentSessionPath_ = filePath;
        hasUnsavedChanges_ = false;
        AddRecentSession(filePath);
        return true;
    }
    
    return false;
}

bool SessionManager::LoadSession(const std::string& filePath, AudioProcessingChain* chain, PluginManager* pluginManager) {
    if (!chain || !pluginManager) {
        return false;
    }
    
    SessionData data;
    if (!DeserializeSession(filePath, data)) {
        return false;
    }
    
    if (ApplySessionToChain(data, chain, pluginManager)) {
        currentSessionPath_ = filePath;
        hasUnsavedChanges_ = false;
        AddRecentSession(filePath);
        return true;
    }
    
    return false;
}

bool SessionManager::IsValidSessionFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    if (std::getline(file, line)) {
        return line.find("VIOLET_SESSION") != std::string::npos;
    }
    
    return false;
}

SessionData SessionManager::CreateSessionFromChain(AudioProcessingChain* chain) {
    SessionData data;
    data.version = SESSION_VERSION;
    
    // Get audio format
    chain->GetFormat(data.audioSettings.sampleRate, 
                     data.audioSettings.channels, 
                     data.audioSettings.bufferSize);
    
    // Get all nodes
    auto nodeIds = chain->GetNodeIds();
    for (uint32_t nodeId : nodeIds) {
        auto node = chain->GetNode(nodeId);
        if (node && node->GetPlugin()) {
            SessionData::PluginNode pluginNode;
            pluginNode.nodeId = nodeId;
            pluginNode.uri = node->GetPlugin()->GetInfo().uri;
            pluginNode.name = node->GetPlugin()->GetInfo().name;
            pluginNode.position = static_cast<uint32_t>(data.plugins.size());
            pluginNode.bypassed = node->IsBypassed();
            
            // Get parameter values
            auto params = node->GetPlugin()->GetParameters();
            for (size_t i = 0; i < params.size(); ++i) {
                float value = node->GetPlugin()->GetParameter(params[i].index);
                pluginNode.parameters[params[i].index] = value;
            }
            
            data.plugins.push_back(pluginNode);
        }
    }
    
    return data;
}

bool SessionManager::ApplySessionToChain(const SessionData& data, AudioProcessingChain* chain, PluginManager* pluginManager) {
    // Clear existing chain
    chain->ClearChain();
    
    // Set audio format
    chain->SetFormat(data.audioSettings.sampleRate,
                     data.audioSettings.channels,
                     data.audioSettings.bufferSize);
    
    // Load plugins in order
    for (const auto& pluginNode : data.plugins) {
        uint32_t nodeId = chain->AddPlugin(pluginNode.uri);
        if (nodeId == 0) {
            continue;  // Failed to load plugin, skip
        }
        
        auto node = chain->GetNode(nodeId);
        if (!node) {
            continue;
        }
        
        // Set bypass state
        node->SetBypassed(pluginNode.bypassed);
        
        // Set parameter values
        for (const auto& param : pluginNode.parameters) {
            chain->SetParameter(nodeId, param.first, param.second);
        }
    }
    
    return true;
}

bool SessionManager::SerializeSession(const SessionData& data, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "VIOLET_SESSION\n";
    file << "VERSION=" << data.version << "\n";
    file << "NAME=" << data.name << "\n";
    file << "\n";
    
    // Write audio settings
    file << "[AUDIO]\n";
    file << "SampleRate=" << data.audioSettings.sampleRate << "\n";
    file << "BufferSize=" << data.audioSettings.bufferSize << "\n";
    file << "Channels=" << data.audioSettings.channels << "\n";
    file << "\n";
    
    // Write plugins
    file << "[PLUGINS]\n";
    file << "Count=" << data.plugins.size() << "\n";
    file << "\n";
    
    for (size_t i = 0; i < data.plugins.size(); ++i) {
        const auto& plugin = data.plugins[i];
        file << "[PLUGIN_" << i << "]\n";
        file << "NodeID=" << plugin.nodeId << "\n";
        file << "URI=" << plugin.uri << "\n";
        file << "Name=" << plugin.name << "\n";
        file << "Position=" << plugin.position << "\n";
        file << "Bypassed=" << (plugin.bypassed ? "1" : "0") << "\n";
        
        // Write parameters
        if (!plugin.parameters.empty()) {
            file << "Parameters=";
            bool first = true;
            for (const auto& param : plugin.parameters) {
                if (!first) file << ",";
                file << param.first << ":" << param.second;
                first = false;
            }
            file << "\n";
        }
        file << "\n";
    }
    
    return true;
}

bool SessionManager::DeserializeSession(const std::string& filePath, SessionData& data) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    
    // Read header
    if (!std::getline(file, line) || line != "VIOLET_SESSION") {
        return false;
    }
    
    std::string currentSection;
    int currentPluginIndex = -1;
    
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
            
            // Check if it's a plugin section
            if (currentSection.find("PLUGIN_") == 0) {
                currentPluginIndex = static_cast<int>(data.plugins.size());
                data.plugins.push_back(SessionData::PluginNode());
            }
            continue;
        }
        
        // Parse key=value
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);
        
        // Parse based on current section
        if (currentSection == "AUDIO") {
            if (key == "SampleRate") data.audioSettings.sampleRate = std::stoi(value);
            else if (key == "BufferSize") data.audioSettings.bufferSize = std::stoi(value);
            else if (key == "Channels") data.audioSettings.channels = std::stoi(value);
        }
        else if (key == "VERSION") {
            data.version = value;
        }
        else if (key == "NAME") {
            data.name = value;
        }
        else if (currentPluginIndex >= 0 && currentPluginIndex < static_cast<int>(data.plugins.size())) {
            auto& plugin = data.plugins[currentPluginIndex];
            
            if (key == "NodeID") plugin.nodeId = std::stoul(value);
            else if (key == "URI") plugin.uri = value;
            else if (key == "Name") plugin.name = value;
            else if (key == "Position") plugin.position = std::stoul(value);
            else if (key == "Bypassed") plugin.bypassed = (value == "1");
            else if (key == "Parameters") {
                // Parse parameters: "index:value,index:value,..."
                std::stringstream ss(value);
                std::string param;
                while (std::getline(ss, param, ',')) {
                    size_t colonPos = param.find(':');
                    if (colonPos != std::string::npos) {
                        uint32_t index = std::stoul(param.substr(0, colonPos));
                        float paramValue = std::stof(param.substr(colonPos + 1));
                        plugin.parameters[index] = paramValue;
                    }
                }
            }
        }
    }
    
    return true;
}

std::vector<std::string> SessionManager::GetRecentSessions() const {
    return recentSessions_;
}

void SessionManager::AddRecentSession(const std::string& filePath) {
    // Remove if already exists
    auto it = std::find(recentSessions_.begin(), recentSessions_.end(), filePath);
    if (it != recentSessions_.end()) {
        recentSessions_.erase(it);
    }
    
    // Add to front
    recentSessions_.insert(recentSessions_.begin(), filePath);
    
    // Limit size
    if (recentSessions_.size() > MAX_RECENT_SESSIONS) {
        recentSessions_.resize(MAX_RECENT_SESSIONS);
    }
    
    SaveRecentSessions();
}

void SessionManager::SaveRecentSessions() {
    ConfigManager config;
    config.Load();
    
    for (size_t i = 0; i < recentSessions_.size(); ++i) {
        std::string key = "recent.session" + std::to_string(i);
        config.SetString(key, recentSessions_[i]);
    }
    
    config.Save();
}

void SessionManager::LoadRecentSessions() {
    ConfigManager config;
    if (!config.Load()) {
        return;
    }
    
    recentSessions_.clear();
    for (int i = 0; i < MAX_RECENT_SESSIONS; ++i) {
        std::string key = "recent.session" + std::to_string(i);
        std::string path = config.GetString(key, "");
        if (!path.empty()) {
            recentSessions_.push_back(path);
        }
    }
}

} // namespace violet
