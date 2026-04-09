#pragma once

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace ipmi_fan_control {

enum class CommandType {
    kHelp,
    kAuto,
    kFixed,
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

class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

ParsedCommand ParseCommandLine(int argc, char** argv);
std::string BuildUsage();

}  // namespace ipmi_fan_control
