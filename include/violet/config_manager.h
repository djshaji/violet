#pragma once

#include <string>
#include <map>
#include <memory>

namespace violet {

// Simple configuration value variant
class ConfigValue {
public:
    enum Type { STRING, INTEGER, DOUBLE, BOOLEAN };
    
    ConfigValue() : type_(STRING), str_value_("") {}
    ConfigValue(const std::string& value) : type_(STRING), str_value_(value) {}
    ConfigValue(int value) : type_(INTEGER), int_value_(value) {}
    ConfigValue(double value) : type_(DOUBLE), double_value_(value) {}
    ConfigValue(bool value) : type_(BOOLEAN), bool_value_(value) {}
    
    Type GetType() const { return type_; }
    
    std::string AsString() const;
    int AsInt() const;
    double AsDouble() const;
    bool AsBool() const;
    
private:
    Type type_;
    union {
        int int_value_;
        double double_value_;
        bool bool_value_;
    };
    std::string str_value_;
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Initialize configuration system
    bool Initialize();
    
    // Save configuration to file
    bool Save();
    
    // Load configuration from file
    bool Load();
    
    // Get/Set configuration values
    ConfigValue Get(const std::string& key, const ConfigValue& defaultValue = ConfigValue()) const;
    void Set(const std::string& key, const ConfigValue& value);
    
    // Convenience methods
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    int GetInt(const std::string& key, int defaultValue = 0) const;
    double GetDouble(const std::string& key, double defaultValue = 0.0) const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    
    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    void SetDouble(const std::string& key, double value);
    void SetBool(const std::string& key, bool value);
    
    // Get configuration file path
    std::string GetConfigPath() const;
    
private:
    void SetDefaults();
    std::string GetAppDataPath() const;
    
    std::map<std::string, ConfigValue> values_;
    std::string config_path_;
};

} // namespace violet