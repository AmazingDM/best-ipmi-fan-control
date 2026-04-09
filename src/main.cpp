#include "ipmi_fan_control/cli.hpp"
#include "ipmi_fan_control/config.hpp"
#include "ipmi_fan_control/control.hpp"
#include "ipmi_fan_control/ipmi.hpp"
#include "ipmi_fan_control/logger.hpp"
#include "ipmi_fan_control/process.hpp"
#include "ipmi_fan_control/service.hpp"

#include <chrono>
#include <csignal>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>

namespace ipmi_fan_control {

namespace {

volatile std::sig_atomic_t g_reload_requested = 0;

extern "C" void HandleReloadSignal(int signal_number) {
    if (signal_number == SIGHUP) {
        g_reload_requested = 1;
    }
}

void InstallReloadSignalHandler() {
#ifndef _WIN32
    struct sigaction action {};
    action.sa_handler = HandleReloadSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGHUP, &action, nullptr) != 0) {
        throw std::runtime_error("Failed to install SIGHUP handler");
    }
#endif
}

bool ConsumeReloadRequest() {
    if (g_reload_requested == 0) {
        return false;
    }
    g_reload_requested = 0;
    return true;
}

void WaitForNextIteration(int interval_seconds) {
    int remaining_seconds = interval_seconds;
    while (remaining_seconds > 0 && g_reload_requested == 0) {
        const int chunk_seconds = remaining_seconds > 1 ? 1 : remaining_seconds;
        std::this_thread::sleep_for(std::chrono::seconds(chunk_seconds));
        remaining_seconds -= chunk_seconds;
    }
}

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
    ControlConfig config = BuildAutoConfig(command);
    Logger::Info("Automatic mode started");
    Logger::Info(DescribeConfig(config));

    int last_speed = -1;
    while (true) {
        if (ConsumeReloadRequest()) {
            if (!command.config_path.has_value()) {
                Logger::Error("Received reload request, but auto mode is using built-in defaults");
            } else {
                try {
                    config = BuildAutoConfig(command);
                    Logger::Info("Reloaded configuration from " + command.config_path->string());
                    Logger::Info(DescribeConfig(config));
                } catch (const std::exception& ex) {
                    Logger::Error("Config reload failed; keeping the previous configuration: " + std::string(ex.what()));
                }
            }
        }

        try {
            const int temperature = client.GetMaxCpuTemperature();
            const int target_speed = DetermineTargetFanSpeed(temperature, config);

            // 只有在阶梯命中结果变化时才下发 IPMI，减少不必要的 BMC 写操作。
            // Only push IPMI updates when the selected step changes to avoid noisy writes.
            if (target_speed != last_speed) {
                client.SetFanSpeed(target_speed);
                last_speed = target_speed;
                Logger::Info("Temperature " + std::to_string(temperature) + "°C -> fan speed " + std::to_string(target_speed) + "%");
            } else {
                Logger::Trace("Temperature stayed within the same step; skipping duplicate fan update");
            }
        } catch (const std::exception& ex) {
            Logger::Error("Automatic fan control failed: " + std::string(ex.what()));
            try {
                // 出现异常时强制切到 100% 并立刻退出，让 systemd 的重启策略接管。
                // On any control-loop failure, force 100% fan speed and exit immediately so
                // systemd can restart the service instead of leaving the chassis in a risky state.
                client.SetFanSpeed(100);
                Logger::Error("Switched fans to 100% and exited so systemd can apply the restart policy");
            } catch (const std::exception& fail_safe_ex) {
                Logger::Error("Failed to switch fans to 100%: " + std::string(fail_safe_ex.what()));
            }
            return 1;
        }

        WaitForNextIteration(config.interval_seconds);
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

        if (command.type == CommandType::kAuto) {
            InstallReloadSignalHandler();
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
            Logger::Info("systemd service installation completed");
            return 0;
        }

        if (command.type == CommandType::kUninstallService) {
            UninstallService(command.service_name, *runner);
            Logger::Info("systemd service uninstallation completed");
            return 0;
        }

        const IpmiClient client(runner);

        switch (command.type) {
            case CommandType::kFixed:
                client.SetFanSpeed(*command.fixed_value);
                Logger::Info("Set fixed fan speed to " + std::to_string(*command.fixed_value) + "%");
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
            case CommandType::kUninstallService:
                break;
        }

        std::cerr << "Unhandled command type" << std::endl;
        return 1;
    } catch (const UsageError& ex) {
        Logger::Error(ex.what());
        std::cerr << "\n" << BuildUsage();
        return 1;
    } catch (const std::exception& ex) {
        Logger::Error(ex.what());
        return 1;
    }
}
