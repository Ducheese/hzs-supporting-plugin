> *IDA is all you need. <br />遇事不决，先逆向。* 

# HZS 大灾变增强插件集（hzs-supporting-plugin）

给 [HanZombieScenario 大灾变插件](https://github.com/H-AN/H-AN-CSS-ZombieScenario) 补充特性的 SourceMod 插件合集，包括：
* **★ 实现多种特殊僵尸技能 / BOSS 僵尸技能**
* **★ 提供若干源码级修复补丁，拦截引擎固有行为**
* **★ 提供配套的击杀信息流显示**
* 提供 BOT 复活方案
* 实现几种人类技能

本插件集大量使用基于 IDA 逆向而来的字节签名和偏移量信息，已在 CS:Source v91 (32位) / v92 (32位) / Steam 正版 (64位) 三种版本上验证过签名匹配的唯一性。单机游玩不成问题，服务器部署尚且未知，也许需要补全适用于 linux 的签名。

## 一、插件列表

带 ★ 的是推荐必装的插件，带 **[DEPRECATED]** 的是不再推荐使用的插件。

| .smx名称 | 简述 | 具体实现 | 依赖 gamedata |
| :--- | :--- | :--- | :--- |
| ★ hzs_zombieskill | 实现 NPC 僵尸技能和 BOSS 僵尸技能 | 已有 9 种特殊僵尸、3 种 BOSS 僵尸，具体见僵尸技能详解 | 是 |
| ★ hzs_botaddfix | 修复大灾变插件不支持中途添加 BOT 和复活 BOT 的限制 | 使用字节补丁跳过所有 `InitializeHostageInfo` 初始化，以此回避硬编码数组维度的 buffer overflow 问题 | 是 |
| ★ hzs_sameteam | 阻止不同阵营间 BOT 的相互敌对状态 | DHook `CBaseEntity::InSameTeam` 无条件返回 true，所有 BOT 和玩家间相互视为同阵营<br /> 与 [zr_stopbotsattacking_humans](https://forums.alliedmods.net/showthread.php?t=332041) 原理相同，签名也能相互印证，但简化掉了 ZR 相关的部分 | 是 |
| ★ hzs_killpenaltyfix | 去除伤害或击杀 NPC 僵尸（人质实体）导致的金钱和踢出惩罚 | 使用字节补丁跳过 `OnTakeDamage_Alive` 与 `Event_Killed` 中的 `AddAccount` 调用，以及 `Event_Killed::CheckForHostageAbuse` 中的踢出检查 | 是 |
| hzs_botfollowfix | 实现 BOT 永久跟随，为追击类大灾变地图提供 BOT 游玩基础 | 通过 SDKCall `CCSBot_Follow` 无视阵营强制所有 BOT 跟随玩家，DHook `CCSBot_StopFollowing` 防止 BOT 中途放弃跟随 | 是 |
| ★ hzs_killfeed | 显示适用于 NPC 僵尸的 HudText 击杀信息流 | 1. 准星下方显示击杀信息、连杀节奏滚动队列<br />2. 准星左侧显示自己的积分和排名 | 否 |
| **[DEPRECATED]**<br />hzs_botfakerespawn | 用伪死亡和伪复活来解决大灾变插件 BOT 不支持复活的限制 | 1. BOT 即将死亡时会被传送至地图坐标 (0, 0, 0)，待复活后再传送至任意 CT 复活点，并有无敌时间和防卡人机制<br />2. 复活后的 BOT 会补满血量并自动买甲<br />3. BOT 名称新增方括号前缀，显示其生命计数或复活倒计时 | 否 |
| hzs_bottruerespawn | 实现 BOT 真复活 | 本质是 hzs_botfakerespawn 在 hzs_botaddfix 解决崩溃前提下的重构版本，直接适配 BOT 真实死亡与复活事件钩子，然后延续生命计数、复活倒计时、随机传送、无敌时间、防卡人等设计<br />**需 hzs_botaddfix 作为前置插件** | 否 |
| hzs_infiniteammo | 简易无限弹药 | 重生时直接把所有种类备弹设成 9999 | 否 |
| hzs_knife | 人类被动技能，让近战武器具有范围伤害 | 挥刀时对面朝方向、一定范围内的所有僵尸都造成一定伤害值 | 否 |
| hzs_scream | 人类主动技能，可以发出尖叫吸引僵尸注意 | 新增命令 `sm_scream`，可强制一定范围内所有僵尸把自己作为攻击目标一段时间，有技能 CD | 否 |

> **注**：多数 32 位插件平台编译的插件是可以直接放到 64 位上用的，但涉及逆向的插件除外。也就是说，表格中所有依赖 gamedata 的插件，必须用 64 位插件平台的 spcomp64 重新编译才可以使用，否则进图必闪退。

## 二、僵尸技能详解

### ★ 特殊僵尸（9 种）

以下特殊僵尸的技能实现，和其视觉模型保持独立。就算全部使用奥巴马僵尸模型，技能也可以正常运作。

| 名称 | 技能 | 频次 |
| :--- | :--- | :--- |
| 迷雾僵尸 | 受伤后立刻在头顶产生大量黑烟（`env_smokestack`）<br />阻碍真人玩家视线，以及**充当氛围组** | 一次性 |
| 自爆僵尸 | 死亡后延时产生爆炸（`env_explosion`）<br />**参数若设置不当会变成对人类伤害 MVP** | 一次性 |
| 治疗僵尸 | 周期性为周围僵尸恢复固定血量 | 无限，有 CD |
| 幽灵僵尸 | 只能看见影子，死亡后才显形<br />对于真人玩家来说基本上就是无法察觉到的、会无意间击杀的存在 | 被动 |
| 恶魔僵尸 | 近距离会击飞玩家，远距离会用投掷物击落玩家武器<br />**该投掷物可以用刀 / 快速近战挡下** | 无限，分长短 CD |
| 屠夫僵尸 | 死亡后在原地生成鬼手陷阱，靠近的玩家会被控制住行动<br />**鬼手陷阱可以用枪提前破坏** | 一次性 |
| 女巫僵尸 | 接近玩家以造成视觉干扰（`color_correction`）或操作扰乱：<br />1. 短时间接触 → 纯灰度滤镜<br />2. 长时间接触 → 严重画面失真 + 切去声音高频 + 看不见任何人类或僵尸的模型 + 方向键映射旋转<br />**个体威胁性不大，但要远离扎堆的女巫僵尸** | 被动 |
| 伪人僵尸 | 随机选取一名玩家，使用其模型，模仿其动作<br />**细看破绽百出，非常喜感，但实战中容易被忽视，美美隐身** | 被动 |
| 神秘僵尸 | 在目标玩家脚下生成传送门，与处于传送门上的玩家互换位置<br />**有它就没有所谓的无敌点了** | 一次性 |

### ★ BOSS 僵尸（3 种）

**BOSS 僵尸的技能实现是和其视觉模型强绑定的**。

比如，异形斗兽的冲撞逻辑，完全根据其冲撞动作帧进行设计；安哥拉的砸地击飞时机、飞行扇翅膀频次，也和其动作帧直接相关；巨型狂暴形态僵尸举起僵尸或者人类过头顶的高度是硬编码的，也和其视觉模型缩放大小直接相关。

| 名称 | 技能 | 频次 |
| :--- | :--- | :--- |
| 巨型狂暴形态僵尸 | 1. 半血以上，举起一撮小僵尸并投掷到目标玩家附近<br />2. 半血以下，冲向目标玩家并尝试擒抱，被抓住的玩家会受到持续的擒抱伤害，难以挣脱 | 无限，分长短 CD |
| 异形斗兽 | 1. 向目标玩家发起冲撞，所经之处都要遭殃，位置不好会吃叠伤<br />2. 如果范围内有多个玩家，会施展震荡波，所有受害者要吃减速 + 缴械 + 扣护甲 | 无限，有 CD |
| 安哥拉（牢安） | 击飞当平A<br />痛了会起飞<br />刮风带毒雾<br />打架爱摇人<br />血少能自愈<br /> | 无限，有 CD |

> **注**：本插件的 `scripting\HZSZombieSkill\global.inc` 里的僵尸名称，与大灾变插件的 `configs\HanZombieScenario\HanZombieScenarioZombieData.cfg` 里的僵尸取名是硬编码对应的，改名需同步两处，且需重新编译替换 `hzs_zombieskill.smx`。

### hzs_zombieskill 中的逆向（`detours.inc` / `sdkcall.inc`）

| 简述 | 具体实现 |
| :--- | :--- |
| 实现 NPC 僵尸（人质实体）无视距离全图索敌 | 用字节补丁 NOP 掉 `CHostage::UpdateFollowing` 的直线距离（2000）与路径长度（4000）检查，再 DHook `CHostage::Idle` 拦截无聊打断。<br />**理论上，只要 nav 可达，僵尸就会追上来** |
| 实现运行时天空盒更换 | SDKCall 直调 `R_LoadNamedSkys` 重载指定天空盒贴图<br />多数天空盒 `"nofog"` 取 1，安哥拉的毒雾（`env_fog_controller`）无法遮蔽天空，因此需要这样一个自由更换天空盒的手段 |
| 获取武器弹匣容量 | SDKCall 直调 `GetMaxClip1` 获取真实弹匣上限，简洁优雅 |
| 避免 ColorCorrection 影响无关玩家 | DHook `CColorCorrection::UpdateTransmitState` 不再默认全员广播，让 SetTransmit 钩子生效，使女巫僵尸的灰度滤镜只对目标玩家生效，不误伤其他人 |
| 让 BOT 自动捡起最近的武器 | SDKCall 直调 `GetWeaponSlot` 读取无主武器所属槽位，直调 `CCSBot_Hide` 让 BOT 移动到武器坐标<br />更直接的移动函数被内联了，无法用 SDKCall 调用 |

## 三、可调参数总览

| ConVar | 默认值 | 说明 |
| :--- | :--- | :--- |
| sm_hzs_killfeed_time | "2.0" | 准星下方击杀信息流每行显示时长（秒） |
| sm_hzs_bottruerespawn_lives | "3" | BOT 生命总数 |
| sm_hzs_bottruerespawn_countdown | "15.0" | BOT 复活倒计时（秒） |
| sm_hzs_bottruerespawn_protect | "3.0" | BOT 复活无敌时间（秒） |
| sm_hzs_knife_damage | "100" | 近战武器范围伤害值 |
| sm_hzs_knife_range | "2.0" | 近战武器有效攻击范围（单位：米） |
| sm_hzs_scream_cooling | "60.0" | 尖叫技能冷却时间（秒） |
| sm_hzs_scream_range | "30.0" | 尖叫技能的有效范围（单位：米） |
| sm_hzs_scream_duration | "15.0" | 尖叫技能吸引僵尸的持续时间（秒） |
| sm_hzs_scream_filepath | "player/waoh.wav" | 尖叫音频路径（不带 sound/） |

hzs_zombieskill 的可调参数过多（各技能数值、音效路径、僵尸名称等全部以 `#define` 集中在 `HZSZombieSkill/global.inc`），因此暂不注册 ConVar。如要调整技能数值，和僵尸名称一样，编辑该文件后重新编译替换 `hzs_zombieskill.smx` 即可。

## 四、配套资源（custom/）

| 文件夹 | 内容 |
| :--- | :--- |
| 插件配套 - hzs_scream | 尖叫音效 4 个（ciallo / mambo / waoh / zaako-zaako） |
| 插件配套 - hzs_zombieskill | 模型、材质、音效、粒子、CC 滤镜、天空盒，见下表 |

| 类型 | 内容 |
| :--- | :--- |
| 模型（models/） | 灵魂火 soulfire<br />鬼手陷阱 zombitrap<br />爆弹兽颅 w_eq_fraggrenade<br />hl2 天空大漩涡 combine_citadelcloudcenter |
| 材质（materials/） | models/ 上述模型对应贴图<br />particle/ 僵尸技能粒子效果所用贴图<br />skybox/ 自制天空盒 sky_green（`"nofog"` 取 0） |
| 音效（sound/） | custom/ 30 个僵尸技能相关音效<br />weapons/angela/ 4 个 csol 安哥拉音效 |
| 粒子（particles/） | z_zombie_effects.pcf 僵尸技能粒子效果 |
| CC 滤镜（resource/） | angela_L_g1p0_r55_g100_b35.raw（毒雾绿色滤镜）<br />witch_L_g1p0.raw（纯灰度滤镜）<br />witch_L_g2p2_p8.raw（严重画面失真滤镜） |

## 五、借物表（Credits）

- csol 资产解包 - H-AN
- 绝大部分僵尸技能音效 - VALVE (Left 4 Dead 2)
- 僵尸技能粒子效果 - Zenlenafelex (https://steamcommunity.com/sharedfiles/filedetails/?id=2119972050)
- Soul Fire 模型 - Borf Chavez (https://sketchfab.com/3d-models/soul-fire-7c802dd0f69d402299dde6127b89de53)
- 毒雾咳嗽音效 - STALKER G.A.M.M.A. Radiation Effects Overhaul
- 震荡波电流音效 - STALKER G.A.M.M.A. Shrike's Dark Signal Blowout and Anomalies Audio
- 投掷物风阻声 - Sofibyte (https://steamcommunity.com/sharedfiles/filedetails/?id=167809847)
- 格挡音效 - Elden Ring (https://gamebanana.com/sounds/82330)
