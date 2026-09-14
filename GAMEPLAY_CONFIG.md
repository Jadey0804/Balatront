# 基础属性配置

统一文件：仓库内 Resources/gameplay.txt。运行读取EXE旁的Resources/gameplay.txt。
每行格式为 `属性名 数值`，支持空行和井号注释，顺序自由。全部40项必须提供一次，缺失、重复、未知名称、非有限数值或越界会在启动时报错。

## 字段

| 前缀 | 配置内容 |
|---|---|
| player | health血量上限/初始值，speed基础移速，radius碰撞半径，invulnerability受伤无敌秒数，attack_interval攻击间隔，road_multiplier道路倍率 |
| goblin / sprinter / brute / turret | health初始血量，speed移速，contact_damage接触伤害，radius碰撞半径 |
| turret.attack_interval | 炮台发射间隔 |
| player_projectile / enemy_projectile | damage伤害，speed飞行速度，lifetime寿命，radius碰撞半径 |
| aoe | damage单目标伤害，cooldown冷却秒数，initial_targets初始目标数，max_targets玩法目标上限 |
| upgrade | kills_per_drop每多少击杀掉落，attack_interval_multiplier攻速强化的间隔倍率，min_attack_interval最小间隔，aoe_targets_added每次目标数增加，pickup_radius拾取距离附加半径 |

速度单位为像素/秒，半径单位为像素，时间单位为秒。美术缩放、帧数和动画FPS仍在Sprites/sprites.txt，不受本配置影响。

## 调参

- 修改仓库源文件后重新生成，资源自动复制，彻底重启游戏。
- 也可直接修改运行目录文件再重启，不需要重新编译；后续生成可能覆盖运行副本，正式修改应保存到仓库源文件。
- Enter重开使用启动时读入的基础值，不保留上一局强化；不实现热更新。
- max_targets最高为固定存储容量256，initial_targets不得超过它。min_attack_interval不得超过初始攻击间隔；攻速倍率范围0.01至1。
- 玩家radius范围1至16，兼容当前32像素格中心出生点搜索；如需更大碰撞体，应同时调整出生点算法。其他半径、速度、生命值和寿命也有输入范围检查，具体范围见GameplaySettings.h字段表。范围限制不是备用基础值。
- contact_damage=0不会触发玩家无敌时间。炮台speed默认0；改成正数会沿用追踪移动逻辑。

## 实现和验证

GameplaySettings.h只在启动加载一次，完整校验后发布配置，缺项不使用硬编码回退。删除Player初始属性常量、ProjectileConfig中的伤害/速度/间隔/寿命/半径常量、CombatConfig的AOE/强化基础常量以及GameConfig的道路倍率常量。敌人外观调试数据保留，与载入的战斗属性组合；HUD不再写死HP35等数值。
对象池数组容量、地图生成规则、敌人生成频率、动画反馈时长等非本次基础属性保持代码配置。旧教程和GamesEngineeringBase.h未改。

分支phase-6-gameplay-config，保留此前未提交的资产接入和自动攻击改动，未提交或推送。
Debug/Release编译通过，仅旧教程C4018警告。额外运行一次真实配置读取检查，输出玩家HP100、速度180、道路270、炮弹伤害20、敌人HP35/18/120/65、AOE3/256，与原基础值一致。没有批量新增测试或运行游戏验收。
手动重点：分别修改玩家血量、某敌人HP、炮弹伤害、道路倍率、AOE数量和强化参数后重启，检查HUD及战斗变化；重开应恢复修改后的初始值。若删掉属性或写入非法数值，启动应报告对应错误。
