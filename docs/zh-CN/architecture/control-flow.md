# 控制流程说明

English: [docs/architecture/control-flow.md](../../architecture/control-flow.md)

## 运行流程

1. 解析 CLI 参数并确定子命令。
2. 对于 `auto` 模式，加载并校验 INI 配置。
3. 周期性执行 `ipmitool sdr type Temperature`。
4. 提取输出中的最高温度。
5. 使用阶梯式规则计算目标风扇转速。
6. 仅当目标转速发生变化时才调用 `ipmitool raw`。
7. 如果温度读取或 IPMI 写入失败，会在可能的情况下先切到 `100%` 风扇转速，再退出并交给 `systemd` 重启服务。

## 模块职责

- `cli.cpp`：命令行解析与帮助输出
- `config.cpp`：严格的 INI 加载与校验，以及 `auto` 未提供 `--config` 时的内置默认值
- `control.cpp`：温度到转速的阶梯匹配
- `ipmi.cpp`：`ipmitool` 调用与输出解析
- `service.cpp`：`systemd` 服务文件生成与安装

## 设计说明

- 控制决策和进程执行逻辑分离，便于测试。
- 配置模型刻意保持简单，只支持阶梯式控速。
- 内置默认值只在 `auto` 未提供 INI 文件时使用。
- 写入风扇前会先切换到手动模式，保持既有硬件控制流程。
