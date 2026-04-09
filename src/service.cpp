#include "ipmi_fan_control/service.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace ipmi_fan_control {

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
        << "ExecStart=" << options.executable_path.string()
        << " auto --config " << options.config_path.string() << "\n"
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

    const std::filesystem::path service_path = std::filesystem::path("/etc/systemd/system") / (options.service_name + ".service");
    std::ofstream service_file(service_path, std::ios::binary);
    if (!service_file) {
        throw std::runtime_error("无法写入服务文件: " + service_path.string());
    }
    service_file << unit_content;

    const CommandResult reload = runner.Run({"systemctl", "daemon-reload"});
    if (reload.exit_code != 0) {
        throw std::runtime_error("systemctl daemon-reload 失败: " + reload.output);
    }

    const CommandResult enable = runner.Run({"systemctl", "enable", "--now", options.service_name});
    if (enable.exit_code != 0) {
        throw std::runtime_error("systemctl enable --now 失败: " + enable.output);
    }
}

}  // namespace ipmi_fan_control
