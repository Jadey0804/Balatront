# Balatront

课程 GamesEngineeringBase 工程，当前完成阶段 0 的静态占位场景。

## 构建和运行

1. 使用安装了 C++ 桌面开发工具和 Windows SDK 的 Visual Studio 2022 打开本目录 Balatront.sln。
2. 选择 x64、Debug 或 Release，生成解决方案。
3. 从 Visual Studio 启动，或在生成的 EXE 所在目录启动 EXE。生成会自动复制 Resources 子目录。
4. 场景显示提供的地图图片中心区域；控制台每两秒报告一次基线 FPS，按 Esc 退出。

目前没有玩家移动、战斗、图块解析或存档。Resources/tiles.txt 和 24 张小图块留给后续阶段。
旧 RiverRaid 类保留在 Balatront.cpp 中作为开发基础，当前入口不实例化它们。

GamesEngineeringBase.h 不准修改。阶段工作均使用新分支，未经明确授权不得 commit 或 push。
详细接口、约束、测试与已知限制见 PHASE0_AUDIT.md。
