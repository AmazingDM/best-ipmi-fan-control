#include "ipmi_fan_control/cli.hpp"
#include "ipmi_fan_control/service.hpp"

#include <sstream>
#include <vector>

namespace ipmi_fan_control {

namespace {

int ParseIntValue(const std::string& raw_value, const std::string& option_name) {
    try {
        size_t consumed = 0;
        const int value = std::stoi(raw_value, &consumed, 10);
        if (consumed != raw_value.size()) {
            throw UsageError("");
        }
        return value;
    } catch (const std::exception&) {
        throw UsageError("Invalid value: " + option_name + "=" + raw_value);
    }
}

std::string RequireValue(const std::vector<std::string>& args, size_t& index, const std::string& option_name) {
    if (index + 1 >= args.size()) {
        throw UsageError("Missing value for option: " + option_name);
    }
    ++index;
    return args[index];
}

}  // namespace

ParsedCommand ParseCommandLine(int argc, char** argv) {
    ParsedCommand parsed;
    if (argc > 0 && argv != nullptr) {
        parsed.executable_path = argv[0];
    }

    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc > 1 ? argc - 1 : 0));

    for (int i = 1; i < argc; ++i) {
        const std::string token = argv[i];
        if (token == "--verbose") {
            parsed.verbose = true;
            continue;
        }
        args.push_back(token);
    }

    if (args.empty()) {
        parsed.type = CommandType::kHelp;
        return parsed;
    }

    const std::string command = args.front();
    if (command == "-h" || command == "--help" || command == "help") {
        parsed.type = CommandType::kHelp;
        if (args.size() != 1) {
            throw UsageError("The help command does not accept extra arguments");
        }
        return parsed;
    }

    if (command == "-v" || command == "--version" || command == "version") {
        parsed.type = CommandType::kVersion;
        if (args.size() != 1) {
            throw UsageError("The version command does not accept extra arguments");
        }
        return parsed;
    }

    if (command == "fixed") {
        parsed.type = CommandType::kFixed;
        if (args.size() != 2) {
            throw UsageError("The fixed command requires a fan speed value");
        }
        parsed.fixed_value = ParseIntValue(args[1], "value");
        return parsed;
    }

    if (command == "auto") {
        parsed.type = CommandType::kAuto;
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string& token = args[i];
            if (token == "--config") {
                parsed.config_path = RequireValue(args, i, token);
            } else if (token == "--interval") {
                parsed.interval_override = ParseIntValue(RequireValue(args, i, token), token);
            } else if (token == "--threshold") {
                parsed.threshold_override = ParseIntValue(RequireValue(args, i, token), token);
            } else {
                throw UsageError("Unknown argument: " + token);
            }
        }
        return parsed;
    }

    if (command == "validate-config") {
        parsed.type = CommandType::kValidateConfig;
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string& token = args[i];
            if (token == "--config") {
                parsed.config_path = RequireValue(args, i, token);
            } else {
                throw UsageError("Unknown argument: " + token);
            }
        }
        if (!parsed.config_path.has_value()) {
            throw UsageError("The validate-config command requires --config");
        }
        return parsed;
    }

    if (command == "install-service") {
        parsed.type = CommandType::kInstallService;
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string& token = args[i];
            if (token == "--config") {
                parsed.config_path = RequireValue(args, i, token);
            } else if (token == "--service-name") {
                try {
                    parsed.service_name = NormalizeServiceName(RequireValue(args, i, token));
                } catch (const std::exception& ex) {
                    throw UsageError(ex.what());
                }
            } else if (token == "--output") {
                parsed.output_path = RequireValue(args, i, token);
            } else if (token == "--dry-run") {
                parsed.dry_run = true;
            } else {
                throw UsageError("Unknown argument: " + token);
            }
        }
        if (!parsed.config_path.has_value()) {
            throw UsageError("The install-service command requires --config");
        }
        try {
            parsed.service_name = NormalizeServiceName(parsed.service_name);
        } catch (const std::exception& ex) {
            throw UsageError(ex.what());
        }
        return parsed;
    }

    if (command == "uninstall-service") {
        parsed.type = CommandType::kUninstallService;
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string& token = args[i];
            if (token == "--service-name") {
                try {
                    parsed.service_name = NormalizeServiceName(RequireValue(args, i, token));
                } catch (const std::exception& ex) {
                    throw UsageError(ex.what());
                }
            } else {
                throw UsageError("Unknown argument: " + token);
            }
        }
        try {
            parsed.service_name = NormalizeServiceName(parsed.service_name);
        } catch (const std::exception& ex) {
            throw UsageError(ex.what());
        }
        return parsed;
    }

    throw UsageError("Unknown command: " + command);
}

std::string BuildUsage() {
    std::ostringstream stream;
    stream
        << "ipmi-fan-control\n"
        << "Step-based IPMI fan control for Linux servers.\n\n"
        << "Usage:\n"
        << "  ipmi-fan-control [--verbose] <command> [options]\n"
        << "  ipmi-fan-control help\n"
        << "  ipmi-fan-control --help\n\n"
        << "Commands:\n"
        << "  help\n"
        << "      Show this help text.\n"
        << "  version\n"
        << "      Show the build version.\n"
        << "  fixed <value>\n"
        << "      Set a fixed fan speed percentage in the range 0-100.\n"
        << "  auto [--config <path>] [--interval <sec>] [--threshold <temp>]\n"
        << "      Run automatic control mode, optionally using an INI config.\n"
        << "  validate-config --config <path>\n"
        << "      Validate that an INI config is accepted.\n"
        << "  install-service --config <path> [--service-name <name>] [--output <file>] [--dry-run]\n"
        << "      Install or generate a systemd service file.\n"
        << "  uninstall-service [--service-name <name>]\n"
        << "      Stop, disable, and remove an installed systemd service.\n\n"
        << "Options:\n"
        << "  -h, --help\n"
        << "      Show this help text.\n"
        << "  --verbose\n"
        << "      Print more detailed logs.\n\n"
        << "Examples:\n"
        << "  ipmi-fan-control\n"
        << "  ipmi-fan-control help\n"
        << "  ipmi-fan-control --version\n"
        << "  ipmi-fan-control fixed 35\n"
        << "  ipmi-fan-control auto --config examples/config.example.ini\n"
        << "  ipmi-fan-control validate-config --config examples/config.example.ini\n"
        << "  ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.ini --dry-run\n"
        << "  ipmi-fan-control uninstall-service\n";
    return stream.str();
}

}  // namespace ipmi_fan_control
