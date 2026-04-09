# YAML Configuration

Simplified Chinese: [docs/zh-CN/configuration/yaml-schema.md](../zh-CN/configuration/yaml-schema.md)

## Fields

### `interval_seconds`

- Type: integer
- Meaning: polling interval in seconds
- Required in YAML files: yes

### `full_speed_threshold`

- Type: integer
- Meaning: force fan speed to `100` once this temperature is reached
- Required in YAML files: yes

### `steps`

- Type: array
- Meaning: ordered fan control steps
- Rule: entries must be sorted by `max_temp` in strictly ascending order
- Required in YAML files: yes

Each step contains:

- `max_temp`: highest temperature allowed for the step
- `fan_speed`: fan speed percentage applied for the step

When `auto` runs without `--config`, the built-in defaults are used instead of loading a YAML file.

## Example

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

## Matching Rules

The tool reads the highest available temperature sensor value and returns the first step where `temperature <= max_temp`. If no step matches, or the temperature reaches `full_speed_threshold`, the fan speed is forced to `100`.

## Validation Rules

- Unknown top-level fields are rejected.
- Unknown fields inside `steps` entries are rejected.
- Missing required fields are rejected.
- `interval_seconds` must be in `1-3600`
- `full_speed_threshold` must be in `1-150`
- `steps` cannot be empty
- `fan_speed` must be in `0-100`
- `max_temp` must be strictly ascending
