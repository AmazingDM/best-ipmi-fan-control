# GitHub Actions 说明

English: [docs/ci/github-actions.md](../../ci/github-actions.md)

## CI 工作流

文件：`.github/workflows/ci.yml`

触发条件：

- 推送到 `main` 或 `master`
- 向 `main` 或 `master` 发起 Pull Request

执行步骤：

1. 安装 C++ 构建依赖
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
3. 使用 Ubuntu 22.04 runner 构建 Release，避免继承 Ubuntu 24.04 较新的 glibc 依赖
4. 上传前对每个架构的最终可执行文件执行 strip
5. 为每个架构发布单个可执行文件产物，不再附带 `lib/` 目录或 `yaml-cpp` 运行库
6. 生成 SHA256 校验文件
7. 创建 GitHub Release 并上传产物

## 可移植性说明

- Release 产物改为在 Ubuntu 22.04 上构建，避免引入 `GLIBC_2.38` 这类在较老 Linux 系统上不存在的依赖。
- Release 上传前会先 strip，因此最终产物大小会更接近日常本机构建后手动分发的可执行文件。

## 维护建议

- 构建依赖变化时，要同步更新两个工作流。
- Release 打包方式调整时，要同步维护产物命名和 SHA256 校验生成逻辑。
- 增加新架构前，先确认 GitHub runner 可用性与依赖安装方案。
