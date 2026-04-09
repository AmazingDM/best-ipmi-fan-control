# Changelog

Policy: every substantive change must be recorded under a new `## YYYY-MM-DD` entry and grouped under `### Changed`, `### Fixed`, and/or `### Breaking Changes` as applicable.

## 2026-04-09

### Changed

- Updated release packaging to publish one executable asset per Linux architecture, with SHA256 checksums and no bundled `lib/` directory or `yaml-cpp` runtime.
- Migrated user-facing docs, examples, and `systemd` unit templates from YAML references to INI references, including `config.ini` paths.
- Clarified contributor guidance so substantive changes must add a dated changelog entry using the existing section structure.

## 2026-04-10

### Changed

- Reworked the main documentation set into English-first pages with synchronized Simplified Chinese mirrors.
- Rewrote contributor, security, architecture, CI, configuration, and migration docs to be reader-facing and task-oriented.
- Updated the example `systemd` unit templates to match the current escaped `ExecStart=` format.

### Fixed

- Made `auto` mode fail safe: on sensor read or IPMI write failure, the service now attempts to switch fans to `100%` and exits non-zero so `systemd` can restart it.
- Classified CLI usage failures separately from runtime failures to avoid printing usage text for operational faults.
- Hardened `install-service` by validating `--service-name`, blocking path traversal, escaping `ExecStart=` arguments, and rolling back unit file and service state on failure.
- Replaced POSIX shell-based command execution with `fork/execvp` plus pipe capture to remove shell parsing from the execution path.
- Expanded tests to cover strict YAML validation, escaped `systemd` units, and invalid service names.

### Breaking Changes

- YAML files loaded with `--config` are now validated against a strict schema:
  - `interval_seconds`, `full_speed_threshold`, and `steps` are required.
  - Unknown top-level fields are rejected.
  - Unknown fields inside `steps` entries are rejected.
- Built-in defaults are now only used when `auto` runs without `--config`.
