### 插件列表

| .smx名称 | 简述 | 具体特性 |
| :--- | :--- | :--- |
|   hzs_botfakerespawn   |   用伪死亡和伪复活来解决H-AN大灾变模式插件BOT不能复活的问题   |   1. BOT即将死亡时会被传送至坐标0 0 0点，等待复活传送至任意CT复活点，复活后有无敌时间和防卡人机制<br />2. BOT名称新增生命总数和复活倒计时前缀<br />3. 复活后的BOT会补满血量并买甲   |
|   hzs_szombieskill   |   实现特殊僵尸技能和BOSS技能   |   -   |
|   hzs_knife   |   人类被动技能，让近战武器具有范围伤害   |   -   |
|   hzs_scream   |   人类主动技能，可以发出尖叫吸引全场僵尸一段时间   |   -   |

### 可调参数总览

| ConVar名称 | 默认值 | 说明 |
| :--- | :--- | :--- |
|   sm_hzs_botfakerespawn_lives   |   "3"   |   BOT生命总数   |
|   sm_hzs_botfakerespawn_countdown   |   "15.0"   |   BOT伪复活倒计时   |
|   sm_hzs_botfakerespawn_protect   |   "3.0"   |   BOT伪复活无敌时间   |

### 尚未解决的问题

1. 有时伪死亡的BOT仍会吸引一部分僵尸的注意（其他插件必须把在0 0 0点的人类视为已死亡）；
2. 有时BOT显示的生命总数为0，但还活着（可能是因为其它插件修改damage的时机较为滞后，造成了伤害值误判，可以尝试用player_death钩子解决）；

### 编译测试环境

SourcePawn Compiler 1.11.0.6947