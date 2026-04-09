#include "ipmi_fan_control/cli.hpp"
#include "ipmi_fan_control/config.hpp"
#include "ipmi_fan_control/control.hpp"
#include "ipmi_fan_control/ipmi.hpp"
#include "ipmi_fan_control/service.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::filesystem::path CreateUniqueTempDirectory() {
    const auto unique_value = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("ipmi-fan-control-tests-" + std::to_string(unique_value));
    std::filesystem::create_directories(path);
    return path;
}

void TestParseMaxTemperature() {
    const std::string sample =
        "Inlet Temp       | 04h | ok  |  7.1 | 25 degrees C\n"
        "Exhaust Temp     | 01h | ok  |  7.1 | 32 degrees C\n"
        "Temp             | 0Eh | ok  |  3.1 | 45 degrees C\n"
        "Temp             | 0Fh | ok  |  3.2 | 40 degrees C\n";

    Expect(ipmi_fan_control::ParseMaxTemperature(sample) == 45, "应当解析出最大温度 45");
}

void TestFilterInfoReport() {
    const std::string sample =
        "Fan1             | 4800 RPM          | ok\n"
        "Voltage 1        | 236 Volts         | ok\n"
        "Exhaust Temp     | 32 degrees C      | ok\n";

    const std::string filtered = ipmi_fan_control::FilterFanAndTemperatureLines(sample);
    Expect(filtered.find("Fan1") != std::string::npos, "过滤结果应保留 Fan 行");
    Expect(filtered.find("Exhaust Temp") != std::string::npos, "过滤结果应保留 Temp 行");
    Expect(filtered.find("Voltage 1") == std::string::npos, "过滤结果不应保留 Voltage 行");
}

void TestControlRule() {
    const ipmi_fan_control::ControlConfig config = ipmi_fan_control::DefaultControlConfig();
    Expect(ipmi_fan_control::DetermineTargetFanSpeed(45, config) == 2, "45 度应命中 2%");
    Expect(ipmi_fan_control::DetermineTargetFanSpeed(70, config) == 100, "达到阈值应强制 100%");
}

void TestLoadYamlConfig() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "valid.yaml";

    std::ofstream output(config_path, std::ios::binary);
    output
        << "interval_seconds: 6\n"
        << "full_speed_threshold: 75\n"
        << "steps:\n"
        << "  - max_temp: 45\n"
        << "    fan_speed: 10\n"
        << "  - max_temp: 60\n"
        << "    fan_speed: 35\n";
    output.close();

    const ipmi_fan_control::ControlConfig config = ipmi_fan_control::LoadConfigFromFile(config_path);
    Expect(config.interval_seconds == 6, "应读取 interval_seconds");
    Expect(config.full_speed_threshold == 75, "应读取 full_speed_threshold");
    Expect(config.steps.size() == 2, "应读取两条阶梯");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadYamlConfigRejectsMissingField() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "missing-field.yaml";

    std::ofstream output(config_path, std::ios::binary);
    output
        << "interval_seconds: 6\n"
        << "steps:\n"
        << "  - max_temp: 45\n"
        << "    fan_speed: 10\n";
    output.close();

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "缺少字段的 YAML 应被拒绝");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadYamlConfigRejectsUnknownField() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "unknown-field.yaml";

    std::ofstream output(config_path, std::ios::binary);
    output
        << "interval_seconds: 6\n"
        << "full_speed_threshold: 75\n"
        << "steps:\n"
        << "  - max_temp: 45\n"
        << "    fan_speed: 10\n"
        << "unexpected_field: true\n";
    output.close();

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "包含未知字段的 YAML 应被拒绝");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadYamlConfigRejectsUnknownStepField() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "unknown-step-field.yaml";

    std::ofstream output(config_path, std::ios::binary);
    output
        << "interval_seconds: 6\n"
        << "full_speed_threshold: 75\n"
        << "steps:\n"
        << "  - max_temp: 45\n"
        << "    fan_speed: 10\n"
        << "    note: unsafe\n";
    output.close();

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "steps 内的未知字段应被拒绝");

    std::filesystem::remove_all(temp_dir);
}

void TestSystemdUnitBuilder() {
    ipmi_fan_control::ServiceInstallOptions options;
    options.executable_path = "/usr/bin/IPMI Fan/ipmi-fan-control";
    options.config_path = "/etc/ipmi fan/100%/config.yaml";
    const std::string unit = ipmi_fan_control::BuildSystemdUnit(options);
    Expect(unit.find("ExecStart=\"/usr/bin/IPMI Fan/ipmi-fan-control\" auto --config \"/etc/ipmi fan/100%%/config.yaml\"") != std::string::npos,
        "服务文件应正确转义带空格和百分号的路径");
}

void TestNormalizeServiceName() {
    Expect(ipmi_fan_control::NormalizeServiceName("rack-fans.service") == "rack-fans", "应去掉 .service 后缀");
    Expect(ipmi_fan_control::NormalizeServiceName("rack_fans@bmc") == "rack_fans@bmc", "合法 service-name 应保留");

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::NormalizeServiceName("../escape"));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "包含路径穿越内容的 service-name 应被拒绝");
}

void TestParseCommandLineRejectsInvalidServiceName() {
    const char* argv[] = {
        "ipmi-fan-control",
        "install-service",
        "--config",
        "config.yaml",
        "--service-name",
        "../escape",
    };

    bool threw_usage_error = false;
    try {
        static_cast<void>(ipmi_fan_control::ParseCommandLine(6, const_cast<char**>(argv)));
    } catch (const ipmi_fan_control::UsageError&) {
        threw_usage_error = true;
    }
    Expect(threw_usage_error, "CLI 应拒绝非法 service-name");
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"ParseMaxTemperature", TestParseMaxTemperature},
        {"FilterInfoReport", TestFilterInfoReport},
        {"ControlRule", TestControlRule},
        {"LoadYamlConfig", TestLoadYamlConfig},
        {"LoadYamlConfigRejectsMissingField", TestLoadYamlConfigRejectsMissingField},
        {"LoadYamlConfigRejectsUnknownField", TestLoadYamlConfigRejectsUnknownField},
        {"LoadYamlConfigRejectsUnknownStepField", TestLoadYamlConfigRejectsUnknownStepField},
        {"SystemdUnitBuilder", TestSystemdUnitBuilder},
        {"NormalizeServiceName", TestNormalizeServiceName},
        {"ParseCommandLineRejectsInvalidServiceName", TestParseCommandLineRejectsInvalidServiceName},
    };

    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "[PASS] " << test.first << "\n";
        } catch (const std::exception& ex) {
            std::cerr << "[FAIL] " << test.first << ": " << ex.what() << "\n";
            return 1;
        }
    }

    return 0;
}
