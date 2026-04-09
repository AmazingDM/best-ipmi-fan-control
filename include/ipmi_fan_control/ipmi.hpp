#pragma once

#include "ipmi_fan_control/process.hpp"

#include <memory>
#include <string>

namespace ipmi_fan_control {

std::string FilterFanAndTemperatureLines(const std::string& report);
int ParseMaxTemperature(const std::string& report);
std::string FormatFanSpeedHex(int speed);

// IPMI 客户端负责组织 ipmitool 命令和解析输出。
// IPMI client wraps ipmitool commands and parses the resulting output.
class IpmiClient {
public:
    explicit IpmiClient(std::shared_ptr<CommandRunner> runner);

    std::string GetInfoFanTemp() const;
    int GetMaxCpuTemperature() const;
    void SetFanSpeed(int speed) const;

private:
    std::shared_ptr<CommandRunner> runner_;
};

}  // namespace ipmi_fan_control
