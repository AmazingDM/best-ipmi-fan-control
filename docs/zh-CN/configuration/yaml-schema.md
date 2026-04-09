# YAML 配置说明

English: [docs/configuration/yaml-schema.md](../../configuration/yaml-schema.md)

## 字段定义

### `interval_seconds`

- 类型：整数
- 含义：轮询间隔，单位秒
- 在 YAML 文件中必填：是

### `full_speed_threshold`

- 类型：整数
- 含义：达到该温度后直接强制风扇转速为 `100`
- 在 YAML 文件中必填：是

### `steps`

- 类型：数组
- 含义：阶梯式控速规则
- 要求：按 `max_temp` 严格升序排列
- 在 YAML 文件中必填：是

每个阶梯包含：

- `max_temp`：该阶梯对应的最高温度
- `fan_speed`：命中该阶梯时设置的风扇转速百分比

如果 `auto` 未提供 `--config`，程序会直接使用内置默认配置，而不是读取 YAML 文件。

## 示例

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

## 匹配规则

程序会读取当前可用温度传感器中的最高值，并按顺序匹配第一个满足 `temperature <= max_temp` 的阶梯。如果没有任何阶梯命中，或者温度达到 `full_speed_threshold`，则直接将风扇转速设为 `100`。

## 校验规则

- 顶层未知字段会被拒绝
- `steps` 条目中的未知字段会被拒绝
- 缺少必填字段会被拒绝
- `interval_seconds` 范围为 `1-3600`
- `full_speed_threshold` 范围为 `1-150`
- `steps` 不能为空
- `fan_speed` 范围为 `0-100`
- `max_temp` 必须严格升序
