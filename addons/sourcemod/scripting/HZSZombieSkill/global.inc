//========================================================================================
// DEFINES
//========================================================================================

// 常规信息
#define VERSION                "1.1"       // 版本号
#define GAMEUNITS_TO_METERS     0.01905    // 游戏单位转米
#define MAX_MONEY               16000      // 玩家最大金钱数
#define MAX_HEALTH              150        // 玩家最大生命值

// 僵尸名称（需要与HanZombieScenarioZombieData.cfg同步）
#define ZOMBIE_SMOKE           "迷雾僵尸"
#define ZOMBIE_EXPLODE         "自爆僵尸"
#define ZOMBIE_HEAL            "治疗僵尸"
#define ZOMBIE_DEIMOS          "恶魔猎手"
#define ZOMBIE_BUTCHER         "憎恶屠夫"
#define ZOMBIE_WITCH           "嗜血女巫"
#define ZOMBIE_GHOST           "幽灵僵尸"
#define BOSS_ANGELA            "安哥拉"
#define BOSS_PANGZI            "巨型狂暴形态僵尸"

// 技能参数（非BOSS特殊僵尸）
#define SMOKE_FADETIME          5.0        // 几秒后迷雾僵尸的烟开始消散（秒）
#define SMOKE_KILLTIME          10.0       // 几秒后迷雾僵尸的烟彻底消散（秒）
#define EXPLODE_DAMAGE         "100"       // 自爆僵尸爆炸的伤害震级，和伤害大小正相关（字符）
#define EXPLODE_RANGE          "300"       // 自爆僵尸爆炸的伤害范围（字符）
#define HEAL_DAMAGE             50         // 巫蛊术尸群疗时每秒恢复的血量
#define HEAL_RANGE              5.0        // 巫蛊术尸群疗的作用范围（单位：米）
#define DEIMOS_KNOCKPOWER       500.0      // 恶魔猎手击飞玩家的力度（水平和垂直同力度）
#define BUTCHER_STUCKTIME       3.0        // 憎恶屠夫用鬼手把人控制住的持续时间（秒）
#define WITCH_INVERTTIME        3.0        // 嗜血女巫让人操作取反的持续时间（秒）

// 技能参数（BOSS安哥拉）
#define ANGELA_CALL_TIME        15.0       // 安哥拉呼唤僵尸集中攻击某个人类的持续时间（秒）
#define ANGELA_PULL_POWER       500.0      // 安哥拉吸力的力度
#define ANGELA_PULL_TIME        5.0        // 安哥拉吸力的持续时间（秒）
#define ANGELA_HEAL_DAMAGE      500        // 安哥拉自愈时每秒恢复的血量
#define ANGELA_HEAL_BULLET      50         // 打断安哥拉自愈所需的累计子弹数
#define ANGELA_KNOCK_DAMAGE     20.0       // 安哥拉群体击飞造成的伤害值（两种群体击飞共用）
#define ANGELA_KNOCK_RANGE      10.0       // 安哥拉群体击飞的作用范围（单位：米）
#define ANGELA_FLY_TIME         15.0       // 安哥拉飞行的持续时间（秒）
#define ANGELA_FLY_CD           30.0       // 安哥拉飞行的冷却时间（秒）
// 两种群体击飞在水平和垂直方向上有力度区别，不方便在这里修改值

// 技能参数（BOSS巨型狂暴形态僵尸）
#define PANGZI_CHARGE_TIME      3.0        // 巨型狂暴形态僵尸冲锋的持续时间（秒）
#define PANGZI_CHARGE_CD        5.0        // 巨型狂暴形态僵尸冲锋的冷却时间（秒）
#define PANGZI_CHARGE_FORCE     500.0      // 巨型狂暴形态僵尸冲锋的速度或者力度
#define PANGZI_GRAPPLE_RANGE    3.0        // 巨型狂暴形态僵尸触发擒抱的范围（单位：米）
#define PANGZI_GRAPPLE_DAMAGE   10.0       // 巨型狂暴形态僵尸擒抱每秒造成的伤害
#define PANGZI_GRAPPLE_CD       5.0        // 巨型狂暴形态僵尸擒抱的冷却时间（秒）
#define PANGZI_GRAPPLE_KEYRATE  4          // 真人玩家挣脱擒抱所需的每秒按E键次数

// 音效路径（非BOSS特殊僵尸）
#define SFX_SMOKE1             "custom/gas_cough_gasp_1.wav"                // 迷雾僵尸吐烟时的音效1（咳嗽声）
#define SFX_SMOKE2             "custom/explosivegasleak.wav"                // 迷雾僵尸吐烟时的音效2（吐烟声）
#define SFX_EXPLODE1           "custom/male_zombie10_growl1.wav"            // 自爆僵尸死亡时的音效
#define SFX_EXPLODE2           "custom/smoker_explode_01.wav"               // 自爆僵尸爆炸时的音效
#define SFX_KNOCKBACK          "custom/loud_chargerimpact_04.wav"           // 恶魔猎手或安哥拉击飞玩家的音效
#define SFX_BUTCHER            "custom/tank_yell_01.wav"                    // 憎恶屠夫的技能提示音效
#define SFX_WITCH              "custom/horrified_4.wav"                     // 嗜血女巫的技能提示音效

