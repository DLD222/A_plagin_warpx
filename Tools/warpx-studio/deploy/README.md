# 部署方式

三种都能用，选一种：

1. **启动器（推荐，WSL 默认可用）**

   ```bash
   warpx-studio          # 已在 PATH：~/.local/bin/warpx-studio
   ```

   自动起服务、等端口就绪、用 Windows 浏览器打开；服务用 `setsid` 脱离终端，关掉终端也不停。

2. **systemd user unit（需要 WSL 启用了 systemd）**

   ```bash
   mkdir -p ~/.config/systemd/user
   cp deploy/warpx-studio.service ~/.config/systemd/user/
   systemctl --user daemon-reload && systemctl --user enable --now warpx-studio
   ```

3. **桌面/开始菜单**：`~/.local/share/applications/warpx-studio.desktop`（WSLg 会把 Linux 桌面项
   带进 Windows 开始菜单）。

服务重启不会影响正在跑的仿真：warpx 以独立进程组运行、日志直接写文件，
新服务启动时会从 `audit_runs/studio/*/studio_job.json` 重新 attach 并回放进度。
