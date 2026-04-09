#include "ipmi_fan_control/service.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace ipmi_fan_control {

namespace {

std::string EscapeSystemdArgument(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');

    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '%':
                escaped += "%%";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }

    escaped.push_back('"');
    return escaped;
}

std::string ReadFileContents(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to read file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void WriteFileContents(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Unable to write service file: " + path.string());
    }
    output << content;
    if (!output) {
        throw std::runtime_error("Failed to write service file: " + path.string());
    }
}

bool IsSystemdUnitEnabled(const CommandRunner& runner, const std::string& service_name) {
    const CommandResult result = runner.Run({"systemctl", "is-enabled", service_name});
    return result.exit_code == 0;
}

bool IsSystemdUnitActive(const CommandRunner& runner, const std::string& service_name) {
    const CommandResult result = runner.Run({"systemctl", "is-active", service_name});
    return result.exit_code == 0;
}

bool IsSystemdUnitKnown(const CommandRunner& runner, const std::string& service_name) {
    const CommandResult result = runner.Run({"systemctl", "status", "--no-pager", service_name});
    return result.exit_code == 0 || result.exit_code == 3 || result.output.find("Loaded:") != std::string::npos;
}

}  // namespace

std::string NormalizeServiceName(const std::string& service_name) {
    std::string normalized = service_name;
    constexpr const char* kSuffix = ".service";
    if (normalized.size() >= std::char_traits<char>::length(kSuffix) &&
        normalized.compare(normalized.size() - std::char_traits<char>::length(kSuffix), std::char_traits<char>::length(kSuffix), kSuffix) == 0) {
        normalized.resize(normalized.size() - std::char_traits<char>::length(kSuffix));
    }

    if (normalized.empty()) {
        throw std::runtime_error("service-name cannot be empty");
    }

    for (const unsigned char ch : normalized) {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '@') {
            continue;
        }
        throw std::runtime_error("service-name contains invalid characters: " + service_name);
    }

    if (normalized == "." || normalized == "..") {
        throw std::runtime_error("service-name is invalid: " + service_name);
    }

    return normalized;
}

std::string BuildSystemdUnit(const ServiceInstallOptions& options) {
    std::ostringstream stream;
    stream
        << "[Unit]\n"
        << "Description=IPMI fan control service\n"
        << "After=network-online.target\n"
        << "Wants=network-online.target\n\n"
        << "[Service]\n"
        << "Type=simple\n"
        << "Restart=always\n"
        << "RestartSec=5\n"
        << "ExecStart=" << EscapeSystemdArgument(options.executable_path.string())
        << " auto --config " << EscapeSystemdArgument(options.config_path.string()) << "\n"
        << "ExecReload=/bin/kill -HUP $MAINPID\n"
        << "KillMode=mixed\n\n"
        << "[Install]\n"
        << "WantedBy=multi-user.target\n";
    return stream.str();
}

std::filesystem::path ResolveExecutablePath(const std::filesystem::path& argv0) {
#ifndef _WIN32
    std::array<char, 4096> buffer {};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length > 0) {
        buffer[static_cast<size_t>(length)] = '\0';
        return std::filesystem::path(buffer.data());
    }
#endif

    if (argv0.empty()) {
        throw std::runtime_error("Unable to resolve the current executable path");
    }
    return std::filesystem::absolute(argv0);
}

