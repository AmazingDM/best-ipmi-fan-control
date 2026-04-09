#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ipmi_fan_control {

// 单条阶梯规则：当温度不超过 max_temp 时，风扇设置为 fan_speed。
// One control step: if the temperature is not above max_temp, set fan_speed.
struct FanStep {
    int max_temp = 0;
    int fan_speed = 0;
};

// 自动控速配置，既可来自 YAML，也可来自内置默认值。
// Auto-control configuration loaded from YAML or from built-in defaults.
struct ControlConfig {
    int interval_seconds = 5;
    int full_speed_threshold = 70;
    std::vector<FanStep> steps;
};

std::vector<FanStep> DefaultFanSteps();
ControlConfig DefaultControlConfig();
ControlConfig LoadConfigFromFile(const std::filesystem::path& file_path);
void ValidateConfig(const ControlConfig& config);
std::string DescribeConfig(const ControlConfig& config);

}  // namespace ipmi_fan_control
