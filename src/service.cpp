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
        throw std::runtime_error("无法读取文件: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void WriteFileContents(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("无法写入服务文件: " + path.string());
    }
    output << content;
    if (!output) {
        throw std::runtime_error("写入服务文件失败: " + path.string());
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

}  // namespace

std::string NormalizeServiceName(const std::string& service_name) {
    std::string normalized = service_name;
    constexpr const char* kSuffix = ".service";
    if (normalized.size() >= std::char_traits<char>::length(kSuffix) &&
        normalized.compare(normalized.size() - std::char_traits<char>::length(kSuffix), std::char_traits<char>::length(kSuffix), kSuffix) == 0) {
        normalized.resize(normalized.size() - std::char_traits<char>::length(kSuffix));
    }

    if (normalized.empty()) {
        throw std::runtime_error("service-name 不能为空");
    }

    for (const unsigned char ch : normalized) {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '@') {
            continue;
        }
        throw std::runtime_error("service-name 包含非法字符: " + service_name);
    }

    if (normalized == "." || normalized == "..") {
        throw std::runtime_error("service-name 非法: " + service_name);
    }

    return normalized;
}

std::string BuildSystemdUnit(const ServiceInstallOptions& options) {
    std::ostringstream stream;
    stream
        << "[Unit]\n"
        << "Description=IPMI 风扇控制服务 / IPMI fan control service\n"
        << "After=network-online.target\n"
        << "Wants=network-online.target\n\n"
        << "[Service]\n"
        << "Type=simple\n"
        << "Restart=always\n"
        << "RestartSec=5\n"
        << "ExecStart=" << EscapeSystemdArgument(options.executable_path.string())
        << " auto --config " << EscapeSystemdArgument(options.config_path.string()) << "\n"
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
        throw std::runtime_error("无法解析当前可执行文件路径");
    }
    return std::filesystem::absolute(argv0);
}

void InstallService(const ServiceInstallOptions& options, const CommandRunner& runner) {
    if (options.config_path.empty()) {
        throw std::runtime_error("配置文件路径不能为空");
    }
    if (!std::filesystem::exists(options.config_path)) {
        throw std::runtime_error("配置文件不存在: " + options.config_path.string());
    }

    const std::string unit_content = BuildSystemdUnit(options);
    if (options.dry_run) {
        throw std::runtime_error("dry-run 输出应由调用方处理");
    }

    if (options.output_path.has_value()) {
        if (options.output_path->has_parent_path()) {
            std::filesystem::create_directories(options.output_path->parent_path());
        }
        std::ofstream output(*options.output_path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("无法写入服务文件: " + options.output_path->string());
        }
        output << unit_content;
        return;
    }

#ifndef _WIN32
    if (geteuid() != 0) {
        throw std::runtime_error("安装 systemd 服务需要 root 权限");
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
            throw std::runtime_error("systemctl daemon-reload 失败: " + reload.output);
        }

        enable_attempted = true;
        const CommandResult enable = runner.Run({"systemctl", "enable", "--now", service_name});
        if (enable.exit_code != 0) {
            throw std::runtime_error("systemctl enable --now 失败: " + enable.output);
        }
    } catch (const std::exception& ex) {
        std::string rollback_note = "；已尝试回滚服务文件";
        try {
            if (had_existing_file) {
                WriteFileContents(service_path, previous_content);
            } else if (std::filesystem::exists(service_path)) {
                std::filesystem::remove(service_path);
            }

            const CommandResult rollback_reload = runner.Run({"systemctl", "daemon-reload"});
            if (rollback_reload.exit_code != 0) {
                rollback_note += "，但 daemon-reload 回滚失败: " + rollback_reload.output;
            }

            if (enable_attempted) {
                const CommandResult enable_state = was_enabled
                    ? runner.Run({"systemctl", "enable", service_name})
                    : runner.Run({"systemctl", "disable", service_name});
                if (enable_state.exit_code != 0) {
                    rollback_note += was_enabled
                        ? "，且 enable 状态恢复失败: " + enable_state.output
                        : "，且 disable 状态恢复失败: " + enable_state.output;
                }

                const CommandResult active_state = was_active
                    ? runner.Run({"systemctl", "restart", service_name})
                    : runner.Run({"systemctl", "stop", service_name});
                if (active_state.exit_code != 0) {
                    rollback_note += was_active
                        ? "，且 active 状态恢复失败: " + active_state.output
                        : "，且 stop 状态恢复失败: " + active_state.output;
                }
            }
        } catch (const std::exception& rollback_ex) {
            rollback_note += "，但回滚失败: " + std::string(rollback_ex.what());
        }

        throw std::runtime_error(std::string(ex.what()) + rollback_note);
    }
}

}  // namespace ipmi_fan_control
