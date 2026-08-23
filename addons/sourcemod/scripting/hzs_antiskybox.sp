#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>

#define VERSION "1.0"

// 64 位 Steam 专服天空盒表面序号为 77（default_silent）
#define SKYBOX_SURFPROP 77

ConVar g_cvLimitTime;
ConVar g_cvCheckInterval;

float g_fSkyStandingTime[MAXPLAYERS + 1];

public Plugin myinfo =
{
    name = "HZS Anti-Skybox",
    author = "Ducheese",
    description = "防止玩家踩在天空盒上（超时处死）",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    g_cvLimitTime = CreateConVar("sm_hzs_antiskybox_time", "5.0", "允许踩在天空盒上的最大时间（秒），超时处死", FCVAR_NOTIFY, true, 1.0, true, 60.0);
    g_cvCheckInterval = CreateConVar("sm_hzs_antiskybox_interval", "0.5", "检测时间间隔（秒）", FCVAR_NOTIFY, true, 0.1, true, 2.0);

    CreateTimer(0.5, Timer_CheckSkybox, _, TIMER_REPEAT);

    HookEvent("round_start", Event_RoundStart, EventHookMode_Post);
    HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);
}

public void OnClientPutInServer(int client)
{
    g_fSkyStandingTime[client] = 0.0;
}

public void OnClientDisconnect(int client)
{
    g_fSkyStandingTime[client] = 0.0;
}

public void Event_RoundStart(Event event, const char[] name, bool dontBroadcast)
{
    for (int i = 1; i <= MaxClients; i++)
    {
        g_fSkyStandingTime[i] = 0.0;
    }
}

public void Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (client > 0 && client <= MaxClients)
    {
        g_fSkyStandingTime[client] = 0.0;
    }
}

public Action Timer_CheckSkybox(Handle timer)
{
    float interval = g_cvCheckInterval.FloatValue;
    float limit = g_cvLimitTime.FloatValue;

    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsClientInGame(client) || !IsPlayerAlive(client) || IsFakeClient(client))
        {
            g_fSkyStandingTime[client] = 0.0;
            continue;
        }

        if (IsStandingOnSkybox(client))
        {
            g_fSkyStandingTime[client] += interval;

            if (g_fSkyStandingTime[client] >= limit)
            {
                g_fSkyStandingTime[client] = 0.0;
                ForcePlayerSuicide(client);
                PrintToChatAll("\x04[提示]\x01 玩家 \x03%N \x01因踩在天空盒上超过 \x03%.0f\x01 秒已被处死。", client, limit);
            }
            else
            {
                float remain = limit - g_fSkyStandingTime[client];
                PrintCenterText(client, "你正踩在天空盒上！\n将在 %.1f 秒后被处死，请立即离开！", remain);
            }
        }
        else
        {
            g_fSkyStandingTime[client] = 0.0;
        }
    }

    return Plugin_Continue;
}

/**
 * 精准检测玩家是否“踩在”天空盒表面上：
 * 1. 玩家必须处于着地状态（FL_ONGROUND）；
 * 2. 仅向脚底下方探测 15 距离的有限线段（RayType_EndPoint），不打无限射线；
 * 3. 必须真实命中且命中面的表面属性为 64 位专服天空盒序号（77）。
 */
bool IsStandingOnSkybox(int client)
{
    // 观察者与 noclip 状态不判定
    MoveType moveType = GetEntityMoveType(client);
    if (moveType == MOVETYPE_NOCLIP || moveType == MOVETYPE_OBSERVER || moveType == MOVETYPE_NONE)
        return false;

    // 必须处于着地状态（踩在地面上）
    int flags = GetEntityFlags(client);
    if (!(flags & FL_ONGROUND))
        return false;

    float startPos[3], endPos[3];
    GetEntPropVector(client, Prop_Send, "m_vecOrigin", startPos);
    endPos[0] = startPos[0];
    endPos[1] = startPos[1];
    endPos[2] = startPos[2] - 15.0;

    // 1. 脚底中心射线检测
    Handle trace = TR_TraceRayFilterEx(startPos, endPos, MASK_SOLID, RayType_EndPoint, TraceEntityFilter_World);
    bool bStandingOnSky = false;

    if (TR_DidHit(trace))
    {
        int surfProps = TR_GetSurfaceProps(trace);
        if (surfProps == SKYBOX_SURFPROP)
        {
            bStandingOnSky = true;
        }
    }
    delete trace;

    // 2. 边缘 Hull 碰撞体检测（防止站在天空盒边缘时中心射线打空）
    if (!bStandingOnSky)
    {
        trace = TR_TraceHullFilterEx(startPos, endPos, view_as<float>({-16.0, -16.0, 0.0}), view_as<float>({16.0, 16.0, 10.0}), MASK_SOLID, TraceEntityFilter_World);
        if (TR_DidHit(trace))
        {
            int surfProps = TR_GetSurfaceProps(trace);
            if (surfProps == SKYBOX_SURFPROP)
            {
                bStandingOnSky = true;
            }
        }
        delete trace;
    }

    return bStandingOnSky;
}

public bool TraceEntityFilter_World(int entity, int mask, any self)
{
    return (entity == 0);
}
