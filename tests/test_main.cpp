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
        std::filesystem::current_path() / "test-artifacts" / ("ipmi-fan-control-tests-" + std::to_string(unique_value));
    std::filesystem::create_directories(path);
    return path;
}

void WriteTextFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    output << content;
    if (!output) {
        throw std::runtime_error("Failed to write test file: " + path.string());
    }
}

void TestParseMaxTemperature() {
    const std::string sample =
        "Inlet Temp       | 04h | ok  |  7.1 | 25 degrees C\n"
        "Exhaust Temp     | 01h | ok  |  7.1 | 32 degrees C\n"
        "Temp             | 0Eh | ok  |  3.1 | 45 degrees C\n"
        "Temp             | 0Fh | ok  |  3.2 | 40 degrees C\n";

    Expect(ipmi_fan_control::ParseMaxTemperature(sample) == 45, "Expected max temperature to be 45");
}

void TestFilterInfoReport() {
    const std::string sample =
        "Fan1             | 4800 RPM          | ok\n"
        "Voltage 1        | 236 Volts         | ok\n"
        "Exhaust Temp     | 32 degrees C      | ok\n";

    const std::string filtered = ipmi_fan_control::FilterFanAndTemperatureLines(sample);
    Expect(filtered.find("Fan1") != std::string::npos, "Filtered output should keep Fan lines");
    Expect(filtered.find("Exhaust Temp") != std::string::npos, "Filtered output should keep Temp lines");
    Expect(filtered.find("Voltage 1") == std::string::npos, "Filtered output should drop Voltage lines");
}

void TestControlRule() {
    const ipmi_fan_control::ControlConfig config = ipmi_fan_control::DefaultControlConfig();
    Expect(ipmi_fan_control::DetermineTargetFanSpeed(45, config) == 2, "45C should map to 2%");
    Expect(ipmi_fan_control::DetermineTargetFanSpeed(70, config) == 100, "Threshold temperature should force 100%");
}

void TestLoadIniConfig() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "valid.ini";

    WriteTextFile(
        config_path,
        "# sample config\n"
        "[control]\n"
        "interval_seconds = 6\n"
        "full_speed_threshold = 75\n"
        "\n"
        "[step.1]\n"
        "max_temp = 45\n"
        "fan_speed = 10\n"
        "\n"
        "[step.2]\n"
        "max_temp = 60\n"
        "fan_speed = 35\n");

    const ipmi_fan_control::ControlConfig config = ipmi_fan_control::LoadConfigFromFile(config_path);
    Expect(config.interval_seconds == 6, "Expected interval_seconds to be loaded");
    Expect(config.full_speed_threshold == 75, "Expected full_speed_threshold to be loaded");
    Expect(config.steps.size() == 2, "Expected two control steps");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadIniConfigRejectsMissingField() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "missing-field.ini";

    WriteTextFile(
        config_path,
        "[control]\n"
        "interval_seconds = 6\n"
        "\n"
        "[step.1]\n"
        "max_temp = 45\n"
        "fan_speed = 10\n");

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "INI files with missing fields should be rejected");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadIniConfigRejectsUnknownField() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "unknown-field.ini";

    WriteTextFile(
        config_path,
        "[control]\n"
        "interval_seconds = 6\n"
        "full_speed_threshold = 75\n"
        "unexpected_field = true\n"
        "\n"
        "[step.1]\n"
        "max_temp = 45\n"
        "fan_speed = 10\n");

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "INI files with unknown fields should be rejected");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadIniConfigRejectsUnknownStepField() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "unknown-step-field.ini";

    WriteTextFile(
        config_path,
        "[control]\n"
        "interval_seconds = 6\n"
        "full_speed_threshold = 75\n"
        "\n"
        "[step.1]\n"
        "max_temp = 45\n"
        "fan_speed = 10\n"
        "note = unsafe\n");

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "Unknown fields inside step sections should be rejected");

    std::filesystem::remove_all(temp_dir);
}

void TestLoadIniConfigRejectsOutOfOrderSteps() {
    const std::filesystem::path temp_dir = CreateUniqueTempDirectory();
    const std::filesystem::path config_path = temp_dir / "out-of-order-step.ini";

    WriteTextFile(
        config_path,
        "[control]\n"
        "interval_seconds = 6\n"
        "full_speed_threshold = 75\n"
        "\n"
        "[step.2]\n"
        "max_temp = 45\n"
        "fan_speed = 10\n");

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::LoadConfigFromFile(config_path));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "Out-of-order step sections should be rejected");

    std::filesystem::remove_all(temp_dir);
}

