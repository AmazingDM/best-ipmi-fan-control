#include "ipmi_fan_control/process.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <cstdio>
#include <process.h>
#define popen _popen
#define pclose _pclose
#else
#include <sys/wait.h>
#include <unistd.h>
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
    if (WIFSIGNALED(raw_exit_code)) {
        return 128 + WTERMSIG(raw_exit_code);
    }
    return raw_exit_code;
#endif
}

}  // namespace

CommandResult PosixCommandRunner::Run(const std::vector<std::string>& command) const {
    if (command.empty()) {
        throw std::runtime_error("Command cannot be empty");
    }

#ifdef _WIN32
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
        throw std::runtime_error("Unable to start child process");
    }

    std::string output;
    std::array<char, 512> buffer {};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output.append(buffer.data());
    }

    const int status = pclose(pipe);
    return {NormalizeExitCode(status), output};
#else
    int pipe_fds[2] = {-1, -1};
    if (pipe(pipe_fds) != 0) {
        throw std::runtime_error("Unable to create child process output pipe");
    }

    const pid_t child_pid = fork();
    if (child_pid < 0) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        throw std::runtime_error("Unable to create child process");
    }

    if (child_pid == 0) {
        dup2(pipe_fds[1], STDOUT_FILENO);
        dup2(pipe_fds[1], STDERR_FILENO);
        close(pipe_fds[0]);
        close(pipe_fds[1]);

        std::vector<char*> argv;
        argv.reserve(command.size() + 1);
        for (const std::string& arg : command) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());

        const std::string error = "execvp failed: " + std::string(std::strerror(errno)) + "\n";
        write(STDERR_FILENO, error.c_str(), error.size());
        _exit(127);
    }

    close(pipe_fds[1]);

    std::string output;
    std::array<char, 512> buffer {};
    ssize_t read_size = 0;
    while ((read_size = read(pipe_fds[0], buffer.data(), buffer.size())) > 0) {
        output.append(buffer.data(), static_cast<size_t>(read_size));
    }
    close(pipe_fds[0]);
    if (read_size < 0) {
        int status = 0;
        static_cast<void>(waitpid(child_pid, &status, 0));
        throw std::runtime_error("Failed to read child process output");
    }

    int status = 0;
    if (waitpid(child_pid, &status, 0) < 0) {
        throw std::runtime_error("Failed to wait for child process");
    }

    return {NormalizeExitCode(status), output};
#endif
}

}  // namespace ipmi_fan_control
