#include "violet/utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <fstream>
#include <windows.h>

namespace violet {
namespace utils {

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string ToUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::vector<std::string> Split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool StartsWith(const std::string& str, const std::string& prefix) {
    return str.substr(0, prefix.length()) == prefix;
}

bool EndsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

std::string GetExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return std::string(path);
}

std::string GetExecutableDirectory() {
    std::string path = GetExecutablePath();
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return "";
}

std::string JoinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    
    std::string result = path1;
    if (result.back() != '\\' && result.back() != '/') {
        result += "\\";
    }
    
    std::string p2 = path2;
    if (p2.front() == '\\' || p2.front() == '/') {
        p2 = p2.substr(1);
    }
    
    return result + p2;
}

bool FileExists(const std::string& path) {
    DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirectoryExists(const std::string& path) {
    DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

double SamplesToMs(int samples, int sampleRate) {
    return (static_cast<double>(samples) / sampleRate) * 1000.0;
}

int MsToSamples(double ms, int sampleRate) {
    return static_cast<int>((ms / 1000.0) * sampleRate);
}

std::string GetLastErrorString() {
    DWORD error = GetLastError();
    if (error == 0) return "";
    
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr);
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    return message;
}

void ShowErrorMessage(const std::string& message, const std::string& title) {
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

void ShowWarningMessage(const std::string& message, const std::string& title) {
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
}

void ShowInfoMessage(const std::string& message, const std::string& title) {
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

} // namespace utils
} // namespace violet