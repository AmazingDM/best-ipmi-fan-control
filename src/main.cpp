#include "ipmi_fan_control/cli.hpp"
#include "ipmi_fan_control/config.hpp"
#include "ipmi_fan_control/control.hpp"
#include "ipmi_fan_control/ipmi.hpp"
#include "ipmi_fan_control/logger.hpp"
#include "ipmi_fan_control/process.hpp"
#include "ipmi_fan_control/service.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>

namespace ipmi_fan_control {

namespace {

ControlConfig BuildAutoConfig(const ParsedCommand& command) {
    ControlConfig config = command.config_path.has_value() ? LoadConfigFromFile(*command.config_path) : DefaultControlConfig();

    if (command.interval_override.has_value()) {
        config.interval_seconds = *command.interval_override;
    }
    if (command.threshold_override.has_value()) {
        config.full_speed_threshold = *command.threshold_override;
    }

    ValidateConfig(config);
    return config;
}

int RunAuto(const ParsedCommand& command, const IpmiClient& client) {
    const ControlConfig config = BuildAutoConfig(command);
    Logger::Info("自动模式启动");
    Logger::Info(DescribeConfig(config));

    int last_speed = -1;
    while (true) {
        try {
            const int temperature = client.GetMaxCpuTemperature();
            const int target_speed = DetermineTargetFanSpeed(temperature, config);

            // 只有在阶梯命中结果变化时才下发 IPMI，减少不必要的 BMC 写操作。
            // Only push IPMI updates when the selected step changes to avoid noisy writes.
            if (target_speed != last_speed) {
                client.SetFanSpeed(target_speed);
                last_speed = target_speed;
                Logger::Info("温度 " + std::to_string(temperature) + "°C，对应风扇转速 " + std::to_string(target_speed) + "%");
            } else {
                Logger::Trace("温度未跨越新阶梯，跳过重复设置");
            }
        } catch (const std::exception& ex) {
            Logger::Error(ex.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(config.interval_seconds));
    }
}

}  // namespace

}  // namespace ipmi_fan_control

int main(int argc, char** argv) {
    using namespace ipmi_fan_control;

    try {
        const ParsedCommand command = ParseCommandLine(argc, argv);
        Logger::SetVerbose(command.verbose);

        if (command.type == CommandType::kHelp) {
            std::cout << BuildUsage();
            return 0;
        }

        const auto runner = std::make_shared<PosixCommandRunner>();

        if (command.type == CommandType::kInstallService) {
            ServiceInstallOptions options;
            options.executable_path = ResolveExecutablePath(command.executable_path);
            options.config_path = std::filesystem::absolute(*command.config_path);
            options.service_name = command.service_name;
            options.output_path = command.output_path.has_value()
                ? std::optional<std::filesystem::path>(std::filesystem::absolute(*command.output_path))
                : std::nullopt;
            options.dry_run = command.dry_run;

            if (options.dry_run) {
                std::cout << BuildSystemdUnit(options);
                return 0;
            }

            InstallService(options, *runner);
            Logger::Info("systemd 服务安装完成");
            return 0;
        }

        const IpmiClient client(runner);

        switch (command.type) {
            case CommandType::kInfo:
                std::cout << client.GetInfoFanTemp();
                return 0;
            case CommandType::kFixed:
                client.SetFanSpeed(*command.fixed_value);
                Logger::Info("已设置固定风扇转速 " + std::to_string(*command.fixed_value) + "%");
                return 0;
            case CommandType::kValidateConfig: {
                const ControlConfig config = LoadConfigFromFile(*command.config_path);
                std::cout << DescribeConfig(config);
                return 0;
            }
            case CommandType::kAuto:
                return RunAuto(command, client);
            case CommandType::kHelp:
            case CommandType::kInstallService:
                break;
        }

        std::cerr << "未处理的命令类型" << std::endl;
        return 1;
    } catch (const std::exception& ex) {
        Logger::Error(ex.what());
        std::cerr << "\n" << BuildUsage();
        return 1;
    }
}
