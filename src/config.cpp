#include "ipmi_fan_control/config.hpp"

#include <yaml-cpp/yaml.h>

#include <array>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace ipmi_fan_control {

std::vector<FanStep> DefaultFanSteps() {
    return {
        {40, 0},
        {50, 2},
        {55, 10},
        {60, 30},
        {62, 40},
        {65, 50},
        {70, 90},
    };
}

ControlConfig DefaultControlConfig() {
    ControlConfig config;
    config.steps = DefaultFanSteps();
    return config;
}

namespace {

template <typename T>
T ReadScalar(const YAML::Node& node, const char* key) {
    if (!node[key]) {
        throw std::runtime_error(std::string("缺少配置字段: ") + key);
    }
    return node[key].as<T>();
}

template <size_t N>
void RejectUnknownKeys(const YAML::Node& node, const std::array<const char*, N>& allowed_keys, const std::string& scope) {
    if (!node.IsMap()) {
        throw std::runtime_error(scope + " 必须是对象");
    }

    for (const auto& entry : node) {
        if (!entry.first.IsScalar()) {
            throw std::runtime_error(scope + " 包含无法识别的键");
        }

        const std::string key = entry.first.as<std::string>();
        const auto match = std::find_if(allowed_keys.begin(), allowed_keys.end(), [&](const char* allowed_key) {
            return key == allowed_key;
        });
        if (match == allowed_keys.end()) {
            throw std::runtime_error(scope + " 包含未知字段: " + key);
        }
    }
}

}  // namespace

void ValidateConfig(const ControlConfig& config) {
    if (config.interval_seconds < 1 || config.interval_seconds > 3600) {
        throw std::runtime_error("interval_seconds 必须在 1 到 3600 之间");
    }

    if (config.full_speed_threshold < 1 || config.full_speed_threshold > 150) {
        throw std::runtime_error("full_speed_threshold 必须在 1 到 150 之间");
    }

    if (config.steps.empty()) {
        throw std::runtime_error("steps 不能为空");
    }

    int previous_max_temp = -1;
    for (size_t i = 0; i < config.steps.size(); ++i) {
        const FanStep& step = config.steps[i];
        if (step.max_temp < 0 || step.max_temp > 150) {
            throw std::runtime_error("steps[" + std::to_string(i) + "].max_temp 超出允许范围");
        }
        if (step.fan_speed < 0 || step.fan_speed > 100) {
            throw std::runtime_error("steps[" + std::to_string(i) + "].fan_speed 超出 0-100 范围");
        }
        if (step.max_temp <= previous_max_temp) {
            throw std::runtime_error("steps 必须按 max_temp 严格升序排列");
        }
        previous_max_temp = step.max_temp;
    }
}

ControlConfig LoadConfigFromFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("配置文件不存在: " + file_path.string());
    }

    const YAML::Node root = YAML::LoadFile(file_path.string());
    RejectUnknownKeys(root, std::array<const char*, 3>{"interval_seconds", "full_speed_threshold", "steps"}, "根配置");

    ControlConfig config;
    config.interval_seconds = ReadScalar<int>(root, "interval_seconds");
    config.full_speed_threshold = ReadScalar<int>(root, "full_speed_threshold");

    const YAML::Node steps_node = root["steps"];
    if (!steps_node) {
        throw std::runtime_error("缺少配置字段: steps");
    }
    if (!steps_node.IsSequence()) {
        throw std::runtime_error("steps 必须是数组");
    }

    config.steps.clear();
    for (size_t i = 0; i < steps_node.size(); ++i) {
        const YAML::Node step_node = steps_node[i];
        RejectUnknownKeys(step_node, std::array<const char*, 2>{"max_temp", "fan_speed"}, "steps[" + std::to_string(i) + "]");

        FanStep step;
        step.max_temp = ReadScalar<int>(step_node, "max_temp");
        step.fan_speed = ReadScalar<int>(step_node, "fan_speed");
        config.steps.push_back(step);
    }

    ValidateConfig(config);
    return config;
}

std::string DescribeConfig(const ControlConfig& config) {
    std::ostringstream stream;
    stream << "配置有效\n";
    stream << "interval_seconds: " << config.interval_seconds << "\n";
    stream << "full_speed_threshold: " << config.full_speed_threshold << "\n";
    stream << "steps:\n";
    for (const FanStep& step : config.steps) {
        stream << "  - max_temp: " << step.max_temp << ", fan_speed: " << step.fan_speed << "\n";
    }
    return stream.str();
}

}  // namespace ipmi_fan_control
