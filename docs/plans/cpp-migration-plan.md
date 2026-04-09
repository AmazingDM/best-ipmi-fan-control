# C++ Migration Summary

Simplified Chinese: [docs/zh-CN/plans/cpp-migration-plan.md](../zh-CN/plans/cpp-migration-plan.md)

## Scope

Migrate the original Rust-based IPMI fan control tool to C++, introduce YAML step-based fan control, add `systemd` service installation support, and replace the old Rust automation with C++-focused GitHub Actions.

## What Changed

1. Rebuilt the project with `CMake + yaml-cpp`
2. Replaced hard-coded temperature mapping with YAML configuration
3. Kept `info / fixed / auto` and added `validate-config / install-service`
4. Added tests, example assets, service templates, CI, and release automation

## Current Status

- The codebase is split into CLI, configuration, control, IPMI, process, logging, and service modules.
- The example YAML can be validated and used for automatic control.
- `install-service` can generate or install a `systemd` unit for Linux deployments.
- CI builds and tests the project, and the release workflow packages Linux binaries.
