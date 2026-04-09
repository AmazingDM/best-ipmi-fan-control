# C++ 迁移计划

## 目标

将原先基于 Rust 的 IPMI 风扇控制工具迁移为 C++ 版本，并引入 YAML 阶梯控速配置、`systemd` 服务安装能力以及 GitHub Actions 自动化流水线。

## 实施内容

1. 使用 `CMake + yaml-cpp` 重建工程结构
2. 将原先硬编码的温度映射迁移为 YAML 配置
3. 保留 `info / fixed / auto` 命令，并新增 `validate-config / install-service`
4. 采用中英双语注释重写核心逻辑
5. 新增单元测试、示例配置、服务模板和 GitHub Actions

## 验收标准

- 代码结构清晰，核心职责分层明确
- 示例 YAML 能被校验并驱动自动控速
- `install-service` 能生成或安装可用的 `systemd` 服务
- CI 可完成构建与测试
- Release 工作流可生成 Linux 二进制发布包
