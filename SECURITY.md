# Security Policy

## Reporting a Vulnerability

Please do not open a public issue for security-sensitive reports. Use the maintainer contact information associated with the repository and include as much detail as possible:

- affected version
- reproduction steps
- expected behavior versus actual behavior
- impact and severity
- a minimal reproducible example when available

## What Counts as a Security or Safety Issue

This project directly changes server fan speed through `ipmitool`, so the following classes of issues should be treated as security or operational safety problems:

- incorrect fan speed enforcement
- temperature parsing errors that can lead to unsafe control decisions
- YAML validation gaps that allow dangerous configurations
- service installation behavior that introduces privilege or path risks

When reporting one of these issues, include the hardware platform, BMC or iDRAC type, `ipmitool` version, and relevant logs if available.
