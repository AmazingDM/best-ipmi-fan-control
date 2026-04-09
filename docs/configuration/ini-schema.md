# INI Configuration

Simplified Chinese: [docs/zh-CN/configuration/ini-schema.md](../zh-CN/configuration/ini-schema.md)

The repository examples and service templates now use `config.ini`.

## Sections and Keys

### `[control]`

Required keys:

- Type: integer
- `interval_seconds`: polling interval in seconds
- `full_speed_threshold`: force fan speed to `100` once this temperature is reached

Validation rules:

- `interval_seconds` must be in `1-3600`
- `full_speed_threshold` must be in `1-150`

### `[step.N]`

Create one numbered section per fan step, such as `[step.1]`, `[step.2]`, and `[step.3]`.

Required keys:

- `max_temp`: highest temperature allowed for the step
- `fan_speed`: fan speed percentage applied for the step

Validation rules:

- at least one `[step.N]` section is required
- `fan_speed` must be in `0-100`
- `max_temp` must be strictly ascending across the numbered step sections

When `auto` runs without `--config`, the built-in defaults are used instead of loading an INI file.

When the tool runs as the installed `systemd` service, editing the configured INI file can be applied with:

```bash
sudo systemctl reload ipmi-fan-control
```

The service reload path sends `SIGHUP`, re-reads the INI file, validates it, and keeps the previous configuration if the new file is invalid.

## Example

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

## Matching Rules

The tool reads the highest available temperature sensor value and returns the first step where `temperature <= max_temp`. If no step matches, or the temperature reaches `full_speed_threshold`, the fan speed is forced to `100`.

## Validation Rules

- Missing required sections or keys are rejected.
- Unknown keys should be rejected within the supported sections.
- `interval_seconds` must be in `1-3600`
- `full_speed_threshold` must be in `1-150`
- At least one `[step.N]` section is required.
- `fan_speed` must be in `0-100`
- `max_temp` must be strictly ascending
