# 文档索引

English: [README.md](../../README.md)

## 概览

这里汇总 `ipmi-fan-control` 的简体中文文档，方便中文读者查阅；如需英文版，可从各页面跳转到对应英文文档。

## 页面

- [架构说明](architecture/control-flow.md)
- [INI 配置说明](configuration/ini-schema.md)
- [GitHub Actions 说明](ci/github-actions.md)
- [C++ 迁移总结](plans/cpp-migration-plan.md)

## 服务常用命令

```bash
sudo ./build/ipmi-fan-control install-service --config /etc/ipmi-fan-control/config.ini
sudo ./build/ipmi-fan-control uninstall-service
sudo systemctl start ipmi-fan-control
sudo systemctl stop ipmi-fan-control
sudo systemctl restart ipmi-fan-control
sudo systemctl reload ipmi-fan-control
systemctl status ipmi-fan-control
```

修改 `/etc/ipmi-fan-control/config.ini` 后，可执行 `sudo systemctl reload ipmi-fan-control` 热重载配置；如果修改的是 `.service` 单元文件本身，则先执行 `sudo systemctl daemon-reload`，再重启或重载服务。
`uninstall-service` 只卸载 systemd 服务自身，不会自动删除配置文件。
