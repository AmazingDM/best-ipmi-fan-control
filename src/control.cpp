#include "ipmi_fan_control/control.hpp"

namespace ipmi_fan_control {

int DetermineTargetFanSpeed(int temperature, const ControlConfig& config) {
    ValidateConfig(config);

    // 高温阈值优先，确保风险温度下直接拉满风扇。
    // Full-speed threshold takes priority to fail safe under hot conditions.
    if (temperature >= config.full_speed_threshold) {
        return 100;
    }

    // 阶梯规则按温度上限升序排列，命中首个区间即可返回。
    // Steps are sorted by upper temperature bound, so the first match wins.
    for (const FanStep& step : config.steps) {
        if (temperature <= step.max_temp) {
            return step.fan_speed;
        }
    }

    return 100;
}

}  // namespace ipmi_fan_control
