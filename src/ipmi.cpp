#include "ipmi_fan_control/ipmi.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace ipmi_fan_control {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void EnsureSuccess(const CommandResult& result, const std::string& command_name) {
    if (result.exit_code != 0) {
        throw std::runtime_error(command_name + " failed: " + result.output);
    }
}

}  // namespace

std::string FilterFanAndTemperatureLines(const std::string& report) {
    std::ostringstream stream;
    std::istringstream input(report);
    std::string line;

    while (std::getline(input, line)) {
        const std::string lower = ToLower(line);
        if (lower.find("fan") != std::string::npos || lower.find("temp") != std::string::npos) {
            stream << line << "\n";
        }
    }

    return stream.str();
}

int ParseMaxTemperature(const std::string& report) {
    const std::regex pattern(R"((\d+)\s+degrees)", std::regex::icase);
    std::sregex_iterator current(report.begin(), report.end(), pattern);
    const std::sregex_iterator end;

    int max_temperature = -1;
    for (; current != end; ++current) {
        const int value = std::stoi((*current)[1].str());
        if (value > max_temperature) {
            max_temperature = value;
        }
    }

    if (max_temperature < 0) {
        throw std::runtime_error("Failed to parse a temperature from ipmitool output");
    }

    return max_temperature;
}

std::string FormatFanSpeedHex(int speed) {
    if (speed < 0 || speed > 100) {
        throw std::runtime_error("Fan speed must be between 0 and 100");
    }

    std::ostringstream stream;
    stream << "0x";
    constexpr char kHexDigits[] = "0123456789abcdef";
    stream << kHexDigits[(speed >> 4) & 0x0F] << kHexDigits[speed & 0x0F];
    return stream.str();
}

IpmiClient::IpmiClient(std::shared_ptr<CommandRunner> runner) : runner_(std::move(runner)) {
    if (runner_ == nullptr) {
        throw std::runtime_error("CommandRunner cannot be null");
    }
}

std::string IpmiClient::GetInfoFanTemp() const {
    const CommandResult result = runner_->Run({"ipmitool", "sdr", "list", "full"});
    EnsureSuccess(result, "ipmitool sdr list full");
    return FilterFanAndTemperatureLines(result.output);
}

int IpmiClient::GetMaxCpuTemperature() const {
    const CommandResult result = runner_->Run({"ipmitool", "sdr", "type", "Temperature"});
    EnsureSuccess(result, "ipmitool sdr type Temperature");
    return ParseMaxTemperature(result.output);
}

void IpmiClient::SetFanSpeed(int speed) const {
    const std::string speed_hex = FormatFanSpeedHex(speed);

    // 先切到手动模式，再设置百分比，保持和原工具一致。
    // Switch to manual mode first, then apply percentage to preserve the old behavior.
    const CommandResult enable_manual = runner_->Run({"ipmitool", "raw", "0x30", "0x30", "0x01", "0x00"});
    EnsureSuccess(enable_manual, "ipmitool raw manual-mode");

    const CommandResult set_speed = runner_->Run({"ipmitool", "raw", "0x30", "0x30", "0x02", "0xff", speed_hex});
    EnsureSuccess(set_speed, "ipmitool raw set-speed");
}

}  // namespace ipmi_fan_control
