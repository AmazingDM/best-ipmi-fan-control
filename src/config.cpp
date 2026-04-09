#include "ipmi_fan_control/config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace ipmi_fan_control {

std::vector<FanStep> DefaultFanSteps() {
    return {
        {40, 0},
        {50, 2},
        {55, 10},
        {60, 30},
        {62, 40},
        {65, 50},
        {70, 90},
    };
}

ControlConfig DefaultControlConfig() {
    ControlConfig config;
    config.steps = DefaultFanSteps();
    return config;
}

namespace {

enum class SectionType {
    kNone,
    kControl,
    kStep,
};

struct SectionState {
    SectionType type = SectionType::kNone;
    std::string name;
    size_t line_number = 0;
    int step_index = 0;
    std::vector<std::pair<std::string, std::string>> entries;
};

std::string_view Trim(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return value;
}

std::string ToString(std::string_view value) {
    return std::string(value.begin(), value.end());
}

std::string StripComment(std::string_view line) {
    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch != '#' && ch != ';') {
            continue;
        }
        if (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1])) != 0) {
            return ToString(Trim(line.substr(0, i)));
        }
    }
    return ToString(Trim(line));
}

int ParseInt(std::string_view raw_value, const std::string& context) {
    const std::string value = ToString(Trim(raw_value));
    if (value.empty()) {
        throw std::runtime_error(context + " cannot be empty");
    }

    try {
        size_t consumed = 0;
        const int parsed = std::stoi(value, &consumed, 10);
        if (consumed != value.size()) {
            throw std::runtime_error("");
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error(context + " must be an integer");
    }
}

bool HasKey(const SectionState& section, const std::string& key) {
    return std::any_of(section.entries.begin(), section.entries.end(), [&](const auto& entry) {
        return entry.first == key;
    });
}

std::string ReadRequiredValue(const SectionState& section, const std::string& key) {
    const auto match = std::find_if(section.entries.begin(), section.entries.end(), [&](const auto& entry) {
        return entry.first == key;
    });
    if (match == section.entries.end()) {
        throw std::runtime_error("[" + section.name + "] is missing required field: " + key);
    }
    return match->second;
}

void EnsureAllowedKey(const SectionState& section, const std::string& key) {
    if (section.type == SectionType::kControl) {
        if (key == "interval_seconds" || key == "full_speed_threshold") {
            return;
        }
    } else if (section.type == SectionType::kStep) {
        if (key == "max_temp" || key == "fan_speed") {
            return;
        }
    }
    throw std::runtime_error("[" + section.name + "] contains unknown field: " + key);
}

void FinalizeSection(const SectionState& section, ControlConfig& config, bool& saw_control) {
    if (section.type == SectionType::kNone) {
        return;
    }

    if (section.type == SectionType::kControl) {
        if (saw_control) {
            throw std::runtime_error("Duplicate section: [control]");
        }
        config.interval_seconds = ParseInt(ReadRequiredValue(section, "interval_seconds"), "[control].interval_seconds");
        config.full_speed_threshold =
            ParseInt(ReadRequiredValue(section, "full_speed_threshold"), "[control].full_speed_threshold");
        saw_control = true;
        return;
    }

    // 在收尾阶段统一验证每个 step 节，避免把“读文件”和“语义校验”交织在一起，
    // 这样错误信息会稳定地指向完整的节名和键名，也更容易在后续扩展 parser 时复用。
    // Finalize each step section in one place so file reading and semantic validation stay
    // separate. That keeps error messages tied to the full section/key name and makes the
    // parser easier to extend without duplicating validation logic in the line reader.
    FanStep step;
    step.max_temp = ParseInt(ReadRequiredValue(section, "max_temp"), "[" + section.name + "].max_temp");
    step.fan_speed = ParseInt(ReadRequiredValue(section, "fan_speed"), "[" + section.name + "].fan_speed");
    config.steps.push_back(step);
}

}  // namespace

