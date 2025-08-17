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
#include <sdkhooks>
#include <cstrike>
#include <morecolors>
#include <HanZombieScenarioAPI>

//========================================================================================
// HANDLES & VARIABLES
//========================================================================================

int g_iLivesRemaining[MAXPLAYERS+1];
bool g_bIsWaitingRespawn[MAXPLAYERS+1];
float g_fDeadTime[MAXPLAYERS+1];
char g_ClientName[MAXPLAYERS+1][32];

Handle SpawnPoint;
Handle TimerTask[MAXPLAYERS+1] = {INVALID_HANDLE,...};

ConVar cvarRespawnLives;
ConVar cvarRespawnCountdown;
ConVar cvarRespawnProtect;

//========================================================================================
//========================================================================================

public Plugin myinfo =
{
    name = "HZS BOT Fake Respawn",
    author = "Ducheese",
    description = "用伪死亡和伪复活来解决H-AN大灾变模式插件BOT不能复活的问题",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
}

public void OnPluginStart()
{
    CreateConVar("sm_hzs_botfakerespawn_version", VERSION, "插件版本", FCVAR_PROTECTED);
    cvarRespawnLives = CreateConVar("sm_hzs_botfakerespawn_lives", "3", "BOT生命总数", FCVAR_NOTIFY, true, 1.0);
    cvarRespawnCountdown = CreateConVar("sm_hzs_botfakerespawn_countdown", "15.0", "BOT伪复活倒计时", FCVAR_NOTIFY, true, 0.0);
    cvarRespawnProtect = CreateConVar("sm_hzs_botfakerespawn_protect", "3.0", "BOT伪复活无敌时间", FCVAR_NOTIFY, true, 0.0);

    HookEvent("player_spawn", Event_PlayerSpawn);                          // 真重生时，所有数组都初始化
    HookEvent("round_freeze_end", Event_RoundFreezeEnd);                   // 为了配合round exec插件，即便晚于round start加载插件，也能顺利获取地图出生点实体信息
    HookUserMessage(GetUserMessageId("SayText2"), Hook_SayText2, true);    // 避免改名信息刷屏

    SpawnPoint = CreateArray(1);           
}

public void OnMapStart()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        if (TimerTask[i] != INVALID_HANDLE)
        {
            CloseHandle(TimerTask[i]);
            TimerTask[i] = INVALID_HANDLE;
        }
    }  
}

public void OnClientPutInServer(int client)
{
    GetClientName(client, g_ClientName[client], 32);
    SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);
}

public void OnClientDisconnect_Post(int client)
{
    g_ClientName[client] = NULL_STRING;
    SDKUnhook(client, SDKHook_OnTakeDamage, OnTakeDamage);
}

//========================================================================================
// HOOK
//========================================================================================

public void Event_RoundFreezeEnd(Handle event, const char[] name, bool dontBroadcast)
{
    ClearArray(SpawnPoint);

    int entity;

    while ((entity = FindEntityByClassname(entity, "info_player_counterterrorist")) != -1)      // 伪复活只传送CT复活点
    {
        PushArrayCell(SpawnPoint, EntIndexToEntRef(entity));
    }

    // while ((entity = FindEntityByClassname(entity, "info_player_terrorist")) != -1) 
    // {
    //     PushArrayCell(SpawnPoint, EntIndexToEntRef(entity));
    // }
}

public void Event_PlayerSpawn(Handle event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(GetEventInt(event, "userid"));

    if (IsFakeClient(client))
    {
        // 数组初始化
        g_bIsWaitingRespawn[client] = false;
        g_iLivesRemaining[client] = GetConVarInt(cvarRespawnLives);
        
        // 刚复活时的前缀更新
        UpdateNamePrefix(client);
    }
}

