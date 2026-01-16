//========================================================================================
// DEFINES
//========================================================================================

// 常规信息
#define VERSION                 "1.0"
#define GAMEUNITS_TO_METERS     0.01905 
#define MAX_MONEY               16000
#define MAX_HEALTH              150

// 僵尸名称（需要与HanZombieScenarioZombieData.cfg同步）
#define ZOMBIE_SMOKE            "迷雾僵尸"
#define ZOMBIE_EXPLODE          "自爆僵尸"
#define ZOMBIE_HEAL             "治疗僵尸"
#define ZOMBIE_DEIMOS           "恶魔猎手"
#define ZOMBIE_BUTCHER          "憎恶屠夫"
#define ZOMBIE_WITCH            "嗜血女巫"
#define ZOMBIE_GHOST            "幽灵僵尸"
#define BOSS_ANGELA             "安哥拉"
#define BOSS_PANGZI             "巨型狂暴形态僵尸"

// 技能参数
#define SmokeFadeTime           5.0        // 几秒后迷雾僵尸的烟开始消散（秒）
#define SmokeKillTime           10.0       // 几秒后迷雾僵尸的烟彻底消散（秒）
#define ExplodeDamage           "100"      // 自爆僵尸爆炸伤害震级（字符）
#define ExplodeRange            "300"      // 自爆僵尸爆炸伤害范围（字符）
#define HealDamage              50         // 巫蛊术尸群疗时每秒恢复的血量
#define HealRange               5.0        // 巫蛊术尸群疗作用范围（单位：米）
#define KnockBackPower          500.0      // 恶魔猎手击飞的力度
#define StuckTime               3.0        // 憎恶屠夫把人控制住的持续时间（秒）
#define DisorderTime            3.0        // 致幻女妖的致幻持续时间（秒）

#define CallDuration            15.0       // 安哥拉呼唤僵尸集中攻击持续时间（秒）
#define PullPower               500.0      // 安哥拉吸力的力度
#define SelfHealDamage          500        // 安哥拉自愈时每秒恢复的血量
#define BulletThreshold         50         // 打断安哥拉自愈所需的子弹数
// #define GroupKnockPower         800.0      // 两种群体击飞动作，已经设计好了水平和垂直方向上的区别，不方便在这里修改值
#define GroupKnockDamage        20.0       // 安哥拉群体击飞生成的伤害值
#define GroupKnockRange         10.0       // 安哥拉群体击飞作用范围（单位：米）
#define FlyDuration             15.0       // 飞行持续时间（秒）
#define FlyCDTime               30.0       // 飞行冷却时间（秒）

#define ChargeDuration          3.0        // 巨型狂暴形态僵尸冲刺持续时间（秒）
#define ChargeCDTime            5.0       // 巨型狂暴形态僵尸冲刺的冷却时间（秒）
#define ChargeForce             500.0      // 巨型狂暴形态僵尸冲刺的速度或者力度
#define GrappleRange            2.0        // 巨型狂暴形态僵尸触发擒抱的范围（单位：米）
#define GrappleDamage           10.0       // 巨型狂暴形态僵尸擒抱每秒造成的伤害
#define EscapeKeyPressRate      4          // 挣脱擒抱所需的每秒按E键次数

// 音效路径
#define SFX_SMOKE1              "custom/gas_cough_gasp_1.wav"                // 迷雾僵尸吐烟时的音效1（咳嗽声）
#define SFX_SMOKE2              "custom/explosivegasleak.wav"                // 迷雾僵尸吐烟时的音效2（吐烟声）
#define SFX_EXPLODE1            "custom/male_zombie10_growl1.wav"            // 自爆僵尸死亡时的音效
#define SFX_EXPLODE2            "custom/smoker_explode_01.wav"               // 自爆僵尸爆炸时的音效
#define SFX_KNOCKBACK           "custom/loud_chargerimpact_04.wav"           // 恶魔猎手击飞玩家的音效
#define SFX_STUCK               "custom/tank_yell_01.wav"                    // 憎恶屠夫定身玩家的音效
#define SFX_DISORDER            "custom/horrified_4.wav"                     // 嗜血女巫干扰玩家的音效

