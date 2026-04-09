# GitHub Actions 说明

## CI 工作流

文件：`.github/workflows/ci.yml`

触发条件：

- 推送到 `main` 或 `master`
- 向 `main` 或 `master` 发起 Pull Request

执行内容：

1. 安装 C++ 构建依赖与 `yaml-cpp`
2. 运行 CMake 配置
3. 编译项目
4. 执行 CTest

## Release 工作流

文件：`.github/workflows/release.yml`

触发方式：

- 手动执行 `workflow_dispatch`
- 输入版本号，例如 `v1.0.0`

执行内容：

1. 分别在 `Linux x86_64` 和 `Linux arm64` runner 上构建
2. 运行测试
3. 打包可执行文件、示例 YAML 与服务模板
4. 生成 SHA256 校验文件
5. 创建 GitHub Release 并上传产物

## 维护建议

- 如果修改了构建依赖，请同步更新两个工作流中的安装步骤
- 如果新增运行时资源文件，请同步加入 Release 打包列表
- 如果后续支持更多架构，应优先验证 GitHub runner 可用性和依赖安装策略
