//========================================================================================
// DEFINES
//========================================================================================

#define VERSION                "1.0"
#define NAME_CHANGE_STRING     "#Cstrike_Name_Change"

//========================================================================================
// INCLUDES
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <cstrike>
#include <morecolors>

//========================================================================================
// HANDLES & VARIABLES
//========================================================================================

int g_iLivesRemaining[MAXPLAYERS+1];
float g_fDeadTime[MAXPLAYERS+1];
char g_ClientName[MAXPLAYERS+1][32];

Handle h_SpawnPoint;
Handle h_RespawnTask[MAXPLAYERS+1] = {INVALID_HANDLE,...};
Handle h_CollisionTask[MAXPLAYERS+1] = {INVALID_HANDLE,...};

ConVar cvarRespawnLives;
ConVar cvarRespawnCountdown;
ConVar cvarRespawnProtect;

//========================================================================================
//========================================================================================

public Plugin myinfo =
{
    name = "HZS BOT True Respawn",
    author = "Ducheese",
    description = "让H-AN大灾变模式插件BOT真死真复活（配合hzs_botaddfix不会崩服）",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
}

public void OnPluginStart()
{
    CreateConVar("sm_hzs_bottruerespawn_version", VERSION, "插件版本", FCVAR_PROTECTED);
    cvarRespawnLives = CreateConVar("sm_hzs_bottruerespawn_lives", "3", "BOT生命总数（0=无限复活）", FCVAR_NOTIFY, true, 0.0);
    cvarRespawnCountdown = CreateConVar("sm_hzs_bottruerespawn_countdown", "15.0", "BOT复活倒计时", FCVAR_NOTIFY, true, 0.0);
    cvarRespawnProtect = CreateConVar("sm_hzs_bottruerespawn_protect", "3.0", "BOT复活无敌时间", FCVAR_NOTIFY, true, 0.0);

    HookEvent("player_spawn", Event_PlayerSpawn);                          // 给复活倒计时收个尾
    HookEvent("player_death", Event_PlayerDeath);                          // BOT死亡，更新生命计数，启动复活倒计时
    HookEvent("player_team", Event_PlayerTeam);                            // 真人退出或补位BOT加入队伍时，启动复活

    HookEvent("round_start", Event_RoundStart);                            // 新回合重置所有BOT的生命总数
    HookEvent("round_freeze_end", Event_RoundFreezeEnd);                   // 为了配合round exec插件，即便晚于round start加载插件，也能顺利获取地图出生点实体信息
    HookUserMessage(GetUserMessageId("SayText2"), Hook_SayText2, true);    // 避免改名信息刷屏

    h_SpawnPoint = CreateArray(1);                                         // 保存复活点实体信息
}

public void OnMapStart()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        if (h_RespawnTask[i] != INVALID_HANDLE)
        {
            KillTimer(h_RespawnTask[i]);
            h_RespawnTask[i] = INVALID_HANDLE;
        }

        if (h_CollisionTask[i] != INVALID_HANDLE)
        {
            KillTimer(h_CollisionTask[i]);
            h_CollisionTask[i] = INVALID_HANDLE;
        }
    }
}

public void OnClientPutInServer(int client)
{
    GetClientName(client, g_ClientName[client], 32);

    // 回合中途加入的BOT也要初始化生命数（round_start 时它还没进服）
    if (IsFakeClient(client))
        g_iLivesRemaining[client] = GetConVarInt(cvarRespawnLives);
}

public void OnClientDisconnect_Post(int client)
{
    g_ClientName[client] = NULL_STRING;

    if (h_RespawnTask[client] != INVALID_HANDLE)
    {
        KillTimer(h_RespawnTask[client]);
        h_RespawnTask[client] = INVALID_HANDLE;
    }

    if (h_CollisionTask[client] != INVALID_HANDLE)
    {
        KillTimer(h_CollisionTask[client]);
        h_CollisionTask[client] = INVALID_HANDLE;
    }
}

//========================================================================================
// HOOK
//========================================================================================

public void Event_PlayerTeam(Handle event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(GetEventInt(event, "userid"));
    int team = GetEventInt(event, "team");

    if (IsValidClient(client, false) && IsFakeClient(client))
    {
        if (team > CS_TEAM_SPECTATOR && !IsPlayerAlive(client))
        {
            if (g_iLivesRemaining[client] <= 0)
                g_iLivesRemaining[client] = GetConVarInt(cvarRespawnLives);

            if (h_RespawnTask[client] == INVALID_HANDLE)
            {
                StartRespawnCountdown(client);
                UpdateNamePrefix(client, true);
            }
        }
    }
}