void ValidateConfig(const ControlConfig& config) {
    if (config.interval_seconds < 1 || config.interval_seconds > 3600) {
        throw std::runtime_error("interval_seconds must be between 1 and 3600");
    }

    if (config.full_speed_threshold < 1 || config.full_speed_threshold > 150) {
        throw std::runtime_error("full_speed_threshold must be between 1 and 150");
    }

    if (config.steps.empty()) {
        throw std::runtime_error("At least one [step.N] section is required");
    }

    int previous_max_temp = -1;
    for (size_t i = 0; i < config.steps.size(); ++i) {
        const FanStep& step = config.steps[i];
        if (step.max_temp < 0 || step.max_temp > 150) {
            throw std::runtime_error("steps[" + std::to_string(i) + "].max_temp is out of range");
        }
        if (step.fan_speed < 0 || step.fan_speed > 100) {
            throw std::runtime_error("steps[" + std::to_string(i) + "].fan_speed must be in the range 0-100");
        }
        if (step.max_temp <= previous_max_temp) {
            throw std::runtime_error("steps must be strictly sorted by max_temp in ascending order");
        }
        previous_max_temp = step.max_temp;
    }
}

ControlConfig LoadConfigFromFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("Config file does not exist: " + file_path.string());
    }

    std::ifstream input(file_path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to read config file: " + file_path.string());
    }

    ControlConfig config = DefaultControlConfig();
    config.steps.clear();

    SectionState current_section;
    bool saw_control = false;
    int expected_step_index = 1;
    std::string raw_line;
    size_t line_number = 0;

    while (std::getline(input, raw_line)) {
        ++line_number;

        const std::string line = StripComment(raw_line);
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            if (line.back() != ']') {
                throw std::runtime_error("Line " + std::to_string(line_number) + " is missing a closing ] in the section header");
            }

            FinalizeSection(current_section, config, saw_control);
            current_section = SectionState {};
            current_section.name = ToString(Trim(std::string_view(line).substr(1, line.size() - 2)));
            current_section.line_number = line_number;

            if (current_section.name == "control") {
                current_section.type = SectionType::kControl;
                continue;
            }

            constexpr std::string_view step_prefix = "step.";
            if (current_section.name.compare(0, step_prefix.size(), step_prefix) == 0) {
                const std::string index_text = current_section.name.substr(step_prefix.size());
                current_section.step_index = ParseInt(index_text, "[" + current_section.name + "]");
                if (current_section.step_index != expected_step_index) {
                    throw std::runtime_error(
                        "Step sections must appear sequentially as [step.1], [step.2], ...; expected [step." +
                        std::to_string(expected_step_index) + "]");
                }
                ++expected_step_index;
                current_section.type = SectionType::kStep;
                continue;
            }

            throw std::runtime_error("Unknown config section: [" + current_section.name + "]");
        }

        if (current_section.type == SectionType::kNone) {
            throw std::runtime_error("Line " + std::to_string(line_number) + " must appear inside a section");
        }

        const size_t delimiter = line.find('=');
        if (delimiter == std::string::npos) {
            throw std::runtime_error("Line " + std::to_string(line_number) + " must use the format key = value");
        }

        const std::string key = ToString(Trim(std::string_view(line).substr(0, delimiter)));
        const std::string value = ToString(Trim(std::string_view(line).substr(delimiter + 1)));
        if (key.empty()) {
            throw std::runtime_error("Line " + std::to_string(line_number) + " is missing a key name");
        }
        if (HasKey(current_section, key)) {
            throw std::runtime_error("[" + current_section.name + "] contains duplicate field: " + key);
        }
        EnsureAllowedKey(current_section, key);
        current_section.entries.emplace_back(key, value);
    }

    FinalizeSection(current_section, config, saw_control);

    if (!input.eof() && input.fail()) {
        throw std::runtime_error("Failed while reading config file: " + file_path.string());
    }

    if (!saw_control) {
        throw std::runtime_error("Missing required section: [control]");
    }

    ValidateConfig(config);
    return config;
}

std::string DescribeConfig(const ControlConfig& config) {
    std::ostringstream stream;
    stream << "Config is valid\n";
    stream << "interval_seconds: " << config.interval_seconds << "\n";
    stream << "full_speed_threshold: " << config.full_speed_threshold << "\n";
    stream << "steps:\n";
    for (const FanStep& step : config.steps) {
        stream << "  - max_temp: " << step.max_temp << ", fan_speed: " << step.fan_speed << "\n";
    }
    return stream.str();
}

}  // namespace ipmi_fan_control
