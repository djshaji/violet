#pragma once

#include <string>
#include <vector>

namespace violet {
namespace utils {

// String utilities
std::string Trim(const std::string& str);
std::string ToLower(const std::string& str);
std::string ToUpper(const std::string& str);
std::vector<std::string> Split(const std::string& str, char delimiter);
bool StartsWith(const std::string& str, const std::string& prefix);
bool EndsWith(const std::string& str, const std::string& suffix);

// Path utilities
std::string GetExecutablePath();
std::string GetExecutableDirectory();
std::string JoinPath(const std::string& path1, const std::string& path2);
bool FileExists(const std::string& path);
bool DirectoryExists(const std::string& path);

// Windows utilities
std::wstring StringToWString(const std::string& str);
std::string WStringToString(const std::wstring& wstr);

// Audio utilities
double SamplesToMs(int samples, int sampleRate);
int MsToSamples(double ms, int sampleRate);

// Error handling
std::string GetLastErrorString();
void ShowErrorMessage(const std::string& message, const std::string& title = "Error");
void ShowWarningMessage(const std::string& message, const std::string& title = "Warning");
void ShowInfoMessage(const std::string& message, const std::string& title = "Information");

} // namespace utils
} // namespace violet