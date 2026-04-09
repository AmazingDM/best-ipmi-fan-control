# ipmi-fan-control

[![C++ CI](https://github.com/AmazingDM/better-ipmi-fan-control/actions/workflows/ci.yml/badge.svg)](https://github.com/AmazingDM/better-ipmi-fan-control/actions/workflows/ci.yml)

Step-based IPMI fan control for Linux servers, implemented in C++. The tool reads temperature sensors through `ipmitool`, maps the highest temperature to a YAML-defined fan speed step, and can install itself as a `systemd` service.

中文文档: [docs/zh-CN/README.md](docs/zh-CN/README.md)

## Features

- Step-based fan control driven by a single YAML file
- CLI commands for `info`, `fixed`, `auto`, `validate-config`, and `install-service`
- `systemd` service installation with a specified config path
- GitHub Actions CI for build and test validation
- Manual GitHub Release workflow for Linux `x86_64` and `arm64`

## Documentation

- [Documentation index](docs/README.md)
- [Architecture overview](docs/architecture/control-flow.md)
- [YAML configuration reference](docs/configuration/yaml-schema.md)
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
- `yaml-cpp`

Ubuntu / Debian example:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build libyaml-cpp-dev ipmitool
```

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Quick Start

Print fan and temperature information:

```bash
./build/ipmi-fan-control info
```

Set a fixed fan speed:

```bash
./build/ipmi-fan-control fixed 35
```

Run automatic control with a YAML file:

```bash
./build/ipmi-fan-control auto --config examples/config.example.yaml
```

Validate a YAML file:

```bash
./build/ipmi-fan-control validate-config --config examples/config.example.yaml
```

Preview the generated `systemd` unit:

```bash
./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.yaml --dry-run
```

Install and enable the `systemd` service:

```bash
sudo ./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.yaml
```

## Example YAML

```yaml
interval_seconds: 5
full_speed_threshold: 70

steps:
  - max_temp: 40
    fan_speed: 0
  - max_temp: 50
    fan_speed: 10
  - max_temp: 58
    fan_speed: 30
  - max_temp: 64
    fan_speed: 50
  - max_temp: 69
    fan_speed: 80
```

Matching rules:

- The tool uses the highest temperature from the available temperature sensors.
- It returns the first step where `temperature <= max_temp`.
- If the temperature exceeds all steps, or reaches `full_speed_threshold`, fan speed is forced to `100`.

## Repository Layout

- `include/`: public headers
- `src/`: core implementation
- `tests/`: unit tests
- `examples/`: example YAML files
- `etc/systemd/system/`: service templates
- `docs/`: project documentation
- `docs/zh-CN/`: Simplified Chinese translations of the main docs

## Safety Notice

This tool directly controls server fan speed. A bad configuration may cause overheating, excessive noise, or hardware protection events. Validate the YAML file in a controlled environment before deploying it on production hardware.
