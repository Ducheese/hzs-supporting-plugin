//========================================================================================
// hzs_killfeed.sp — HAN 灾变击杀信息流
//
// 风格参考 TW MW System for Tactical Warfare 5.0.sp 的 Kill Feed：
//   - 准星下方 HUD 滚动队列（新行插顶、逐格下移、每行独立定时器过期）
//   - 按僵尸类型分级染色 + 分档积分（普通50/特殊200/BOSS1000）+ 积分榜 + 连杀提示
// 阉割项（无对照）：助攻/复仇/伤害矩阵/PvP 奖牌体系
//
// 数据源：Han_OnZombieDeath 转发（玩家杀僵尸），僵尸打玩家不做任何反馈
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <HanZombieScenarioAPI>

//========================================================================================
// 配置
//========================================================================================

#define FEED_LINES    5          // 滚动队列行数上限
#define FEED_LEN      64         // 单行长度
#define FEED_CHANNEL  2          // 准星下方通道
#define SCORE_CHANNEL 1          // 积分榜通道（原版奖牌位）
#define FEED_Y        0.55       // 准星下方
#define SCORE_X       0.40       // 积分榜（准星左侧）
#define SCORE_Y       0.42       // 两行起始：✪积分 / 名次▲▼差距

#define STREAK_WINDOW 5.0        // 连杀时间窗（秒）

// 各类型击杀积分：普通 50 / 特殊 200 / BOSS 1000（索引 = TYPE_*）
static const int g_iKillReward[3] = {50, 200, 1000};

// 僵尸类型（按 Han_GetZombieName 显示名匹配，独立插件拿不到内部 g_ZombieTypeName）
// BOSS：安哥拉 / 巨型狂暴形态僵尸 / 异形斗兽
// 特殊：治疗 / 自爆 / 幽灵 / 迷雾 / 恶魔 / 女巫 / 屠夫 / 神秘 / 伪人（含 zombieskill 的技能僵尸）
// 其余（普通/狂暴/强化/芭比系）→ 普通
// 注意：静态 const 二维数组的 sizeof 不可靠（error 163），用计数宏遍历，改名单须同步宏
#define TYPE_NORMAL   0
#define TYPE_SPECIAL  1
#define TYPE_BOSS     2
#define TYPE_STREAK   3        // feed 行类型：连杀提示

#define BOSS_NAME_COUNT 3
static const char BOSS_NAMES[][] =
{
    "安哥拉", "巨型狂暴形态僵尸", "异形斗兽"
};

#define SPECIAL_NAME_COUNT 9
static const char SPECIAL_NAMES[][] =
{
    "治疗僵尸", "自爆僵尸", "幽灵僵尸", "迷雾僵尸", "恶魔僵尸", "女巫僵尸", "屠夫僵尸", "神秘僵尸", "伪人僵尸"
};

//========================================================================================
// 全局
//========================================================================================

// 积分（每回合重置，客户端索引）
int g_iScore[MAXPLAYERS + 1];

// 连杀（每玩家，客户端索引，无实体索引问题）
int g_iStreak[MAXPLAYERS + 1];
float g_fLastKillTime[MAXPLAYERS + 1];

// feed 队列（每玩家，新行在 [0]）
char g_sFeed[MAXPLAYERS + 1][FEED_LINES][FEED_LEN];
int g_iFeedType[MAXPLAYERS + 1][FEED_LINES];

ConVar g_cvFeedTime;

public Plugin myinfo =
{
    name = "HZS Kill Feed",
    author = "Ducheese",
    description = "HAN 灾变击杀信息流：准星下方滚动 + 分档积分榜 + 连杀提示",
    version = "1.0.0",
    url = "https://space.bilibili.com/1889622121"
};

//========================================================================================
// 生命周期
//========================================================================================

public void OnPluginStart()
{
    g_cvFeedTime = CreateConVar("hzs_killfeed_time", "2.0", "每行击杀信息显示时长（秒）", FCVAR_NOTIFY, true, 0.5, true, 10.0);

    HookEvent("round_start", Event_RoundStart, EventHookMode_Post);
}