void TestSystemdUnitBuilder() {
    ipmi_fan_control::ServiceInstallOptions options;
    options.executable_path = "/usr/bin/IPMI Fan/ipmi-fan-control";
    options.config_path = "/etc/ipmi fan/100%/config.ini";
    const std::string unit = ipmi_fan_control::BuildSystemdUnit(options);
    Expect(unit.find("ExecStart=\"/usr/bin/IPMI Fan/ipmi-fan-control\" auto --config \"/etc/ipmi fan/100%%/config.ini\"") != std::string::npos,
        "Service unit should escape paths containing spaces and percent signs");
}

void TestNormalizeServiceName() {
    Expect(ipmi_fan_control::NormalizeServiceName("rack-fans.service") == "rack-fans", "Expected .service suffix to be removed");
    Expect(ipmi_fan_control::NormalizeServiceName("rack_fans@bmc") == "rack_fans@bmc", "Expected a valid service name to be preserved");

    bool threw = false;
    try {
        static_cast<void>(ipmi_fan_control::NormalizeServiceName("../escape"));
    } catch (const std::exception&) {
        threw = true;
    }
    Expect(threw, "Service names containing path traversal should be rejected");
}

void TestParseCommandLineRejectsInvalidServiceName() {
    const char* argv[] = {
        "ipmi-fan-control",
        "install-service",
        "--config",
        "config.ini",
        "--service-name",
        "../escape",
    };

    bool threw_usage_error = false;
    try {
        static_cast<void>(ipmi_fan_control::ParseCommandLine(6, const_cast<char**>(argv)));
    } catch (const ipmi_fan_control::UsageError&) {
        threw_usage_error = true;
    }
    Expect(threw_usage_error, "CLI should reject an invalid service name");
}

void TestParseCommandLineDefaultsToHelp() {
    const char* argv[] = {
        "ipmi-fan-control",
    };

    const ipmi_fan_control::ParsedCommand parsed = ipmi_fan_control::ParseCommandLine(1, const_cast<char**>(argv));
    Expect(parsed.type == ipmi_fan_control::CommandType::kHelp, "CLI without arguments should default to help");
}

void TestParseCommandLineAcceptsHelpCommand() {
    const char* argv[] = {
        "ipmi-fan-control",
        "help",
    };

    const ipmi_fan_control::ParsedCommand parsed = ipmi_fan_control::ParseCommandLine(2, const_cast<char**>(argv));
    Expect(parsed.type == ipmi_fan_control::CommandType::kHelp, "The help command should parse as help");
}

void TestParseCommandLineRejectsLegacyInfoCommand() {
    const char* argv[] = {
        "ipmi-fan-control",
        "info",
    };

    bool threw_usage_error = false;
    try {
        static_cast<void>(ipmi_fan_control::ParseCommandLine(2, const_cast<char**>(argv)));
    } catch (const ipmi_fan_control::UsageError&) {
        threw_usage_error = true;
    }
    Expect(threw_usage_error, "The legacy info command should be rejected");
}

void TestBuildUsageLooksLikeStandardHelp() {
    const std::string usage = ipmi_fan_control::BuildUsage();
    Expect(usage.find("Usage:\n") != std::string::npos, "Help text should contain a Usage section");
    Expect(usage.find("Commands:\n") != std::string::npos, "Help text should contain a Commands section");
    Expect(usage.find("Options:\n") != std::string::npos, "Help text should contain an Options section");
    Expect(usage.find("Examples:\n") != std::string::npos, "Help text should contain an Examples section");
    Expect(usage.find("  help\n") != std::string::npos, "Help text should list the help command");
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"ParseMaxTemperature", TestParseMaxTemperature},
        {"FilterInfoReport", TestFilterInfoReport},
        {"ControlRule", TestControlRule},
        {"LoadIniConfig", TestLoadIniConfig},
        {"LoadIniConfigRejectsMissingField", TestLoadIniConfigRejectsMissingField},
        {"LoadIniConfigRejectsUnknownField", TestLoadIniConfigRejectsUnknownField},
        {"LoadIniConfigRejectsUnknownStepField", TestLoadIniConfigRejectsUnknownStepField},
        {"LoadIniConfigRejectsOutOfOrderSteps", TestLoadIniConfigRejectsOutOfOrderSteps},
        {"SystemdUnitBuilder", TestSystemdUnitBuilder},
        {"NormalizeServiceName", TestNormalizeServiceName},
        {"ParseCommandLineRejectsInvalidServiceName", TestParseCommandLineRejectsInvalidServiceName},
        {"ParseCommandLineDefaultsToHelp", TestParseCommandLineDefaultsToHelp},
        {"ParseCommandLineAcceptsHelpCommand", TestParseCommandLineAcceptsHelpCommand},
        {"ParseCommandLineRejectsLegacyInfoCommand", TestParseCommandLineRejectsLegacyInfoCommand},
        {"BuildUsageLooksLikeStandardHelp", TestBuildUsageLooksLikeStandardHelp},
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
