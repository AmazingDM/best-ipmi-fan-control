#include "ipmi_fan_control/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace ipmi_fan_control {

namespace {

bool g_verbose = false;
std::mutex g_log_mutex;

std::string BuildTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t current_time = std::chrono::system_clock::to_time_t(now);

    std::tm time_info {};
#ifdef _WIN32
    localtime_s(&time_info, &current_time);
#else
    localtime_r(&current_time, &time_info);
#endif

    std::ostringstream stream;
    stream << std::put_time(&time_info, "%Y-%m-%dT%H:%M:%S");
    return stream.str();
}

}  // namespace

void Logger::SetVerbose(bool verbose) {
    g_verbose = verbose;
}

void Logger::Info(const std::string& message) {
    Log("INFO", message, true);
}

void Logger::Error(const std::string& message) {
    Log("ERROR", message, true);
}

void Logger::Trace(const std::string& message) {
    Log("TRACE", message, false);
}

void Logger::Log(const char* level, const std::string& message, bool allow_when_quiet) {
    if (!allow_when_quiet && !g_verbose) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::cerr << BuildTimestamp() << " " << level << " " << message << std::endl;
}

}  // namespace ipmi_fan_control