void InstallService(const ServiceInstallOptions& options, const CommandRunner& runner) {
    if (options.config_path.empty()) {
        throw std::runtime_error("Config path cannot be empty");
    }
    if (!std::filesystem::exists(options.config_path)) {
        throw std::runtime_error("Config file does not exist: " + options.config_path.string());
    }

    const std::string unit_content = BuildSystemdUnit(options);
    if (options.dry_run) {
        throw std::runtime_error("dry-run output must be handled by the caller");
    }

    if (options.output_path.has_value()) {
        if (options.output_path->has_parent_path()) {
            std::filesystem::create_directories(options.output_path->parent_path());
        }
        std::ofstream output(*options.output_path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Unable to write service file: " + options.output_path->string());
        }
        output << unit_content;
        return;
    }

#ifndef _WIN32
    if (geteuid() != 0) {
        throw std::runtime_error("Installing a systemd service requires root privileges");
    }
#endif

    const std::string service_name = NormalizeServiceName(options.service_name);
    const std::filesystem::path service_path = std::filesystem::path("/etc/systemd/system") / (service_name + ".service");
    const bool had_existing_file = std::filesystem::exists(service_path);
    const std::string previous_content = had_existing_file ? ReadFileContents(service_path) : std::string();
    const bool was_enabled = IsSystemdUnitEnabled(runner, service_name);
    const bool was_active = IsSystemdUnitActive(runner, service_name);
    bool enable_attempted = false;

    try {
        WriteFileContents(service_path, unit_content);

        const CommandResult reload = runner.Run({"systemctl", "daemon-reload"});
        if (reload.exit_code != 0) {
            throw std::runtime_error("systemctl daemon-reload failed: " + reload.output);
        }

        enable_attempted = true;
        const CommandResult enable = runner.Run({"systemctl", "enable", "--now", service_name});
        if (enable.exit_code != 0) {
            throw std::runtime_error("systemctl enable --now failed: " + enable.output);
        }
    } catch (const std::exception& ex) {
        // 安装流程会同时修改 unit 文件、enable 状态和运行状态，失败时必须尽量恢复旧状态，
        // 否则可能把主机留在“服务文件已替换但未成功启动”的危险半配置状态。
        // The install path mutates the unit file, enable state, and runtime state together.
        // On failure we best-effort roll everything back to avoid leaving the host in a
        // partially updated and potentially non-boot-resilient service configuration.
        std::string rollback_note = "; attempted to roll back the service file";
        try {
            if (had_existing_file) {
                WriteFileContents(service_path, previous_content);
            } else if (std::filesystem::exists(service_path)) {
                std::filesystem::remove(service_path);
            }

            const CommandResult rollback_reload = runner.Run({"systemctl", "daemon-reload"});
            if (rollback_reload.exit_code != 0) {
                rollback_note += ", but daemon-reload rollback failed: " + rollback_reload.output;
            }

            if (enable_attempted) {
                const CommandResult enable_state = was_enabled
                    ? runner.Run({"systemctl", "enable", service_name})
                    : runner.Run({"systemctl", "disable", service_name});
                if (enable_state.exit_code != 0) {
                    rollback_note += was_enabled
                        ? ", and enable state restoration failed: " + enable_state.output
                        : ", and disable state restoration failed: " + enable_state.output;
                }

                const CommandResult active_state = was_active
                    ? runner.Run({"systemctl", "restart", service_name})
                    : runner.Run({"systemctl", "stop", service_name});
                if (active_state.exit_code != 0) {
                    rollback_note += was_active
                        ? ", and active state restoration failed: " + active_state.output
                        : ", and stop state restoration failed: " + active_state.output;
                }
            }
        } catch (const std::exception& rollback_ex) {
            rollback_note += ", but rollback failed: " + std::string(rollback_ex.what());
        }

        throw std::runtime_error(std::string(ex.what()) + rollback_note);
    }
}

void UninstallService(const std::string& service_name_input, const CommandRunner& runner) {
#ifndef _WIN32
    if (geteuid() != 0) {
        throw std::runtime_error("Uninstalling a systemd service requires root privileges");
    }
#endif

    const std::string service_name = NormalizeServiceName(service_name_input);
    const std::filesystem::path service_path = std::filesystem::path("/etc/systemd/system") / (service_name + ".service");
    const bool unit_exists = std::filesystem::exists(service_path);
    const bool was_enabled = IsSystemdUnitEnabled(runner, service_name);
    const bool was_active = IsSystemdUnitActive(runner, service_name);
    const bool unit_known = unit_exists || was_enabled || was_active || IsSystemdUnitKnown(runner, service_name);

    if (was_active) {
        const CommandResult stop = runner.Run({"systemctl", "stop", service_name});
        if (stop.exit_code != 0) {
            throw std::runtime_error("systemctl stop failed: " + stop.output);
        }
    }

    if (was_enabled) {
        const CommandResult disable = runner.Run({"systemctl", "disable", service_name});
        if (disable.exit_code != 0) {
            throw std::runtime_error("systemctl disable failed: " + disable.output);
        }
    }

    if (unit_exists) {
        std::filesystem::remove(service_path);
    }

    if (unit_exists || unit_known) {
        const CommandResult reload = runner.Run({"systemctl", "daemon-reload"});
        if (reload.exit_code != 0) {
            throw std::runtime_error("systemctl daemon-reload failed: " + reload.output);
        }

        const CommandResult reset_failed = runner.Run({"systemctl", "reset-failed", service_name});
        if (reset_failed.exit_code != 0) {
            throw std::runtime_error("systemctl reset-failed failed: " + reset_failed.output);
        }
    }
}

}  // namespace ipmi_fan_control
