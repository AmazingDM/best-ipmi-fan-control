# ipmi-fan-control

[![C++ CI](https://github.com/AmazingDM/best-ipmi-fan-control/actions/workflows/ci.yml/badge.svg)](https://github.com/AmazingDM/best-ipmi-fan-control/actions/workflows/ci.yml)

一个基于 C++ 的 IPMI 风扇控制工具。它通过调用 `ipmitool` 读取温度传感器数据，并根据 YAML 配置中的阶梯规则设置风扇转速，适用于 Linux + `systemd` 的服务器环境。

## 功能特性

- 使用直观的 YAML 文件定义温度与风扇转速的阶梯关系
- 保留 `info`、`fixed`、`auto` 命令行模式
- 支持 `validate-config` 校验配置文件
- 支持 `install-service` 一键注册 `systemd` 服务并开机自启
- 提供 GitHub Actions 自动构建、测试与 Release 发布流程

## 目录说明

- `include/`：公开头文件
- `src/`：核心实现
- `tests/`：单元测试
- `examples/`：示例 YAML 配置
- `etc/systemd/system/`：示例服务文件
- `docs/`：迁移计划、架构说明、配置说明与 CI 文档

## 依赖要求

运行环境：

- Linux
- `ipmitool`
- `systemd`

构建依赖：

- CMake 3.20+
- 支持 C++17 的编译器
- `yaml-cpp`

Ubuntu / Debian 示例：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build libyaml-cpp-dev ipmitool
```

## 构建方法

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 快速开始

查看风扇与温度信息：

```bash
./build/ipmi-fan-control info
```

设置固定风扇转速：

```bash
./build/ipmi-fan-control fixed 35
```

使用示例配置启动自动控速：

```bash
./build/ipmi-fan-control auto --config examples/config.example.yaml
```

校验配置文件：

```bash
./build/ipmi-fan-control validate-config --config examples/config.example.yaml
```

预览将要生成的 `systemd` 服务文件：

```bash
./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.yaml --dry-run
```

安装并启用 `systemd` 服务：

```bash
sudo ./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.yaml
```

## YAML 配置示例

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

规则说明：

- 程序每次轮询后会选取当前温度传感器中的最大值作为控制依据
- 依次匹配首个 `temperature <= max_temp` 的阶梯
- 如果温度高于所有阶梯，或达到 `full_speed_threshold`，则强制风扇为 `100%`

## GitHub Actions

- `ci.yml`：在 `push` 和 `pull_request` 时自动构建并运行测试
- `release.yml`：手动输入版本号后构建 `Linux x86_64` 与 `Linux arm64` 包，并上传到 GitHub Release

更多说明见：

- [docs/plans/cpp-migration-plan.md](docs/plans/cpp-migration-plan.md)
- [docs/architecture/control-flow.md](docs/architecture/control-flow.md)
- [docs/configuration/yaml-schema.md](docs/configuration/yaml-schema.md)
- [docs/ci/github-actions.md](docs/ci/github-actions.md)

## 免责声明

此工具会直接控制服务器风扇，配置错误可能导致温度过高、噪声异常或硬件保护触发。请先在受控环境中验证 YAML 配置，再部署到生产环境。