public Action OnTakeDamage(int victim, int &attacker, int &inflictor, float &damage, int &damagetype)
{
    if (GetClientHealth(victim) > damage || g_bIsWaitingRespawn[victim] || !IsFakeClient(victim) || g_iLivesRemaining[victim] <= 0) return Plugin_Continue;

    if (!IsValidClient(attacker, false))     //  在0 0 0点，同为人类可能因为一些缘故相互误伤，同时这个damage可能被误判，所以要验证attacker
    {
        // int weapon = GetEntPropEnt(, Prop_Data, "m_hActiveWeapon");
        // char classname[32];
        // GetEntityClassname(weapon, classname, sizeof(classname));
        // PrintToChatAll("凶手：%N %s, 血量：%d-%f=%f", attacker, classname, GetClientHealth(victim), damage, GetClientHealth(victim) - damage);

        g_iLivesRemaining[victim]--;

        if (g_iLivesRemaining[victim] > 0)  // 减完之后还剩
        {
            if (Han_IsZombie(attacker) && IsValidEntity(attacker))
            {
                char ZombieName[32];
                Han_GetZombieName(attacker, ZombieName, sizeof(ZombieName));
                CPrintToChatAll("{green}[华仔] {red}%s被%s杀死了！他还剩%d条命", g_ClientName[victim], ZombieName, g_iLivesRemaining[victim]);
            }
            else
            {
                CPrintToChatAll("{green}[华仔] {red}%s死了！他还剩%d条命", g_ClientName[victim], g_iLivesRemaining[victim]);
            }

            g_bIsWaitingRespawn[victim] = true;
            g_fDeadTime[victim] = GetGameTime();

            TeleportEntity(victim, view_as<float>({0.0, 0.0, 0.0}), view_as<float>({0.0, 0.0, 0.0}), view_as<float>({0.0, 0.0, 0.0}));      // 怕提前死了，先传送了

            damage = 0.0;
            return Plugin_Changed;
        }
        else
        {
            if (Han_IsZombie(attacker) && IsValidEntity(attacker))
            {
                char ZombieName[32];
                Han_GetZombieName(attacker, ZombieName, sizeof(ZombieName));
                CPrintToChatAll("{green}[华仔] {red}%s被%s杀死了！彻底死透了！", g_ClientName[victim], ZombieName);
            }
            else
            {
                CPrintToChatAll("{green}[华仔] {red}%s死了！彻底死透了！", g_ClientName[victim]);
            }

            UpdateNamePrefix(victim);   // 彻底死了的也会更新前缀
        }
    }

    return Plugin_Continue;
}

public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{
    if (g_bIsWaitingRespawn[client] && IsFakeClient(client))
    {
        if (GetGameTime() - g_fDeadTime[client] > GetConVarFloat(cvarRespawnCountdown))
        {
            SetClientHealth(client, 100);
            FakeClientCommand(client, "buy vesthelm");
            FakeClientCommand(client, "buy vest");

            TeleportRespawnPoint(client);

            g_bIsWaitingRespawn[client] = false;
        }
        else
        {
            TeleportEntity(client, view_as<float>({0.0, 0.0, 0.0}), view_as<float>({0.0, 0.0, 0.0}), view_as<float>({0.0, 0.0, 0.0}));
        }

        UpdateNamePrefix(client);     // 复活倒计时在名字上实时更新
    }

    return Plugin_Continue;
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

void UpdateNamePrefix(int client)
{
    char NewName[64];

    if (!g_bIsWaitingRespawn[client])
        Format(NewName, sizeof(NewName), "[❤x%d] %s", g_iLivesRemaining[client], g_ClientName[client]);       // 并不会乱码，不赖
    else
    {
        int temp = RoundToZero(GetConVarFloat(cvarRespawnCountdown) - (GetGameTime() - g_fDeadTime[client]));
        Format(NewName, sizeof(NewName), "[%ds] %s", temp, g_ClientName[client]);
    }

    SetClientName(client, NewName);
}

void TeleportRespawnPoint(int client)
{
    int rand = GetRandomInt(0, GetArraySize(SpawnPoint)-1);    // 随机数

    int ref = GetArrayCell(SpawnPoint, rand, 0);

    if (ref != INVALID_ENT_REFERENCE)
    {
        float Pos[3], Ang[3];
        GetEntPropVector(ref, Prop_Send, "m_vecOrigin", Pos);
        GetEntPropVector(ref, Prop_Send, "m_angRotation", Ang);
        TeleportEntity(client, Pos, Ang, NULL_VECTOR);

        // 取消碰撞然后延时恢复
        SetEntProp(client, Prop_Data, "m_CollisionGroup", 2);                                                           // COLLISION_GROUP_DEBRIS_TRIGGER 消除碰撞体积
        TimerTask[client] = CreateTimer(1.0, Timer_SetCollision, client, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);       // 每秒进行一次碰撞检测

        // 无敌时间
        ProtectClient(client, GetConVarFloat(cvarRespawnProtect));
    }
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
    TimerTask[client] = INVALID_HANDLE;

    return Plugin_Continue;
}

//========================================================================================
// STOCK
//========================================================================================

stock bool IsValidClient(int client, bool bAlive = false)    // 从sika那挪过来的常用函数
{
    return (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsClientSourceTV(client) && (!bAlive || IsPlayerAlive(client)));
}

stock void SetClientHealth(int client, int health)
{
    SetEntProp(client, Prop_Send, "m_iHealth", health, 1);
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