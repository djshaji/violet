#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace violet {

// Forward declarations
class AudioProcessingChain;
class PluginManager;

// Session data structure
struct SessionData {
    std::string name;
    std::string path;
    std::string version;
    
    struct PluginNode {
        uint32_t nodeId;
        std::string uri;
        std::string name;
        uint32_t position;
        bool bypassed;
        std::map<uint32_t, float> parameters;  // paramIndex -> value
    };
    
    std::vector<PluginNode> plugins;
    
    struct AudioSettings {
        uint32_t sampleRate;
        uint32_t bufferSize;
        uint32_t channels;
    };
    
    AudioSettings audioSettings;
};

// Session manager for save/load functionality
class SessionManager {
public:
    SessionManager();
    ~SessionManager();
    
    // Session operations
    bool NewSession();
    bool SaveSession(const std::string& filePath, AudioProcessingChain* chain);
    bool LoadSession(const std::string& filePath, AudioProcessingChain* chain, PluginManager* pluginManager);
    
    // Current session info
    std::string GetCurrentSessionPath() const { return currentSessionPath_; }
    bool HasUnsavedChanges() const { return hasUnsavedChanges_; }
    void SetUnsavedChanges(bool unsaved) { hasUnsavedChanges_ = unsaved; }
    
    // Session file validation
    static bool IsValidSessionFile(const std::string& filePath);
    
    // Recent sessions
    std::vector<std::string> GetRecentSessions() const;
    void AddRecentSession(const std::string& filePath);
    
private:
    bool SerializeSession(const SessionData& data, const std::string& filePath);
    bool DeserializeSession(const std::string& filePath, SessionData& data);
    
    SessionData CreateSessionFromChain(AudioProcessingChain* chain);
    bool ApplySessionToChain(const SessionData& data, AudioProcessingChain* chain, PluginManager* pluginManager);
    
    void SaveRecentSessions();
    void LoadRecentSessions();
    
    std::string currentSessionPath_;
    bool hasUnsavedChanges_;
    std::vector<std::string> recentSessions_;
    
    static const int MAX_RECENT_SESSIONS = 10;
    static const char* SESSION_VERSION;
    static const char* SESSION_EXTENSION;
};

} // namespace violet
