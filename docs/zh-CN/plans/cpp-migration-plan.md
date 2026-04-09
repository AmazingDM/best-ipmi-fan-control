# C++ 迁移总结

English: [docs/plans/cpp-migration-plan.md](../../plans/cpp-migration-plan.md)

## 范围

将原始的 Rust 版 IPMI 风扇控制工具迁移为 C++ 版本，引入 YAML 阶梯控速，增加 `systemd` 服务安装能力，并用面向 C++ 的 GitHub Actions 替换旧的 Rust 自动化流程。

## 当前状态

1. 使用 `CMake + yaml-cpp` 重建工程
2. 用 YAML 配置替代硬编码的温度映射
3. 保留 `info / fixed / auto`，并新增 `validate-config / install-service`
4. 增加测试、示例资源、服务模板、CI 和 Release 自动化

## 当前状态补充

- 代码已经按 CLI、配置、控速、IPMI、进程、日志和服务安装等职责拆分。
- 示例 YAML 可以被校验并用于自动控速。
- `install-service` 可以为 Linux 部署生成或安装 `systemd` 服务。
- CI 会执行构建和测试，Release 工作流会打包 Linux 二进制产物。
