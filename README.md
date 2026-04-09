# ipmi-fan-control

[![C++ CI](https://github.com/AmazingDM/better-ipmi-fan-control/actions/workflows/ci.yml/badge.svg)](https://github.com/AmazingDM/better-ipmi-fan-control/actions/workflows/ci.yml)

Step-based IPMI fan control for Linux servers, implemented in C++. The tool reads temperature sensors through `ipmitool`, maps the highest temperature to an INI-defined fan speed step, and can install itself as a `systemd` service.

中文文档: [docs/zh-CN/README.md](docs/zh-CN/README.md)

## Features

- Step-based fan control driven by a single INI file
- CLI commands for `help`, `fixed`, `auto`, `validate-config`, and `install-service`
- `systemd` service installation with a specified config path
- GitHub Actions CI for build and test validation
- Manual GitHub Release workflow for Linux `x86_64` and `arm64`

## Documentation

- [Documentation index](docs/README.md)
- [Architecture overview](docs/architecture/control-flow.md)
- [INI configuration reference](docs/configuration/ini-schema.md)
- [GitHub Actions workflows](docs/ci/github-actions.md)
- [C++ migration summary](docs/plans/cpp-migration-plan.md)

## Requirements

Runtime:

- Linux
- `ipmitool`
- `systemd`

Build:

- CMake 3.20+
- A C++17 compiler

Ubuntu / Debian example:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build ipmitool
```

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

GitHub Releases publish one Linux executable per target architecture plus a `.sha256` checksum. Release assets no longer bundle a `lib/` directory or any extra parser runtime.

## Quick Start

Show the built-in help:

```bash
./build/ipmi-fan-control
```

Set a fixed fan speed:

```bash
./build/ipmi-fan-control fixed 35
```

Run automatic control with an INI file:

```bash
./build/ipmi-fan-control auto --config examples/config.example.ini
```

Validate an INI file:

```bash
./build/ipmi-fan-control validate-config --config examples/config.example.ini
```

Preview the generated `systemd` unit:

```bash
./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.ini --dry-run
```

Install and enable the `systemd` service:

```bash
sudo ./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.ini
```

Manage the service after installation:

```bash
sudo systemctl start ipmi-fan-control
sudo systemctl stop ipmi-fan-control
sudo systemctl restart ipmi-fan-control
sudo systemctl reload ipmi-fan-control
systemctl status ipmi-fan-control
```

After editing `config.ini`, run `sudo systemctl reload ipmi-fan-control` to hot-reload the file without restarting the process. If you change the unit file itself, run `sudo systemctl daemon-reload` before restarting or reloading the service.

## Example INI

```ini
[control]
interval_seconds = 5
full_speed_threshold = 70

[step.1]
max_temp = 40
fan_speed = 0

[step.2]
max_temp = 50
fan_speed = 10

[step.3]
max_temp = 58
fan_speed = 30

[step.4]
max_temp = 64
fan_speed = 50

[step.5]
max_temp = 69
fan_speed = 80
```

Matching rules:

- The tool uses the highest temperature from the available temperature sensors.
- It returns the first step where `temperature <= max_temp`.
- If the temperature exceeds all steps, or reaches `full_speed_threshold`, fan speed is forced to `100`.

## Repository Layout

- `include/`: public headers
- `src/`: core implementation
- `tests/`: unit tests
- `examples/`: example configuration files
- `etc/systemd/system/`: service templates
- `docs/`: project documentation
- `docs/zh-CN/`: Simplified Chinese translations of the main docs

## Safety Notice

This tool directly controls server fan speed. A bad configuration may cause overheating, excessive noise, or hardware protection events. Validate the INI file in a controlled environment before deploying it on production hardware.
