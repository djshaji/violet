#include "violet/config_manager.h"
#include "violet/utils.h"
#include <fstream>
#include <sstream>
#include <windows.h>
#include <shlobj.h>

namespace violet {

// ConfigValue implementation
std::string ConfigValue::AsString() const {
    switch (type_) {
    case STRING: return str_value_;
    case INTEGER: return std::to_string(int_value_);
    case DOUBLE: return std::to_string(double_value_);
    case BOOLEAN: return bool_value_ ? "true" : "false";
    default: return "";
    }
}

int ConfigValue::AsInt() const {
    switch (type_) {
    case INTEGER: return int_value_;
    case STRING: return std::stoi(str_value_);
    case DOUBLE: return static_cast<int>(double_value_);
    case BOOLEAN: return bool_value_ ? 1 : 0;
    default: return 0;
    }
}

double ConfigValue::AsDouble() const {
    switch (type_) {
    case DOUBLE: return double_value_;
    case INTEGER: return static_cast<double>(int_value_);
    case STRING: return std::stod(str_value_);
    case BOOLEAN: return bool_value_ ? 1.0 : 0.0;
    default: return 0.0;
    }
}

bool ConfigValue::AsBool() const {
    switch (type_) {
    case BOOLEAN: return bool_value_;
    case INTEGER: return int_value_ != 0;
    case DOUBLE: return double_value_ != 0.0;
    case STRING: return str_value_ == "true" || str_value_ == "1";
    default: return false;
    }
}

// ConfigManager implementation
ConfigManager::ConfigManager() {
    SetDefaults();
}

ConfigManager::~ConfigManager() {
    Save();
}

bool ConfigManager::Initialize() {
    // Get application data directory
    std::string appDataPath = GetAppDataPath();
    if (appDataPath.empty()) {
        return false;
    }
    
    // Create Violet directory if it doesn't exist
    std::string violetDir = appDataPath + "\\Violet";
    CreateDirectoryA(violetDir.c_str(), nullptr);
    
    config_path_ = violetDir + "\\config.ini";
    
    // Try to load existing configuration
    Load();
    
    return true;
}

bool ConfigManager::Save() {
    if (config_path_.empty()) {
        return false;
    }
    
    std::ofstream file(config_path_);
    if (!file.is_open()) {
        return false;
    }
    
    // Write a simple INI-style format
    file << "# Violet Configuration File\n";
    file << "# Auto-generated - do not edit manually\n\n";
    
    for (const auto& pair : values_) {
        file << pair.first << "=" << pair.second.AsString() << "\n";
    }
    
    return true;
}

bool ConfigManager::Load() {
    if (config_path_.empty()) {
        return false;
    }
    
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        return false; // File doesn't exist, use defaults
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Trim whitespace
        key = utils::Trim(key);
        value = utils::Trim(value);
        
        // Try to determine value type and store
        if (value == "true" || value == "false") {
            values_[key] = ConfigValue(value == "true");
        } else if (value.find('.') != std::string::npos) {
            try {
                double d = std::stod(value);
                values_[key] = ConfigValue(d);
            } catch (...) {
                values_[key] = ConfigValue(value);
            }
        } else {
            try {
                int i = std::stoi(value);
                values_[key] = ConfigValue(i);
            } catch (...) {
                values_[key] = ConfigValue(value);
            }
        }
    }
    
    return true;
}

ConfigValue ConfigManager::Get(const std::string& key, const ConfigValue& defaultValue) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return defaultValue;
}

void ConfigManager::Set(const std::string& key, const ConfigValue& value) {
    values_[key] = value;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) const {
    return Get(key, ConfigValue(defaultValue)).AsString();
}

int ConfigManager::GetInt(const std::string& key, int defaultValue) const {
    return Get(key, ConfigValue(defaultValue)).AsInt();
}

double ConfigManager::GetDouble(const std::string& key, double defaultValue) const {
    return Get(key, ConfigValue(defaultValue)).AsDouble();
}

bool ConfigManager::GetBool(const std::string& key, bool defaultValue) const {
    return Get(key, ConfigValue(defaultValue)).AsBool();
}

void ConfigManager::SetString(const std::string& key, const std::string& value) {
    Set(key, ConfigValue(value));
}

void ConfigManager::SetInt(const std::string& key, int value) {
    Set(key, ConfigValue(value));
}

void ConfigManager::SetDouble(const std::string& key, double value) {
    Set(key, ConfigValue(value));
}

void ConfigManager::SetBool(const std::string& key, bool value) {
    Set(key, ConfigValue(value));
}

std::string ConfigManager::GetConfigPath() const {
    return config_path_;
}

void ConfigManager::SetDefaults() {
    // Audio settings
    SetInt("audio.sample_rate", 44100);
    SetInt("audio.buffer_size", 256);
    SetInt("audio.bit_depth", 32);
    SetBool("audio.auto_start", false);
    
    // UI settings
    SetInt("ui.window_width", 1000);
    SetInt("ui.window_height", 700);
    SetInt("ui.window_x", -1);  // -1 means center
    SetInt("ui.window_y", -1);
    SetBool("ui.dark_theme", false);
    SetBool("ui.show_toolbar", true);
    SetBool("ui.show_statusbar", true);
    
    // Plugin settings
    SetString("plugins.scan_paths", "");
    SetBool("plugins.auto_scan", true);
    SetInt("plugins.max_instances", 16);
    
    // Session settings
    SetString("session.last_file", "");
    SetBool("session.auto_save", true);
    SetInt("session.auto_save_interval", 300); // 5 minutes
}

std::string ConfigManager::GetAppDataPath() const {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return std::string(path);
    }
    return "";
}

} // namespace violet