# 阶段 0 工程审计

日期：2026-09-06。分支：`phase-0-project-baseline`。

## 授权和范围

- 在当前 Balatront.cpp 的课程示例基础上逐阶段开发。
- GamesEngineeringBase.h 不准修改。允许引用；其内部 STL 和平台 API 不计入学生新增代码禁令。
- 不增加第三方库或外部 API；不 commit、不 push。
- 本阶段只建立可编译、可启动的工程基线，不实现阶段 1 的玩家和摄像机。

## 仓库结构

实际 Git 根目录是本文件所在目录，外层 Docs 和外层解决方案不在此 Git 仓库内。
当前仅有一个编译入口 Balatront.cpp，内容已是 FileIO 版本。
仓库内 Balatront.sln 原先错误地多引用一层 Balatront 目录，本阶段修正为同级 vcxproj。
已有 x64/Debug 构建中间文件被 Git 跟踪；本次验证使用仓库外的 phase0-artifacts 目录，避免改写它们。

## 已核实的框架接口

| 类型 | 接口和使用注意 |
| --- | --- |
| Window | create(width,height,title)，checkInput()，clear()，draw(...)，present() |
| Window | getWidth()/getHeight()，keyPressed(VK_*) 返回按住状态，不是单次按下事件 |
| Window | getBackBuffer() 返回 RGB 像素缓冲区；使用方必须保证坐标范围 |
| Timer | dt() 返回秒，并在调用时重置；每帧只读取一次再分发 |
| Image | load(filename)，width/height/channels/data，at()/atUnchecked()，alphaAtUnchecked()，free() |
| Image | 不可复制，可移动；析构释放像素。load 对缺失文件的处理不安全，不可盲目加载不存在的路径 |
| XBoxControllers | hasController()，getFirstPlayerController()，probeControllers() |
| XBoxController | 更正（阶段 1 审计）：update()、按钮查询、getID()；摇杆数值私有且没有 getter，不能直接读取 |
| SoundManager | load()/play()、loadMusic()/playMusic()；本阶段不接入 |

没有找到可直接复用的 Vector2、CollisionShape、虚拟摄像机或文字 HUD 接口。
后续实现前确认最小自定义方案，不照抄设计文档中的伪接口。

Window::present 使用 Present(0,0)，没有开启垂直同步；FPS 只能作为该场景的本机基线。
窗口关闭按钮由框架调用 exit(0)；Esc 则可以从游戏循环正常返回。框架限制只记录、不修改。

## 容量和诊断约定

GameConfig.h 声明窗口尺寸、敌人 256、玩家弹 512、敌方弹 256、地图最大 80×60、图块 32、卡牌 8、持有槽位 16。
AOE 存储上限暂与敌人容量一致，具体玩法上限在阶段 4 确认。
GameState 只声明 Menu/Playing/Paused/Shop/GameOver，不表示状态功能已经实现。
GAME_ASSERT 用于开发期内部不变量，GAME_DEBUG_LOG 仅 Debug 输出。
外部输入错误应报告并返回失败；对象池满属于正常失败，Release 也必须检查，不能只依赖断言。
当前教程仍使用标准字符串、流和 min/max；按约束不得新增 STL 容器或算法依赖。

## 占位资源与运行基线

用户提供外层 Resources，并授权先用作占位。26 个文件原样复制到仓库 Resources，逐个哈希一致，原目录未改动。
包含 24 张 32×32 RGB 图块、1344×1344 RGBA landscape.png、42×42 的 tiles.txt。不是 RiverRaid 的飞机/遮罩/排列文件。
本阶段 main 使用 landscape.png 的中心 1024×768 裁剪作为静态背景，未实现摄像机或图块解析；原教程类保留但不实例化。
构建后自动复制 PNG/TXT 至输出目录的 Resources，VS 调试工作目录设为输出目录。
加载前检查文件存在，避免框架对缺失文件崩溃。损坏图片的完整解码校验尚不支持。

保留的教程问题：地图头部/编号未验证，失败后仍可访问空地图；碰撞采样存在边界误差；位移按帧计算；C4018 有符号比较警告。
这些尚未修复，不作为最终游戏已通过的能力。后续地图/移动阶段应替换或修正对应逻辑。

## 验收状态

- 本次改动后的 x64 Debug、Release 构建已通过，保留教程 C4018 警告。日志在外层 phase0-artifacts/Debug-build.log 和 Release-build.log。
- 自有代码未发现禁用容器，未新增外部库/API。
- Debug/Release 均在实际用户桌面显示占位场景，已通过 computer-use 截图目视检查。两次 Esc 后对应进程均退出；截图保留在本次任务工具记录中。
- 实际桌面日志：外层 phase0-artifacts/debug-desktop.log、release-desktop.log；对应 errors 日志为空。
- 观测片段 Debug 约233–239 FPS（4.18–4.29 ms/帧），Release 约940–950 FPS（1.05–1.06 ms/帧）。无同步、静态背景、无玩法，不是完整游戏或压力测试性能。
- 初次沙箱运行不在用户桌面，已停止；其 release-run.log 不作为正式画面验收证据。
- GamesEngineeringBase.h SHA256：143F76A049B5C7DCDFAD4B2269ADE45238D83FCE33690B3B0D32C4F70DD2A10C。
- 阶段 0 基线验收通过。未实现阶段 1，不自动进入下一阶段。未 commit 或 push。