#define SFX_PULL                "custom/hole_hit5.wav"                       // 玩家被吸力控制时，高频播放的吸力音效
#define SFX_BREATH_PULL         "custom/tank_breathe_05.wav"                 // 安哥拉使用吸力时的吐息声
#define SFX_SMASH               "custom/explode_3.wav"                       // 安哥拉砸地动作的音效
#define SFX_SWING               "weapons/angela/zbs_attack_swing.wav"        // 安哥拉挥臂动作的音效
#define SFX_ZOMBIECALL          "custom/mega_mob_incoming.wav"               // 安哥拉呼唤僵尸集中攻击的音效
#define SFX_BREATH_HEAL_ING     "custom/tank_growl_09.wav"                   // 安哥拉自愈时的吐息声
#define SFX_BREATH_HEAL_FULL    "custom/tank_breathe_06.wav"                 // 安哥拉自愈至满血时的吐息声
#define SFX_BREATH_HEAL_FAIL    "custom/tank_growl_02.wav"                   // 安哥拉自愈被打断时的吐息声
#define SFX_FLY                 "weapons/angela/angra_zbs_fly2.wav"          // 安哥拉扇动翅膀的音效
#define SFX_POISON              "weapons/angela/angra_zbs_poison_spit.wav"   // 安哥拉喷毒的音效

#define SFX_CHARGE_HOWL         "custom/charger_charge_02.wav"               // 巨型狂暴形态僵尸冲刺嚎叫声
#define SFX_CHARGE_SHAKE        "custom/tank_walk06.wav"                     // 巨型狂暴形态僵尸冲刺震地声
#define SFX_GRAPPLE_HURT        "custom/charger_smash_01.wav"                // 巨型狂暴形态僵尸造成擒抱伤害的音效

//========================================================================================
// HANDLES & VARIABLES
//========================================================================================

// 玩家相关数组
int g_iZombieCall[MAXPLAYERS+1];          // 玩家是否被呼唤僵尸集中攻击，记录的是安哥拉的实体号
int g_iZombiePull[MAXPLAYERS+1];          // 玩家是否被安哥拉所吸，记录的是安哥拉的实体号

bool g_bIsStuck[MAXPLAYERS+1];            // 玩家是否中了憎恶屠夫的定身
bool g_bIsDisorder[MAXPLAYERS+1];         // 玩家是否中了嗜血女巫的干扰

char g_ClientName[MAXPLAYERS+1][32];      // 记住玩家的名字，用于print信息

// 僵尸相关数组
int g_iBulletCount[2048];                 // 安哥拉在自愈期间所受的子弹数累计
int g_iMaxHealth[2048];                   // 记住僵尸的最大血量

bool g_bIsSkillUsed[2048];                // 技能是否使用过，或者是否正在使用某个技能（有些是一次性技能，有些可以重复使用）

// 计时器
Handle RepeatTask[2048] = {INVALID_HANDLE,...};      // 实现治疗效果所用的timer，巫蛊术尸和安哥拉共用
Handle RepeatTask2[2048] = {INVALID_HANDLE,...};     // 吸力效果

//========================================================================================
// BOSS飞行逻辑 H-AN提供
//========================================================================================

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

int g_iLeaderOffset = -1;

float g_flLastFlyEnd[2048];        // 记录上一次飞行结束的时间（飞行CD）
float g_flLastFlySound[2048];      // 记录上一次扇翅膀音效的时间

Handle RepeatTask3[2048] = {INVALID_HANDLE,...};

//========================================================================================
// BOSS擒抱逻辑 Gemini提供
//========================================================================================

int g_iUsePressCount[MAXPLAYERS+1];       // 玩家在1秒内按E的次数
int g_iLastButtons[MAXPLAYERS+1];         // 玩家上一帧的按键（因为按下松开才算一次）

bool g_bIsGrappled[MAXPLAYERS+1];         // 玩家是否被擒抱

Handle RepeatTask5[MAXPLAYERS+1] = {INVALID_HANDLE, ...};   // 擒抱伤害句柄
Handle RepeatTask6[MAXPLAYERS+1] = {INVALID_HANDLE, ...};   // 挣脱擒抱句柄

int g_iGrappling[2048];                   // 僵尸擒抱对象，记录玩家的实体号

Handle RepeatTask4[2048] = {INVALID_HANDLE,...};     // 狂暴僵尸冲刺句柄