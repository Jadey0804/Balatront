# 敌人美术配置接入

## 启动错误修复

用户运行时报告Frame dimensions不匹配，核对后实际是5张PNG（turretBody、turretPipe、brute walk/hurt/death）使用索引色，课程Image::load不支持该编码。已将仓库资源转换为8位RGBA PNG，逐像素核对包含透明度的ARGB值全部相同，尺寸不变。外层下载原文件保持原样，勿用其索引色副本覆盖转换后的工程资源。
原先加载失败与尺寸不符共用报错，现已拆开，并报告类型/动作、控制台路径及尺寸信息。Debug/Release构建通过；5张修复资源已同步到现有x64/Debug/Resources和本次sprite-artifacts输出目录。框架不改，未提交或推送。尚需用户重启游戏确认完整运行。

分支phase-6-sprite-assets，保留未提交的自动攻击改动，本次未提交/推送。

## 配置与资源

实际工程源配置：Resources/Sprites/sprites.txt，运行读取EXE旁同名文件。外层用户Resources中的12张引用PNG已复制到仓库Resources，保留子目录和原图；附带已找到的Kenney许可证。没有复制无关资产包或aseprite文件。
后续请修改仓库内配置和资源，再生成并重启游戏；外层副本不会自动同步。构建已递归复制PNG/TXT及原有目录结构。

列顺序：Type Action "ImagePath" FrameW FrameH Frames FPS Scale Loop。路径相对于Resources，空格路径必须加引号。动作表映射goblin->Grunt、sprinter->Sprinter、brute->Brute；turretBody/turretPipe是同一Turret的两个部件。
单行横向动画：尺寸必须等于FrameW*Frames乘FrameH；静态图1帧/FPS0。loop=1循环，0停在末帧。静态hurt显示0.15秒。比例原样使用配置，包含图片透明留白，图像锚点为帧中心。
EnemySprites.h使用固定24槽的共享动画资源，各敌人只保存动画状态/时间，不逐敌人加载图片。未知类型/动作、重复动作、缺失移动或炮台idle、格式/尺寸错误在启动时报告。路径不存在先拦截；课程解码器对任意损坏PNG的容错未扩展。

## 行为

- 移动使用walk/fly，左右朝向通过水平翻转显示。图片原色保留，不再套用旧占位染色；F1仍显示原碰撞体和HP，碰撞尺寸未随图片改变。
- 有attack配置的近战敌人接触玩家范围时播放attack，动画期间仍按原逻辑追踪/接触扣血。未添加动作命中帧、蓄力或动作结束伤害。
- hurt优先覆盖attack；未配置hurt的goblin沿用白色受击闪光。没有brute attack素材，不人为补动画。
- 死亡立即active=false，不再锁定/碰撞/射击，但dying保留槽位直到death播放完成。只在槽位完全释放后复用，避免死亡画面变成新敌人。配置中炮台没有death，所以立即消失。
- 炮台本体静止，炮管按目标方向实时旋转。当前素材原朝向向下，围绕16×16帧中心旋转并与本体中心对齐。没有改炮弹发射位置，仍从敌人中心生成。
- 暂停冻结动画。GameOver冻结世界，包括尚未结束的死亡动画；重开重置敌人及动画状态。
- 玩家、炮弹、UI均无配置条目，保持现有外观；此读取器当前仅支持本次五组敌人资源，后续新增资源类型需扩展映射。

## 编译与手动验收

Debug/Release x64各构建一次成功，只有旧教程Balatront.cpp:180的C4018警告。输出在外层sprite-artifacts；Release资源子目录中已复制12张引用PNG、配置和Kenney许可证。GamesEngineeringBase.h哈希未变，无新增外部API、第三方库或STL容器。未新增/运行测试或启动游戏。

1. 重新生成并启动，确认三类移动敌人分别显示Orc、Bat、Fire golem，移动动画播放，左右朝向切换。
2. 接触goblin/sprinter观察攻击动画；射击brute/sprinter观察受击动作。动画仅是现有接触战斗的反馈，不保证伤害与某一帧同步。
3. 击杀敌人，确认立即停止造成伤害，死亡画面播完消失；自动攻击不再选取尸体。
4. 30秒后寻找炮台，绕其移动检查炮管方向与红弹方向一致，本体不转。确认安装轴心是否符合美术预期。
5. Esc暂停，动画停止，恢复继续；死亡重开没有旧尸体。不同图片透明留白不同，请按画面调整Scale，尤其goblin的100×100帧内角色本身较小。
6. 修改仓库sprites.txt中的缩放或FPS，重新生成并彻底重启游戏，确认变化；单帧炮台不会播放动画。

炮管轴心与各精灵视觉大小仍需要用户画面验收，编译通过不等于美术效果通过。
