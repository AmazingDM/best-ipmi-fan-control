# Contributing

Thanks for contributing. This project touches hardware control paths, so correctness, readability, and safe defaults matter more than cleverness.

## Local Development

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Documentation

- Keep end-user and engineering docs direct, factual, and task-oriented.
- Keep the main pages available in both English and Simplified Chinese.
- Keep code comments concise and reserve them for non-obvious control logic, hardware assumptions, or safety behavior.

## Change Requirements

- Add tests for hardware-related parsing or control logic changes whenever practical.
- Update example YAML and configuration docs when the config structure changes.
- Update `README.md` when CLI behavior or installation flow changes.

## Before Opening a PR

- The project builds successfully.
- Unit tests pass.
- `validate-config` succeeds with the example YAML.
- `install-service --dry-run` outputs the expected `systemd` unit.
