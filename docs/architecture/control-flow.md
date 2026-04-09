# Control Flow

Simplified Chinese: [docs/zh-CN/architecture/control-flow.md](../zh-CN/architecture/control-flow.md)

## Runtime Flow

1. Parse CLI arguments and resolve the selected command.
2. For `auto`, load the INI configuration and validate it.
3. Poll `ipmitool sdr type Temperature`.
4. Extract the highest reported temperature.
5. Match the temperature against the step-based fan rules.
6. Call `ipmitool raw` only when the target fan speed changes.
7. If sensor reads or IPMI writes fail, switch to `100%` fan speed when possible and exit so `systemd` can restart the service.

## Module Responsibilities

- `cli.cpp`: command-line parsing and usage output
- `config.cpp`: strict INI loading and validation, plus built-in defaults for `auto` without `--config`
- `control.cpp`: temperature-to-speed step matching
- `ipmi.cpp`: `ipmitool` invocation and output parsing
- `service.cpp`: `systemd` unit generation and installation

## Design Notes

- Control decisions are separated from process execution so the logic is testable.
- The configuration model is intentionally simple and only supports step-based control.
- Built-in defaults are only used when `auto` runs without an INI file.
- Fan writes always switch to manual mode first to preserve the existing hardware flow.
