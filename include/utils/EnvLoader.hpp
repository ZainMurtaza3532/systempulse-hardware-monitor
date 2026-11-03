#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <mutex>

namespace SystemPulse::Utils {

    class EnvLoader {
    public:
        // Parse a local .env file line-by-line (key=value pairs).
        // Stores variables in memory and sets them in the OS environment
        // (via Windows SetEnvironmentVariableA or POSIX setenv).
        // Returns true if file was loaded; returns false if absent (no throw).
        static bool Load(const std::string& filepath = ".env", bool setSystemEnv = true);

        // Safe accessor function: checks internal map first, falls back to std::getenv,
        // and returns defaultValue if absent.
        static std::string Get(const std::string& key, const std::string& defaultValue = "");

        // Convenience typed accessors
        static int GetInt(const std::string& key, int defaultValue = 0);
        static bool GetBool(const std::string& key, bool defaultValue = false);
        static float GetFloat(const std::string& key, float defaultValue = 0.0f);

        // Check if an environment variable exists in internal cache or OS environment
        static bool Has(const std::string& key);

        // Manually set or override a key-value pair
        static void Set(const std::string& key, const std::string& value, bool setSystemEnv = true);

        // Clear loaded in-memory environment variables
        static void Clear();

        // Retrieve a snapshot copy of all loaded in-memory environment variables
        static std::unordered_map<std::string, std::string> GetAll();

    private:
        static std::unordered_map<std::string, std::string> s_envMap;
        static std::mutex s_mutex;
        static bool s_isLoaded;

        static std::string Trim(std::string_view str);
        static void ParseLine(std::string_view line, bool setSystemEnv);
    };

} // namespace SystemPulse::Utils

