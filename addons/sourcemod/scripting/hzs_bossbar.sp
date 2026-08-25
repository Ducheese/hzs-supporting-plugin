// hzs_bossbar.sp — BOSS 实时受击血条
// 触发：Han_OnZombieHurt / Han_OnZombieDeath
// 显示：█(U+2588) / ░(U+2591)，通过 PrintCenterText 居中展示
// 机制：伤害限频 0.1s，仅向攻击过当前 BOSS 的玩家组广播

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <cstrike>
#include <HanZombieScenarioAPI>

#define VERSION "1.0.2"
#define BAR_LEN_DEFAULT 24
#define BAR_LEN_MIN 10
#define BAR_LEN_MAX 30
#define HURT_INTERVAL 0.10  // 同攻击者同 BOSS 限频（秒）

// BOSS 名称（与 HanZombieScenarioZombieData.cfg 及 HZSZombieSkill 严格一致）
#define BOSS_ANGELA     "安哥拉"
#define BOSS_PANGZI     "巨型狂暴形态僵尸"
#define BOSS_YIXING     "异形斗兽"

public Plugin myinfo = {
    name = "HZS Boss Bar",
    author = "Ducheese",
    description = "BOSS 受击实时血条 █/░",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
};

// 全局数据
int g_iMaxHealth[4096];                 // hostage_entity 索引 -> 出生最大血量
int g_iBarTarget[MAXPLAYERS + 1];       // client -> 当前锁定的 boss ent reference
float g_fLastBarTime[MAXPLAYERS + 1];   // client -> 上次发送 HUD 的时间戳

ConVar g_cvBarLen;
int g_iBarLen;

public void OnPluginStart()
{
    g_cvBarLen = CreateConVar("sm_bossbar_len", "24", "血条长度", _, true, float(BAR_LEN_MIN), true, float(BAR_LEN_MAX));
    g_cvBarLen.AddChangeHook(OnConVarChanged);
    g_iBarLen = g_cvBarLen.IntValue;

    for (int i = 1; i <= MaxClients; i++)
    {
        g_iBarTarget[i] = -1;
        g_fLastBarTime[i] = 0.0;
    }
}

public void OnConVarChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
    g_iBarLen = g_cvBarLen.IntValue;
}

public void OnMapStart()
{
    for (int i = 0; i < sizeof(g_iMaxHealth); i++)
    {
        g_iMaxHealth[i] = 0;
    }

    for (int i = 1; i <= MaxClients; i++)
    {
        g_iBarTarget[i] = -1;
        g_fLastBarTime[i] = 0.0;
    }
}

public void OnClientDisconnect(int client)
{
    if (client >= 1 && client <= MaxClients)
    {
        g_iBarTarget[client] = -1;
        g_fLastBarTime[client] = 0.0;
    }
}

// ============================================================================
// >> HZS 核心事件回调
// ============================================================================

public void Han_OnZombieCreated(int zombie)
{
    if (!IsValidEntity(zombie)) return;
    if (!IsBoss(zombie)) return;

    int maxHp = GetEntProp(zombie, Prop_Data, "m_iHealth");
    if (zombie >= 0 && zombie < sizeof(g_iMaxHealth))
    {
        g_iMaxHealth[zombie] = maxHp;
    }
}

public void Han_OnZombieDeath(int zombie, int killer)
{
    if (!IsValidEntity(zombie)) return;

    char name[64];
    if (!IsBoss(zombie, name, sizeof(name))) return;

    int maxHp = 0;
    if (zombie >= 0 && zombie < sizeof(g_iMaxHealth))
    {
        maxHp = g_iMaxHealth[zombie];
        g_iMaxHealth[zombie] = 0;
    }

    // 匹配并清理该 BOSS 的玩家追踪状态，同时推送 0% 击杀结算 HUD
    int ref = EntIndexToEntRef(zombie);
    for (int i = 1; i <= MaxClients; i++)
    {
        if (g_iBarTarget[i] == ref)
        {
            g_iBarTarget[i] = -1;
            if (IsClientInGame(i) && !IsFakeClient(i) && IsPlayerAlive(i))
            {
                ShowBossBar(i, 0, (maxHp > 0) ? maxHp : 1, name);
            }
        }
    }
}

