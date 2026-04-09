#pragma once

#include "ipmi_fan_control/process.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace ipmi_fan_control {

// 服务安装参数，支持直接安装、仅写文件或 dry-run 预览。
// Service installation options support direct install, file-only output, or dry-run preview.
struct ServiceInstallOptions {
    std::filesystem::path executable_path;
    std::filesystem::path config_path;
    std::string service_name = "ipmi-fan-control";
    std::optional<std::filesystem::path> output_path;
    bool dry_run = false;
};

std::string BuildSystemdUnit(const ServiceInstallOptions& options);
void InstallService(const ServiceInstallOptions& options, const CommandRunner& runner);
void UninstallService(const std::string& service_name, const CommandRunner& runner);
std::filesystem::path ResolveExecutablePath(const std::filesystem::path& argv0);
std::string NormalizeServiceName(const std::string& service_name);

}  // namespace ipmi_fan_control
