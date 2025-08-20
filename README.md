### 一、插件列表

| .smx名称 | 简述 | 具体特性 |
| :--- | :--- | :--- |
|   hzs_botfakerespawn   |   用伪死亡和伪复活来解决H-AN大灾变模式插件BOT不能复活的问题   |   1. BOT即将死亡时会被传送至坐标0 0 0点，等待复活传送至任意CT复活点，复活后有无敌时间和防卡人机制<br />2. BOT名称新增生命总数和复活倒计时前缀<br />3. 复活后的BOT会补满血量并买甲   |
|   hzs_knife   |   人类被动技能，让近战武器具有范围伤害   |   -   |
|   hzs_scream   |   人类主动技能，可以发出尖叫吸引全场僵尸一段时间   |   -   |
|   hzs_zombieskill   |   实现NPC僵尸技能和BOSS技能   |   1. 迷雾僵尸受击后会产生黑烟，一次性技能<br />2. 自爆僵尸死亡后会爆炸，一次性技能<br />3. 治疗僵尸会不断为周围僵尸恢复固定血量，无限次数技能<br />4. 恶魔猎手可以击飞玩家，无限次数技能<br />5. 憎恶屠夫可以控制住玩家的移动，一次性技能<br />6. 嗜血女巫可以干扰玩家的方向键，一次性技能<br />7. 幽灵僵尸几乎透明，死亡后才显形<br />8. 安哥拉有两种群体击飞动作，可以呼唤僵尸集中攻击一个玩家，半血以上会使用吸力，半血以下会自愈或飞行避险   |

### 二、可调参数总览

| ConVar名称 | 默认值 | 说明 |
| :--- | :--- | :--- |
|   sm_hzs_botfakerespawn_lives   |   "3"   |   BOT生命总数   |
|   sm_hzs_botfakerespawn_countdown   |   "15.0"   |   BOT伪复活倒计时   |
|   sm_hzs_botfakerespawn_protect   |   "3.0"   |   BOT伪复活无敌时间   |
|   sm_hzs_knife_damage   |   "100"   |   近战武器范围伤害值   |
|   sm_hzs_knife_range   |   "2.0"   |   近战武器有效攻击范围（单位：米）   |
|   sm_hzs_scream_cooling   |   "60.0"   |   尖叫技能冷却时间   |
|   sm_hzs_scream_range   |   "30.0"   |   尖叫技能的有效范围（单位：米）   |
|   sm_hzs_scream_duration   |   "15.0"   |   尖叫技能吸引僵尸的持续时间   |
|   sm_hzs_scream_filepath   |   "player/waoh.wav"   |   尖叫音频路径（不带sound/）   |

hzs_zombieskill的可调参数过多，包括僵尸名称也要与``HanZombieScenarioZombieData.cfg``对应，因此暂不注册ConVar。如要调整，可打开``HZSZombieSkill\global.inc``进行编辑并保存，然后重新编译``hzs_zombieskill.sp``。

### 三、尚未解决的问题

1. 有时伪死亡的BOT仍会吸引一部分僵尸的注意（其他插件必须把在0 0 0点的人类视为已死亡）

2. 有时BOT显示的生命总数为0，但还活着（可能是因为其它插件修改damage的时机较为滞后，造成了伤害值误判，可以尝试用player_death钩子解决）

3. 安哥拉的技能还没有做完（喷毒、飞行都不太完善）

### 四、编译测试环境

SourcePawn Compiler 1.11.0.6947

### 五、借物表

csol资产解包 - H-AN

部分僵尸音效 - VALVE (Left 4 Dead 2) 

特效粒子 - Zenlenafelex (https://steamcommunity.com/sharedfiles/filedetails/?id=2119972050)