public void Han_OnZombieHurt(int zombie, int attacker)
{
    // 严格先校验客户端索引范围，防止 Client index 0 invalid 崩溃
    if (attacker < 1 || attacker > MaxClients) return;
    if (!IsValidEntity(zombie) || !IsClientInGame(attacker) || !IsPlayerAlive(attacker)) return;
    if (GetClientTeam(attacker) == CS_TEAM_NONE) return;

    char name[64];
    if (!IsBoss(zombie, name, sizeof(name))) return;

    int ref = EntIndexToEntRef(zombie);
    g_iBarTarget[attacker] = ref;

    int nowHp = GetEntProp(zombie, Prop_Data, "m_iHealth");
    int maxHp = (zombie >= 0 && zombie < sizeof(g_iMaxHealth) && g_iMaxHealth[zombie] > 0) ? g_iMaxHealth[zombie] : nowHp;
    if (nowHp < 0) nowHp = 0;

    // 组广播 + 每人 0.1s 限频推送
    float now = GetGameTime();
    for (int i = 1; i <= MaxClients; i++)
    {
        if (!IsClientInGame(i) || IsFakeClient(i) || !IsPlayerAlive(i)) continue;
        if (g_iBarTarget[i] != ref) continue;
        if (now - g_fLastBarTime[i] < HURT_INTERVAL) continue;

        g_fLastBarTime[i] = now;
        ShowBossBar(i, nowHp, maxHp, name);
    }
}

// ============================================================================
// >> 辅助与渲染函数
// ============================================================================

bool IsBoss(int zombie, char[] nameBuf = "", int maxLen = 0)
{
    if (!IsValidEntity(zombie)) return false;

    char name[64];
    if (Han_GetZombieName(zombie, name, sizeof(name)) && name[0] != '\0')
    {
        if (StrEqual(name, BOSS_ANGELA) || StrEqual(name, BOSS_PANGZI) || StrEqual(name, BOSS_YIXING))
        {
            if (maxLen > 0)
            {
                strcopy(nameBuf, maxLen, name);
            }
            return true;
        }
    }

    return false;
}

void ShowBossBar(int client, int nowHp, int maxHp, const char[] name)
{
    if (!IsClientInGame(client) || IsFakeClient(client)) return;

    int barLen = g_iBarLen;
    if (barLen < BAR_LEN_MIN) barLen = BAR_LEN_MIN;
    if (barLen > BAR_LEN_MAX) barLen = BAR_LEN_MAX;
    if (maxHp <= 0) maxHp = 1;

    float p = float(nowHp) / float(maxHp);
    if (p < 0.0) p = 0.0;
    if (p > 1.0) p = 1.0;

    int nowLen = RoundToCeil(p * float(barLen));
    int dmgLen = barLen - nowLen;

    // UTF-8 字符: █ (3B), ░ (3B). 30 * 3 + 1 = 91 字节
    char fill[96], empty[96], bar[192];
    fill[0] = '\0';
    empty[0] = '\0';

    for (int i = 0; i < nowLen; i++) StrCat(fill, sizeof(fill), "█"); // U+2588
    for (int i = 0; i < dmgLen; i++) StrCat(empty, sizeof(empty), "░"); // U+2591
    FormatEx(bar, sizeof(bar), "%s%s", fill, empty);

    int pct = RoundToNearest(p * 100.0);
    char msg[256];
    FormatEx(msg, sizeof(msg), "%s\n%s (%d%%) [%d/%d]", bar, name, pct, nowHp, maxHp);
    PrintCenterText(client, "%s", msg);
}
