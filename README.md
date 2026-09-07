# Balatront

课程 GamesEngineeringBase 工程，阶段 0 已提交，阶段 1 实现键盘移动、固定镜头和暂停。

## 构建和运行

1. 使用安装了 C++ 桌面开发工具和 Windows SDK 的 Visual Studio 2022 打开本目录 Balatront.sln。
2. 选择 x64、Debug 或 Release，生成解决方案。
3. 从 Visual Studio 启动，或在生成的 EXE 所在目录启动 EXE。生成会自动复制 Resources 子目录。
4. 使用英文输入模式，Enter 开始，WASD 移动，Esc 暂停/恢复，暂停时 Q 退出，F1 显示碰撞范围。
5. HUD 显示 HP、位置、时间、FPS 和状态。中文输入法可能拦截 WASD，请先切换英文模式。

目前没有战斗、地形碰撞、图块解析或存档。Resources/tiles.txt 和 24 张小图块留给后续阶段。
玩家使用 L.png 占位，碰撞半径 14，速度 180 px/s。地图使用 1344×1344 背景，窗口逻辑分辨率 1024×768。
左摇杆按用户决定待补：框架没有公开摇杆数值接口，且不允许修改框架。
旧 RiverRaid 类保留在 Balatront.cpp 中作为开发基础，当前入口不实例化它们。

GamesEngineeringBase.h 不准修改。阶段工作均使用新分支，未经明确授权不得 commit 或 push。
详细接口、约束、测试与已知限制见 PHASE0_AUDIT.md 和 PHASE1_AUDIT.md。
