# 贡献指南

感谢关注这个项目。为了降低硬件相关改动带来的风险，提交前请优先保证可读性、可测试性和回归验证。

## 本地开发

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 提交要求

- 所有新增文档使用中文
- 所有新增代码注释使用中英双语
- 与硬件控制相关的逻辑必须补充测试或至少补充解析/匹配层测试
- 如果改动了 YAML 结构，必须同步更新示例配置与说明文档
- 如果改动了 CLI 行为，必须同步更新 `README.md`

## 提交前检查

- 代码可编译
- 单元测试通过
- `validate-config` 能正确校验示例配置
- `install-service --dry-run` 输出的服务文件符合预期
