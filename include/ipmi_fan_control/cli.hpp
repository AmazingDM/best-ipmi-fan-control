#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace ipmi_fan_control {

enum class CommandType {
    kHelp,
    kAuto,
    kFixed,
    kInfo,
    kValidateConfig,
    kInstallService,
};

struct ParsedCommand {
    CommandType type = CommandType::kHelp;
    bool verbose = false;
    std::optional<int> fixed_value;
    std::optional<std::filesystem::path> config_path;
    std::optional<int> interval_override;
    std::optional<int> threshold_override;
    std::string service_name = "ipmi-fan-control";
    std::optional<std::filesystem::path> output_path;
    bool dry_run = false;
    std::filesystem::path executable_path;
};

ParsedCommand ParseCommandLine(int argc, char** argv);
std::string BuildUsage();

}  // namespace ipmi_fan_control