public void Event_RoundStart(Event event, const char[] name, bool dontBroadcast)
{
    // 积分与连杀按回合重置（round_start 在换图时也会触发，顺带覆盖换图残留）
    for (int i = 1; i <= MaxClients; i++)
    {
        g_iScore[i] = 0;
        g_iStreak[i] = 0;
        g_fLastKillTime[i] = 0.0;
    }

    // UpdateScoreboard();
}

public void OnClientDisconnect(int client)
{
    g_iScore[client] = 0;
    g_iStreak[client] = 0;
    g_fLastKillTime[client] = 0.0;
}

//========================================================================================
// 击杀结算（唯一事件源：玩家杀僵尸）
//========================================================================================

public void Han_OnZombieDeath(int zombie, int killer)
{
    if (killer <= 0 || killer > MaxClients || !IsClientInGame(killer))
        return;   // 世界/自杀/僵尸互杀不做反馈

    int type = GetZombieType(zombie);
    int reward = g_iKillReward[type];
    g_iScore[killer] += reward;
    UpdateScoreboard();

    char name[64];
    Han_GetZombieName(zombie, name, sizeof(name));

    char msg[FEED_LEN];
    switch (type)
    {
        case TYPE_NORMAL:  Format(msg, sizeof(msg), "%s Killed +%d", name, reward);
        case TYPE_SPECIAL: Format(msg, sizeof(msg), "%s Killed +%d", name, reward);
        case TYPE_BOSS:    Format(msg, sizeof(msg), "★%s Killed +%d", name, reward);
    }
    AddFeedLine(killer, msg, type);

    StreakCheck(killer);
}

// 显示名 → 类型（普通/特殊/BOSS）
int GetZombieType(int zombie)
{
    char name[64];
    Han_GetZombieName(zombie, name, sizeof(name));

    for (int i = 0; i < BOSS_NAME_COUNT; i++)
    {
        if (StrEqual(name, BOSS_NAMES[i]))
            return TYPE_BOSS;
    }

    for (int i = 0; i < SPECIAL_NAME_COUNT; i++)
    {
        if (StrEqual(name, SPECIAL_NAMES[i]))
            return TYPE_SPECIAL;
    }

    return TYPE_NORMAL;
}

//========================================================================================
// 连杀（5s 窗口，2=DOUBLE 3=TRIPLE >3=MULTI，提示作为 feed 高亮行；文本照抄原插件 medals）
//========================================================================================

void StreakCheck(int client)
{
    float now = GetGameTime();

    if (now - g_fLastKillTime[client] < STREAK_WINDOW)
    {
        g_iStreak[client]++;
        if (g_iStreak[client] == 2)      AddFeedLine(client, "DOUBLE KILL", TYPE_STREAK);
        else if (g_iStreak[client] == 3) AddFeedLine(client, "TRIPLE KILL", TYPE_STREAK);
        else if (g_iStreak[client] > 3)  AddFeedLine(client, "MULTI KILL", TYPE_STREAK);
    }
    else
    {
        g_iStreak[client] = 1;
    }

    g_fLastKillTime[client] = now;
}

//========================================================================================
// feed 队列（原版机制：新行插顶、逐格下移、每行独立定时器过期、userid 传参）
//========================================================================================

void AddFeedLine(int client, const char[] text, int type)
{
    for (int i = FEED_LINES - 1; i > 0; i--)
    {
        strcopy(g_sFeed[client][i], FEED_LEN, g_sFeed[client][i - 1]);
        g_iFeedType[client][i] = g_iFeedType[client][i - 1];
    }
    strcopy(g_sFeed[client][0], FEED_LEN, text);
    g_iFeedType[client][0] = type;

    CreateTimer(g_cvFeedTime.FloatValue, Timer_ClearLine, GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);

    ShowFeed(client);
}

public Action Timer_ClearLine(Handle timer, int userid)
{
    int client = GetClientOfUserId(userid);
    if (client <= 0 || !IsClientInGame(client))
        return Plugin_Stop;   // 掉线了，队列自然废弃

    for (int i = 0; i < FEED_LINES - 1; i++)
    {
        strcopy(g_sFeed[client][i], FEED_LEN, g_sFeed[client][i + 1]);
        g_iFeedType[client][i] = g_iFeedType[client][i + 1];
    }
    g_sFeed[client][FEED_LINES - 1][0] = '\0';

    ShowFeed(client);
    return Plugin_Stop;
}

