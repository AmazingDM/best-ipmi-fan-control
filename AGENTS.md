# Repository Guidelines

## Project Structure & Module Organization

Core C++ code lives in `src/`, with public headers in `include/ipmi_fan_control/`. The main executable entry point is `src/main.cpp`, while CLI parsing, config loading, IPMI access, control rules, logging, and service installation are split into focused translation units. Tests live in `tests/test_main.cpp` and use a small in-repo test harness rather than an external framework. Example config files are under `examples/`, systemd unit templates are in `etc/systemd/system/`, and longer design or CI notes live in `docs/` and `docs/zh-CN/`.

## Build, Test, and Development Commands

Use CMake with Ninja for local work:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/ipmi-fan-control validate-config --config examples/config.example.ini
./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.ini --dry-run
```

The first command configures the project, the second builds the library, binary, and tests, and `ctest` runs the unit suite. The last two commands are the expected smoke checks before opening a PR.

## Coding Style & Naming Conventions

Follow `.editorconfig`: spaces, LF line endings, UTF-8. Markdown keeps trailing whitespace; source files should not. C++ code in this repo currently uses the existing 4-space indentation style, `snake_case` for files and functions, and `PascalCase` for helper/test function names such as `TestParseMaxTemperature`. Keep headers narrow, comments brief, and prefer readable control flow over compact tricks. There is no configured formatter; rely on compiler warnings (`-Wall -Wextra -Wpedantic -Wconversion`) and match nearby code.

## Testing Guidelines

Add or extend tests in `tests/test_main.cpp` for parsing, config validation, control rules, and service-generation changes. Name new tests as `Test...` helpers and register them in the `tests` vector in `main()`. Hardware-facing behavior should be covered with deterministic sample output strings whenever possible.

## Commit & Pull Request Guidelines

Recent history follows short, imperative commits with prefixes like `feat:`, `fix:`, and `chore:`. Keep commits focused and describe the user-visible change or safety fix. PRs should summarize behavior changes, mention config or documentation updates, note the checks you ran, and add a dated `CHANGELOG.md` entry for every substantive change using the `YYYY-MM-DD` + `Changed` / `Fixed` / `Breaking Changes` structure. If CLI output, INI schema, or installation flow changes, update `README.md`, docs, examples, and the changelog in the same PR.
