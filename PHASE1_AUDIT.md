# 阶段 1 移动、状态与摄像机

阶段 0 已本地提交为 1d0a9b1。阶段 1 分支为 phase-1-player-camera。
2026-09-07 用户确认验收完成并授权本地提交；未授权推送。
启动前已有的 x64/Debug 构建产物改动原样保留，不纳入阶段 0 提交。

## 实现范围

- Balatront.cpp 的 main 只创建 Game 并调用 run；原教程类保留。
- Game.h/cpp 管理窗口、资源、输入、更新与绘制；没有新框架或外部 API。
- Gameplay.h：Vector2、Player、Camera、PlaySession，均为最小纯数据/逻辑结构。
- Player 以世界中心坐标存储，180 px/s、100 HP、14 px 圆形碰撞范围。L.png 按比例缩放至 48 px 高。
- 固定镜头跟随并限制在 1344×1344 地图内。Infinite 模式只提供居中接口和逻辑测试，未接入无限地图。
- Menu → Playing；Esc 在 Playing/Paused 间切换，Enter 开始/恢复，暂停时 Q 退出。
- 模拟只在 Playing 更新，暂停清零速度并冻结位置、镜头、计时；FPS 继续刷新。
- Hud.h 用自定义固定 5×7 字模绘制基本英文 HUD，没有调用系统字体 API。
- 新增的 Resources/L.png 与用户原资源一致，自动随生成复制。

## 验证

- x64 Debug/Release 均成功构建。原教程 collision2 的 C4018 警告仍保留，新增代码无编译警告。
- tests/Phase1Tests.cpp 通过：直线/斜线均 180 px/s；30/120 次每秒更新位移一致；玩家四边约束；镜头两角约束；无限镜头接口保持居中；菜单不更新；暂停 600 次更新位置/计时/镜头不变；恢复不补算暂停时间。
- Release 实际桌面截图验证菜单、L.png、HUD、按 D 的位置变化、暂停/恢复。暂停前后日志连续保持 time 117.701 和 position 672.298,672，恢复后计时继续。
- 构建和运行日志位于外层 phase1-artifacts；截图在本任务工具记录中。键盘持续长按/完整四边场景可按 README 手动复查；边界和帧率独立性由独立逻辑测试验证。
- 游戏类包含旧框架头但未修改它；SHA256 应保持 143F76A049B5C7DCDFAD4B2269ADE45238D83FCE33690B3B0D32C4F70DD2A10C。

在 VS x64 Native Tools Command Prompt，从仓库目录执行独立测试：

```text
cl /nologo /EHsc /std:c++14 /Fe:%TEMP%\Phase1Tests.exe /Fo:%TEMP%\Phase1Tests.obj tests\Phase1Tests.cpp
%TEMP%\Phase1Tests.exe
```

## 明确限制

- 用户已选择阶段 1 只做 WASD，左摇杆待补。课程框架的摇杆数值是私有成员，没有 getter；不绕过访问限制。
- 中文输入法会拦截字母键；使用英文输入。框架也没有焦点丢失时清键的公开接口，按住移动键切出窗口存在粘键风险。
- HP/碰撞范围是阶段 1 数据，没有伤害或地形响应；图片中的水域此时可穿过。
- 地图是整图裁剪，尚未解析 tiles.txt，不宣称完成阶段 5。
- Shop/GameOver 仍是枚举占位，阶段 1 不实现商店或死亡流程。
- 实际键盘和 UI 验证使用 Release；Debug 已编译，逻辑测试独立于图形框架。