void ShowFeed(int client)
{
    char buffer[512];
    buffer[0] = '\0';

    for (int i = 0; i < FEED_LINES; i++)
    {
        if (g_sFeed[client][i][0] != '\0')
        {
            StrCat(buffer, sizeof(buffer), g_sFeed[client][i]);
            StrCat(buffer, sizeof(buffer), "\n");
        }
    }

    if (buffer[0] == '\0')
        return;

    // 按最新一行的类型染色（HUD 单色，整块一起染）
    switch (g_iFeedType[client][0])
    {
        case TYPE_SPECIAL: SetHudTextParams(-1.0, FEED_Y, g_cvFeedTime.FloatValue, 135, 206, 250, 255);  // 特殊：浅蓝
        case TYPE_BOSS:    SetHudTextParams(-1.0, FEED_Y, g_cvFeedTime.FloatValue, 255, 200, 0, 255);    // BOSS：金
        case TYPE_STREAK:  SetHudTextParams(-1.0, FEED_Y, g_cvFeedTime.FloatValue, 255, 80, 80, 255);    // 连杀：红
        default:           SetHudTextParams(-1.0, FEED_Y, g_cvFeedTime.FloatValue, 255, 255, 255, 255);  // 普通：白
    }
    ShowHudText(client, FEED_CHANNEL, "%s", buffer);
}

//========================================================================================
// 积分榜（通道 1：只显示自己，准星左侧两行：✪积分 / #名次▲▼差距；第一名 ▲ 领先、其余 ▼ 落后；击杀时立即刷新）
//========================================================================================

void UpdateScoreboard()
{
    for (int viewer = 1; viewer <= MaxClients; viewer++)
    {
        if (!IsClientInGame(viewer))
            continue;

        // 名次 = 在线且积分严格高于自己的玩家数 + 1；
        // 上一位分数 = 高于自己的最低分（紧邻自己上面的那位）；
        // 第二名分数 = 低于自己的最高分（第一名的领先参照）
        int rank = 1;
        int nextScore = -1;    // rank>1：上一位的分数
        int secondScore = -1;  // rank==1：第二名的分数
        for (int i = 1; i <= MaxClients; i++)
        {
            if (i == viewer || !IsClientInGame(i))
                continue;

            if (g_iScore[i] > g_iScore[viewer])
            {
                rank++;
                if (nextScore == -1 || g_iScore[i] < nextScore)
                    nextScore = g_iScore[i];
            }
            else if (g_iScore[i] > secondScore)
                secondScore = g_iScore[i];
        }

        // 两行：✪积分 / #名次▲▼差距（第一名 ▲ 领先第二名，其余 ▼ 落后上一位）
        char text[64];
        char gap[16];
        if (rank == 1)
        {
            if (secondScore != -1)
                Format(gap, sizeof(gap), "▲%d", g_iScore[viewer] - secondScore);
            else
                gap[0] = '\0';   // 全场只有自己，无参照
        }
        else
        {
            Format(gap, sizeof(gap), "▼%d", nextScore - g_iScore[viewer]);
        }

        Format(text, sizeof(text), "✪%d\n#%d%s", g_iScore[viewer], rank, gap);

        // 前三名金银铜，#4+ 灰
        switch (rank)
        {
            case 1:  SetHudTextParams(SCORE_X, SCORE_Y, 1.0, 255, 200, 0, 255);      // 金
            case 2:  SetHudTextParams(SCORE_X, SCORE_Y, 1.0, 130, 185, 240, 255);    // 银（完美）
            case 3:  SetHudTextParams(SCORE_X, SCORE_Y, 1.0, 240, 100, 70, 255);     // 铜（完美）
            default: SetHudTextParams(SCORE_X, SCORE_Y, 1.0, 50, 50, 50, 50);        // 灰
        }
        ShowHudText(viewer, SCORE_CHANNEL, "%s", text);
    }
}
