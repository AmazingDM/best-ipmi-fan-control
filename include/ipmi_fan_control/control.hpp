#pragma once

#include "ipmi_fan_control/config.hpp"

namespace ipmi_fan_control {

int DetermineTargetFanSpeed(int temperature, const ControlConfig& config);

}  // namespace ipmi_fan_control
