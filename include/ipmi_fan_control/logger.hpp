#pragma once

#include <string>

namespace ipmi_fan_control {

class Logger {
public:
    static void SetVerbose(bool verbose);
    static void Info(const std::string& message);
    static void Error(const std::string& message);
    static void Trace(const std::string& message);

private:
    static void Log(const char* level, const std::string& message, bool allow_when_quiet);
};

}  // namespace ipmi_fan_control