public void Event_RoundStart(Handle event, const char[] name, bool dontBroadcast)
{
    // 新回合重置所有BOT的生命总数。
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsValidClient(i, false) && IsFakeClient(i))
        {
            g_iLivesRemaining[i] = GetConVarInt(cvarRespawnLives);
            UpdateNamePrefix(i, false);    // 不清楚和player_spawn哪个先呢，留个冗余吧
        }
    }
}

public void Event_RoundFreezeEnd(Handle event, const char[] name, bool dontBroadcast)
{
    ClearArray(h_SpawnPoint);

    int entity;

    while ((entity = FindEntityByClassname(entity, "info_player_counterterrorist")) != -1)      // 复活只传送到CT复活点
    {
        PushArrayCell(h_SpawnPoint, EntIndexToEntRef(entity));
    }

    // while ((entity = FindEntityByClassname(entity, "info_player_terrorist")) != -1)          // 如果需要T复活点就注释掉这个
    // {
    //     PushArrayCell(h_SpawnPoint, EntIndexToEntRef(entity));
    // }
}

public void Event_PlayerSpawn(Handle event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(GetEventInt(event, "userid"));

    if (IsValidClient(client, false) && IsFakeClient(client))
    {
        // 刚复活时的名字前缀更新。
        UpdateNamePrefix(client, false);
    }
}

public void Event_PlayerDeath(Handle event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(GetEventInt(event, "userid"));

    if (!IsValidClient(client, false) || !IsFakeClient(client)) return;

    // 无限复活模式：cvar 取 0（含负数）时实时判定，不扣生命
    bool bInfinite = (GetConVarInt(cvarRespawnLives) <= 0);

    if (!bInfinite)
        g_iLivesRemaining[client]--;

    if (bInfinite || g_iLivesRemaining[client] > 0)
    {
        // 还有命：启动真实复活倒计时
        PrintDeathMessage(client, false);
        StartRespawnCountdown(client);          // 先设置 g_fDeadTime
        UpdateNamePrefix(client, true);         // 再显示倒计时（冗余）
    }
    else
    {
        // 彻底死透，不再复活
        PrintDeathMessage(client, true);
        UpdateNamePrefix(client, false);
    }
}

public Action Hook_SayText2(UserMsg msg_id, any msg, const int[] players, int playersNum, bool reliable, bool init)
{
    char[] sMessage = new char[24];

    if (GetUserMessageType() == UM_Protobuf)
    {
        Protobuf pbmsg = msg;
        pbmsg.ReadString("msg_name", sMessage, 24);
    }
    else
    {
        BfRead bfmsg = msg;
        bfmsg.ReadByte();
        bfmsg.ReadByte();
        bfmsg.ReadString(sMessage, 24, false);
    }

    if (StrEqual(sMessage, NAME_CHANGE_STRING))
    {
        return Plugin_Handled;          // 屏蔽改名信息
    }

    return Plugin_Continue;
}

//========================================================================================
// FUCTIONS
//========================================================================================

void PrintDeathMessage(int client, bool deadForever)
{
    if (deadForever)
        CPrintToChatAll("{green}[华仔] {red}%s死了！彻底死透了！", g_ClientName[client]);
    else if (GetConVarInt(cvarRespawnLives) <= 0)
        CPrintToChatAll("{green}[华仔] {red}%s死了！", g_ClientName[client]);
    else
        CPrintToChatAll("{green}[华仔] {red}%s死了！他还剩%d条命", g_ClientName[client], g_iLivesRemaining[client]);
}