// 音效路径（BOSS安哥拉）
#define SFX_CALL               "custom/mega_mob_incoming.wav"               // 安哥拉呼唤僵尸集中攻击某个人类的音效
#define SFX_PULL1              "custom/hole_hit5.wav"                       // 安哥拉吸力音效
#define SFX_PULL2              "custom/tank_breathe_05.wav"                 // 安哥拉使用吸力时的吐息声
#define SFX_SMASH              "custom/explode_3.wav"                       // 安哥拉砸地动作的音效
#define SFX_SWING              "weapons/angela/zbs_attack_swing.wav"        // 安哥拉挥臂动作的音效
#define SFX_HEAL1              "custom/tank_growl_09.wav"                   // 安哥拉自愈时的吐息声
#define SFX_HEAL2              "custom/tank_breathe_06.wav"                 // 安哥拉自愈至满血时的吐息声
#define SFX_HEAL3              "custom/tank_growl_02.wav"                   // 安哥拉自愈被打断时的吐息声
#define SFX_FLY                "weapons/angela/angra_zbs_fly2.wav"          // 安哥拉扇动翅膀的音效
#define SFX_POISON             "weapons/angela/angra_zbs_poison_spit.wav"   // 安哥拉喷毒的音效

// 音效路径（BOSS巨型狂暴形态僵尸）
#define SFX_CHARGE1            "custom/charger_charge_02.wav"               // 巨型狂暴形态僵尸冲锋嚎叫声
#define SFX_CHARGE2            "custom/tank_walk06.wav"                     // 巨型狂暴形态僵尸冲锋震地声
#define SFX_GRAPPLE            "custom/charger_smash_01.wav"                // 巨型狂暴形态僵尸造成擒抱伤害的音效

//========================================================================================
// HANDLES & VARIABLES
//========================================================================================

// 偏移量
int g_iLeaderOffset = -1;                 // 获取人质跟随目标的偏移量

// 预缓存模型实体
int g_iBeamSprite = -1;                   // 辅助线模型，用于可视化方向数组

// 记录初始状态数组
char g_ClientName[MAXPLAYERS+1][32];      // 记住玩家的名字，用于print信息
int g_iMaxHealth[2048];                   // 记住每个僵尸的最大血量

// 技能状态管理（非BOSS特殊僵尸）
bool g_bIsStuck[MAXPLAYERS+1];            // 玩家是否中了憎恶屠夫的鬼手控制
bool g_bIsInvert[MAXPLAYERS+1];           // 玩家是否中了嗜血女巫的操作取反
bool g_bIsSkillUsed[2048];                // 技能是否使用过，或者是否正在使用某个技能（有些是一次性技能，有些可以重复使用）

// 技能状态管理（BOSS安哥拉）
int g_iZombieCall[MAXPLAYERS+1];          // 玩家是否被集中攻击，记录的是安哥拉的实体号
int g_iZombiePull[MAXPLAYERS+1];          // 玩家是否被安哥拉所吸，记录的是安哥拉的实体号
int g_iBulletCount[2048];                 // 安哥拉在自愈期间所受的累计子弹数
Handle RepeatTask[2048] = {INVALID_HANDLE,...};      // 实现治疗效果所用的timer，巫蛊术尸和安哥拉共用
Handle RepeatTask2[2048] = {INVALID_HANDLE,...};     // 实现吸力所用的timer

// 技能状态管理（BOSS安哥拉飞行逻辑 H-AN提供）
float g_flLastFlyEnd[2048];               // 记录上一次飞行开始/结束的时间（飞行CD逻辑）
float g_flLastFlySound[2048];             // 记录上一次扇翅膀音效的时间，用于播放音效
Handle RepeatTask3[2048] = {INVALID_HANDLE,...};     // 实现飞行所用的timer

enum BossState
{
    STATE_GROUND = 0,            // 地面模式
    STATE_FLYING                 // 飞行模式
}
BossState g_BossState[2048];

enum FlySubState
{
    FLY_RANDOM = 0,              // 高飞模式
    FLY_TARGET                   // 低飞模式
}
FlySubState g_Mode[2048];

// 技能状态管理（BOSS巨型狂暴形态僵尸 Gemini提供）
bool g_bIsGrappled[MAXPLAYERS+1];         // 玩家是否被擒抱
int g_iUsePressCount[MAXPLAYERS+1];       // 玩家在1秒内按E的次数
int g_iLastButtons[MAXPLAYERS+1];         // 玩家上一帧的按键（因为按下松开才算一次）
int g_iGrappling[2048];                   // 僵尸擒抱对象，记录玩家的实体号
Handle RepeatTask4[2048] = {INVALID_HANDLE,...};            // 实现冲锋所用的timer
Handle RepeatTask5[MAXPLAYERS+1] = {INVALID_HANDLE, ...};   // 实现擒抱伤害所用的timer
Handle RepeatTask6[MAXPLAYERS+1] = {INVALID_HANDLE, ...};   // 实现挣脱擒抱所用的timer