# GitHub Actions

Simplified Chinese: [docs/zh-CN/ci/github-actions.md](../zh-CN/ci/github-actions.md)

## CI Workflow

File: `.github/workflows/ci.yml`

Triggers:

- push to `main` or `master`
- pull request targeting `main` or `master`

Steps:

1. Install the C++ build dependencies and `yaml-cpp`
2. Configure the project with CMake
3. Build the project
4. Run tests with CTest

## Release Workflow

File: `.github/workflows/release.yml`

Trigger:

- manual `workflow_dispatch`
- required input such as `v1.0.0`

Steps:

1. Build on Linux `x86_64` and Linux `arm64`
2. Run the test suite
3. Package the binary together with the `yaml-cpp` runtime library, plus the example YAML, service template, README, and license
4. Generate SHA256 checksums
5. Create a GitHub Release and upload the artifacts

## Maintenance Notes

- Update both workflows when build dependencies change.
- Add new runtime assets to the release packaging step when needed.
- Validate runner availability and dependency installation strategy before adding more architectures.
