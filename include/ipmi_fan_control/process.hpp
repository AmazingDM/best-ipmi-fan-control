#pragma once

#include <string>
#include <vector>

namespace ipmi_fan_control {

struct CommandResult {
    int exit_code = 0;
    std::string output;
};

class CommandRunner {
public:
    virtual ~CommandRunner() = default;
    virtual CommandResult Run(const std::vector<std::string>& command) const = 0;
};

class PosixCommandRunner final : public CommandRunner {
public:
    CommandResult Run(const std::vector<std::string>& command) const override;
};

}  // namespace ipmi_fan_control
