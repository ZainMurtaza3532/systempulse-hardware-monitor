#include "utils/EnvLoader.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace SystemPulse::Utils {

    std::unordered_map<std::string, std::string> EnvLoader::s_envMap;
    std::mutex EnvLoader::s_mutex;
    bool EnvLoader::s_isLoaded = false;

    std::string EnvLoader::Trim(std::string_view str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) {
            return "";
        }
        size_t last = str.find_last_not_of(" \t\r\n");
        return std::string(str.substr(first, (last - first + 1)));
    }

    static std::string UnescapeValue(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                switch (str[i + 1]) {
                    case 'n': result += '\n'; ++i; break;
                    case 'r': result += '\r'; ++i; break;
                    case 't': result += '\t'; ++i; break;
                    case '\\': result += '\\'; ++i; break;
                    case '\"': result += '\"'; ++i; break;
                    case '\'': result += '\''; ++i; break;
                    default: result += str[i]; break;
                }
            } else {
                result += str[i];
            }
        }
        return result;
    }

    void EnvLoader::ParseLine(std::string_view rawLine, bool setSystemEnv) {
        std::string line = Trim(rawLine);

        // Ignore empty lines and comment lines
        if (line.empty() || line[0] == '#' || (line.size() >= 2 && line[0] == '/' && line[1] == '/')) {
            return;
        }

        // Handle optional bash "export " prefix
        if (line.rfind("export ", 0) == 0) {
            line = Trim(line.substr(7));
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            return;
        }

        std::string key = Trim(line.substr(0, eqPos));
        if (key.empty()) {
            return;
        }

        std::string valStr = Trim(line.substr(eqPos + 1));
        std::string value;

        // Process Quoted and Unquoted values
        if (valStr.size() >= 2 && valStr.front() == '"' && valStr.back() == '"') {
            // Double-quoted string: unescape escape sequences
            value = UnescapeValue(valStr.substr(1, valStr.size() - 2));
        } else if (valStr.size() >= 2 && valStr.front() == '\'' && valStr.back() == '\'') {
            // Single-quoted string: raw literal
            value = valStr.substr(1, valStr.size() - 2);
        } else {
            // Unquoted string: strip trailing inline comments
            size_t commentPos = valStr.find('#');
            if (commentPos != std::string::npos) {
                valStr = Trim(valStr.substr(0, commentPos));
            }
            value = valStr;
        }

        s_envMap[key] = value;

        // Optionally synchronize with OS environment
        if (setSystemEnv) {
#ifdef _WIN32
            SetEnvironmentVariableA(key.c_str(), value.c_str());
#else
            setenv(key.c_str(), value.c_str(), 1);
#endif
        }
    }

    bool EnvLoader::Load(const std::string& filepath, bool setSystemEnv) {
        std::lock_guard<std::mutex> lock(s_mutex);

        std::ifstream file(filepath);
        if (!file.is_open()) {
            // File is absent; return false without throwing.
            // Get() will seamlessly query std::getenv.
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            ParseLine(line, setSystemEnv);
        }

        s_isLoaded = true;
        return true;
    }

    std::string EnvLoader::Get(const std::string& key, const std::string& defaultValue) {
        std::lock_guard<std::mutex> lock(s_mutex);

        // 1. Check in-memory map populated from .env
        auto it = s_envMap.find(key);
        if (it != s_envMap.end()) {
            return it->second;
        }

        // 2. Fall back to standard OS environment variables
        const char* osEnv = std::getenv(key.c_str());
        if (osEnv != nullptr) {
            return std::string(osEnv);
        }

        // 3. Fall back to supplied default value
        return defaultValue;
    }

    int EnvLoader::GetInt(const std::string& key, int defaultValue) {
        std::string val = Get(key, "");
        if (val.empty()) {
            return defaultValue;
        }
        try {
            return std::stoi(val);
        } catch (...) {
            return defaultValue;
        }
    }

    bool EnvLoader::GetBool(const std::string& key, bool defaultValue) {
        std::string val = Get(key, "");
        if (val.empty()) {
            return defaultValue;
        }

        std::string lower;
        lower.reserve(val.size());
        for (char c : val) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        if (lower == "true" || lower == "1" || lower == "yes" || lower == "on" || lower == "enabled") {
            return true;
        }
        if (lower == "false" || lower == "0" || lower == "no" || lower == "off" || lower == "disabled") {
            return false;
        }

        return defaultValue;
    }

    float EnvLoader::GetFloat(const std::string& key, float defaultValue) {
        std::string val = Get(key, "");
        if (val.empty()) {
            return defaultValue;
        }
        try {
            return std::stof(val);
        } catch (...) {
            return defaultValue;
        }
    }

    bool EnvLoader::Has(const std::string& key) {
        std::lock_guard<std::mutex> lock(s_mutex);

        if (s_envMap.find(key) != s_envMap.end()) {
            return true;
        }
        return std::getenv(key.c_str()) != nullptr;
    }

    void EnvLoader::Set(const std::string& key, const std::string& value, bool setSystemEnv) {
        std::lock_guard<std::mutex> lock(s_mutex);

        s_envMap[key] = value;
        if (setSystemEnv) {
#ifdef _WIN32
            SetEnvironmentVariableA(key.c_str(), value.c_str());
#else
            setenv(key.c_str(), value.c_str(), 1);
#endif
        }
    }

    void EnvLoader::Clear() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_envMap.clear();
        s_isLoaded = false;
    }

    std::unordered_map<std::string, std::string> EnvLoader::GetAll() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_envMap;
    }

} // namespace SystemPulse::Utils

