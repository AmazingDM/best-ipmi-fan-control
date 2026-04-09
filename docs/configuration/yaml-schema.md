# YAML 配置说明

## 字段定义

### `interval_seconds`

- 类型：整数
- 含义：温度轮询间隔，单位秒
- 默认值：`5`

### `full_speed_threshold`

- 类型：整数
- 含义：达到该温度后强制风扇转速为 `100`
- 默认值：`70`

### `steps`

- 类型：数组
- 含义：温度阶梯规则
- 要求：必须按 `max_temp` 严格升序排列

每一项包含：

- `max_temp`：该阶梯允许的最大温度
- `fan_speed`：命中该阶梯时设置的风扇转速百分比

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

程序会读取当前温度传感器中的最大值，然后按顺序查找首个满足 `temperature <= max_temp` 的阶梯。如果未命中任何阶梯，或者温度达到 `full_speed_threshold`，则直接设为 `100%`。

## 校验规则

- `interval_seconds` 范围为 `1-3600`
- `full_speed_threshold` 范围为 `1-150`
- `steps` 不能为空
- `fan_speed` 范围为 `0-100`
- `max_temp` 必须严格升序
