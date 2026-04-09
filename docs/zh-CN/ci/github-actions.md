# GitHub Actions 说明

English: [docs/ci/github-actions.md](../../ci/github-actions.md)

## CI 工作流

文件：`.github/workflows/ci.yml`

触发条件：

- 推送到 `main` 或 `master`
- 向 `main` 或 `master` 发起 Pull Request

执行步骤：

1. 安装 C++ 构建依赖与 `yaml-cpp`
2. 使用 CMake 配置项目
3. 编译项目
4. 使用 CTest 执行测试

## Release 工作流

文件：`.github/workflows/release.yml`

触发方式：

- 手动执行 `workflow_dispatch`
- 输入版本号，例如 `v1.0.0`

执行步骤：

1. 分别在 Linux `x86_64` 和 Linux `arm64` 上构建
2. 运行测试
3. 打包二进制及其 `yaml-cpp` 运行库，同时附带示例 YAML、服务模板、README 和许可证
4. 生成 SHA256 校验文件
5. 创建 GitHub Release 并上传产物

## 维护建议

- 构建依赖变化时，要同步更新两个工作流。
- 新增运行时资源文件时，要同步加入 Release 打包步骤。
- 增加新架构前，先确认 GitHub runner 可用性与依赖安装方案。
