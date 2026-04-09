#include "ipmi_fan_control/cli.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace ipmi_fan_control {

namespace {

int ParseIntValue(const std::string& raw_value, const std::string& option_name) {
    try {
        size_t consumed = 0;
        const int value = std::stoi(raw_value, &consumed, 10);
        if (consumed != raw_value.size()) {
            throw std::runtime_error("");
        }
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("参数无效: " + option_name + "=" + raw_value);
    }
}

std::string RequireValue(const std::vector<std::string>& args, size_t& index, const std::string& option_name) {
    if (index + 1 >= args.size()) {
        throw std::runtime_error("缺少参数值: " + option_name);
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
        return parsed;
    }

    if (command == "info") {
        parsed.type = CommandType::kInfo;
        if (args.size() != 1) {
            throw std::runtime_error("info 命令不接受额外参数");
        }
        return parsed;
    }

    if (command == "fixed") {
        parsed.type = CommandType::kFixed;
        if (args.size() != 2) {
            throw std::runtime_error("fixed 命令缺少风扇转速值");
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
                throw std::runtime_error("未知参数: " + token);
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
                throw std::runtime_error("未知参数: " + token);
            }
        }
        if (!parsed.config_path.has_value()) {
            throw std::runtime_error("validate-config 需要提供 --config");
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
                parsed.service_name = RequireValue(args, i, token);
            } else if (token == "--output") {
                parsed.output_path = RequireValue(args, i, token);
            } else if (token == "--dry-run") {
                parsed.dry_run = true;
            } else {
                throw std::runtime_error("未知参数: " + token);
            }
        }
        if (!parsed.config_path.has_value()) {
            throw std::runtime_error("install-service 需要提供 --config");
        }
        return parsed;
    }

    throw std::runtime_error("未知命令: " + command);
}

std::string BuildUsage() {
    std::ostringstream stream;
    stream
        << "用法:\n"
        << "  ipmi-fan-control [--verbose] <command> [options]\n\n"
        << "命令:\n"
        << "  info\n"
        << "      输出风扇与温度信息。\n"
        << "  fixed <value>\n"
        << "      设置固定风扇转速百分比，范围 0-100。\n"
        << "  auto [--config <path>] [--interval <sec>] [--threshold <temp>]\n"
        << "      自动模式，可选使用 YAML 配置。\n"
        << "  validate-config --config <path>\n"
        << "      校验 YAML 配置是否合法。\n"
        << "  install-service --config <path> [--service-name <name>] [--output <file>] [--dry-run]\n"
        << "      安装或生成 systemd 服务文件。\n\n"
        << "全局选项:\n"
        << "  --verbose\n"
        << "      输出更详细的日志。\n";
    return stream.str();
}

}  // namespace ipmi_fan_control
