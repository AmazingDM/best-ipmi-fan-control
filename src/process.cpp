#include "ipmi_fan_control/process.hpp"

#include <array>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <process.h>
#define popen _popen
#define pclose _pclose
#else
#include <sys/wait.h>
#endif

namespace ipmi_fan_control {

namespace {

std::string EscapeShellArgument(const std::string& value) {
    std::string escaped = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            escaped += "'\\''";
        } else {
            escaped += ch;
        }
    }
    escaped += "'";
    return escaped;
}

int NormalizeExitCode(int raw_exit_code) {
#ifdef _WIN32
    return raw_exit_code;
#else
    if (WIFEXITED(raw_exit_code)) {
        return WEXITSTATUS(raw_exit_code);
    }
    return raw_exit_code;
#endif
}

}  // namespace

CommandResult PosixCommandRunner::Run(const std::vector<std::string>& command) const {
    if (command.empty()) {
        throw std::runtime_error("命令不能为空");
    }

    std::ostringstream shell_command;
    for (size_t i = 0; i < command.size(); ++i) {
        if (i > 0) {
            shell_command << ' ';
        }
        shell_command << EscapeShellArgument(command[i]);
    }
    shell_command << " 2>&1";

    FILE* pipe = popen(shell_command.str().c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("无法启动子进程");
    }

    std::string output;
    std::array<char, 512> buffer {};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output.append(buffer.data());
    }

    const int status = pclose(pipe);
    return {NormalizeExitCode(status), output};
}

}  // namespace ipmi_fan_control
