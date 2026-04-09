# INI 配置说明

English: [docs/configuration/ini-schema.md](../../configuration/ini-schema.md)

仓库中的示例和服务模板现在统一使用 `config.ini`。

## 分区与键

### `[control]`

必填键：

- 类型：整数
- `interval_seconds`：轮询间隔，单位秒
- `full_speed_threshold`：达到该温度后直接强制风扇转速为 `100`

校验规则：

- `interval_seconds` 范围为 `1-3600`
- `full_speed_threshold` 范围为 `1-150`

### `[step.N]`

每个控速阶梯使用一个编号分区，例如 `[step.1]`、`[step.2]`、`[step.3]`。

必填键：

- `max_temp`：该阶梯对应的最高温度
- `fan_speed`：命中该阶梯时设置的风扇转速百分比

校验规则：

- 至少需要一个 `[step.N]` 分区
- `fan_speed` 范围为 `0-100`
- 各个阶梯分区的 `max_temp` 必须严格升序

如果 `auto` 未提供 `--config`，程序会直接使用内置默认配置，而不是读取 INI 文件。

## 示例

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

## 匹配规则

程序会读取当前可用温度传感器中的最高值，并按顺序匹配第一个满足 `temperature <= max_temp` 的阶梯。如果没有任何阶梯命中，或者温度达到 `full_speed_threshold`，则直接将风扇转速设为 `100`。

## 校验规则

- 缺少必填分区或键会被拒绝
- 支持分区中的未知键应被拒绝
- `interval_seconds` 范围为 `1-3600`
- `full_speed_threshold` 范围为 `1-150`
- 至少需要一个 `[step.N]` 分区
- `fan_speed` 范围为 `0-100`
- `max_temp` 必须严格升序