void StartRespawnCountdown(int client)
{
    if (h_RespawnTask[client] != INVALID_HANDLE)
    {
        KillTimer(h_RespawnTask[client]);
        h_RespawnTask[client] = INVALID_HANDLE;
    }

    g_fDeadTime[client] = GetGameTime();

    h_RespawnTask[client] = CreateTimer(1.0, Timer_Respawn, client, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

public Action Timer_Respawn(Handle timer, int client)
{
    // bAlive 必须传 false，已复活用 IsPlayerAlive 单独判断
    if (!IsValidClient(client, false) || !IsFakeClient(client) || IsPlayerAlive(client))
    {
        h_RespawnTask[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    if (GetGameTime() - g_fDeadTime[client] > GetConVarFloat(cvarRespawnCountdown))
    {
        // 真复活（满血满弹，与人类玩家同流程）
        CS_RespawnPlayer(client);

        // CS_RespawnPlayer 的出生点选择不可控，随机传送到任意CT复活点
        TeleportRespawnPoint(client);

        // CS_RespawnPlayer 不给甲，复活后补买
        FakeClientCommand(client, "buy vesthelm");
        FakeClientCommand(client, "buy vest");

        // 取消碰撞然后延时恢复
        SetEntProp(client, Prop_Data, "m_CollisionGroup", 2);      // COLLISION_GROUP_DEBRIS_TRIGGER 消除碰撞体积
        h_CollisionTask[client] = CreateTimer(1.0, Timer_SetCollision, client, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);       // 每秒进行一次碰撞检测

        // 设置无敌时间
        ProtectClient(client, GetConVarFloat(cvarRespawnProtect));

        h_RespawnTask[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    // 复活倒计时在名字上实时更新
    UpdateNamePrefix(client, true);

    return Plugin_Continue;
}

void TeleportRespawnPoint(int client)
{
    if (GetArraySize(h_SpawnPoint) == 0) return;   // 未收集到出生点就不传送，留在引擎默认出生点

    int rand = GetRandomInt(0, GetArraySize(h_SpawnPoint)-1);    // 随机数

    int ref = GetArrayCell(h_SpawnPoint, rand, 0);

    if (ref != INVALID_ENT_REFERENCE)
    {
        float pos[3], ang[3];
        GetEntPropVector(ref, Prop_Send, "m_vecOrigin", pos);
        GetEntPropVector(ref, Prop_Send, "m_angRotation", ang);
        TeleportEntity(client, pos, ang, NULL_VECTOR);
    }
}

void UpdateNamePrefix(int client, bool waitingRespawn)
{
    char NewName[64];

    if (!waitingRespawn)
    {
        if (GetConVarInt(cvarRespawnLives) <= 0)
            Format(NewName, sizeof(NewName), "[❤∞] %s", g_ClientName[client]);
        else
            Format(NewName, sizeof(NewName), "[❤x%d] %s", g_iLivesRemaining[client], g_ClientName[client]);
    }
    else
    {
        int temp = RoundToCeil(GetConVarFloat(cvarRespawnCountdown) - (GetGameTime() - g_fDeadTime[client]));
        Format(NewName, sizeof(NewName), "[%ds] %s", temp, g_ClientName[client]);
    }

    SetClientName(client, NewName);
}

//========================================================================================
// STOCK
//========================================================================================

stock bool IsValidClient(int client, bool bAlive = false)    // 从sika那挪过来的常用函数
{
    return (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsClientSourceTV(client) && (!bAlive || IsPlayerAlive(client)));
}

stock int IsPlayerStuck(int client)    // 会挡住玩家移动的实体都会被返回
{
    float vecMin[3], vecMax[3], vecOrigin[3];

    GetClientMins(client, vecMin);
    GetClientMaxs(client, vecMax);
    GetClientAbsOrigin(client, vecOrigin);

    TR_TraceHullFilter(vecOrigin, vecOrigin, vecMin, vecMax, MASK_PLAYERSOLID, TraceEntityFilter_NotClient, client);    // MASK_PLAYERSOLID : everything that blocks player movement

    return TR_GetEntityIndex();
}

public bool TraceEntityFilter_NotClient(int entity, int contentsMask, int client)    // 只过滤掉client自己
{
    return entity != client;
}

public Action Timer_SetCollision(Handle timer, int client)
{
    if (IsValidClient(client, true))
    {
        if (IsPlayerStuck(client) > -1)
        {
            // PrintToChatAll("检测到玩家%d重叠", client);
            return Plugin_Continue;
        }
        else
        {
            // PrintToChatAll("恢复碰撞体积");
            SetEntProp(client, Prop_Data, "m_CollisionGroup", 5);    // COLLISION_GROUP_PLAYER 恢复碰撞体积（下一回合也会自动回复碰撞体积）
        }
    }

    KillTimer(timer);
    h_CollisionTask[client] = INVALID_HANDLE;

    return Plugin_Continue;
}

stock void ProtectClient(int client, float duration)
{
    if (IsValidClient(client))
    {
        int team = GetClientTeam(client);

        if (team == CS_TEAM_T)
        {
            SetEntProp(client, Prop_Data, "m_takedamage", 0, 1);
            SetEntityRenderMode(client, RENDER_TRANSADD);
            SetEntityRenderFx(client, RENDERFX_DISTORT);
            SetEntityRenderColor(client, 255, 0, 0, 120);

            CreateTimer(duration, Timer_RemoveProtection, client);
        }
        else if (team == CS_TEAM_CT)
        {
            SetEntProp(client, Prop_Data, "m_takedamage", 0, 1);
            SetEntityRenderMode(client, RENDER_TRANSADD);
            SetEntityRenderFx(client, RENDERFX_DISTORT);
            SetEntityRenderColor(client, 0, 0, 255, 120);

            CreateTimer(duration, Timer_RemoveProtection, client);
        }
    }
}

public Action Timer_RemoveProtection(Handle timer, int client)
{
    if (IsValidClient(client))
    {
        SetEntProp(client, Prop_Data, "m_takedamage", 2, 1);     // 取消无敌
        SetEntityRenderMode(client, RENDER_NORMAL);
        SetEntityRenderFx(client, RENDERFX_NONE);
        SetEntityRenderColor(client);                    // 恢复正常颜色
    }

    return Plugin_Continue;
}
