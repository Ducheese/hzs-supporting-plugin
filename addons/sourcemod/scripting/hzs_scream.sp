//========================================================================================
// DEFINES
//========================================================================================

#define VERSION                "1.0"
#define GAMEUNITS_TO_METERS    0.01905 

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

float lastCallTime[MAXPLAYERS+1];

ConVar cvarScreamCooling;
ConVar cvarScreamRange;
ConVar cvarScreamDuration;
ConVar cvarScreamFilePath;

//========================================================================================
//========================================================================================

public Plugin myinfo =
{
    name = "HZS Scream",
    author = "Ducheese",
    description = "人类主动技能，可以发出尖叫吸引全场僵尸一段时间",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
}

public void OnPluginStart()
{
    CreateConVar("sm_hzs_scream_version", VERSION, "插件版本", FCVAR_PROTECTED);
    cvarScreamCooling = CreateConVar("sm_hzs_scream_cooling", "60.0", "尖叫技能冷却时间", FCVAR_NOTIFY, true, 0.0);
    cvarScreamRange = CreateConVar("sm_hzs_scream_range", "30.0", "尖叫技能的有效范围（单位：米）", FCVAR_NOTIFY, true, 1.0);
    cvarScreamDuration = CreateConVar("sm_hzs_scream_duration", "15.0", "尖叫技能吸引僵尸的持续时间", FCVAR_NOTIFY, true, 1.0);
    cvarScreamFilePath = CreateConVar("sm_hzs_scream_filepath", "player/waoh.wav", "尖叫音频路径（不带sound/）", FCVAR_NOTIFY);

    RegConsoleCmd("sm_scream", Cmd_ZombieCall);
}

public void OnMapStart()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        lastCallTime[i] = -9999.0;
    }
}

//========================================================================================
// HAN ZS HOOK
//========================================================================================

public void Han_OnZombieWin()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        lastCallTime[i] = -9999.0;
    }
}

public void Han_OnHumanWin()
{
    for (int i = 1; i <= MaxClients; i++)
    {
        lastCallTime[i] = -9999.0;
    }
}

//========================================================================================
// HOOK
//========================================================================================

public Action Cmd_ZombieCall(int client, any args)
{
    // 验证合法性
    if (client == 0)
    {
        if (!IsDedicatedServer() && IsClientInGame(1))
            client = 1;
        else
            return Plugin_Handled;
    }

    if (!IsPlayerAlive(client)) return Plugin_Handled;

    float fClientOrigin[3];
    GetClientEyePosition(client, fClientOrigin);

    // 触发技能
    if (GetGameTime() - lastCallTime[client] >= GetConVarFloat(cvarScreamCooling))     // 60秒冷却时间
    {
        float currentTime = GetGameTime();
        lastCallTime[client] = currentTime;

        // 镜头震动
        Shake(client, 30.0, 10.0, 3.0);

        // 吸引僵尸
        CreateZombieCall(client, GetConVarFloat(cvarScreamRange), GetConVarFloat(cvarScreamDuration));

        // 发出环境音
        char FilePath[PLATFORM_MAX_PATH];
        GetConVarString(cvarScreamFilePath, FilePath, sizeof(FilePath));
        PrecacheSound(FilePath, true);
        EmitAmbientSound(FilePath, fClientOrigin, client, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

        CPrintToChat(client, "{green}[华仔] {red}你使用了尖叫技能！僵尸将朝你袭来！");
    }
    else
    {
        CPrintToChat(client, "{green}[华仔] {red}技能冷却中...");
    }

    return Plugin_Continue;
}

//========================================================================================
// TIMER
//========================================================================================

//========================================================================================
// FUCTIONS
//========================================================================================

void CreateZombieCall(int client, float dist, float duration)      // client主动吸引范围内僵尸
{
    float fClientOrigin[3], fTargetOrigin[3];
    GetEntPropVector(client, Prop_Send, "m_vecOrigin", fClientOrigin);

    float fTargetDistance;

    int count = Han_GetZombieCount();

    for (int i = 0; i < count; i++)
    {
        int zombie = Han_GetZombieByIndex(i);

        if (Han_IsZombie(zombie))
        {
            if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
            {
                continue;
            }

            GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", fTargetOrigin);
            fTargetDistance = GetVectorDistance(fClientOrigin, fTargetOrigin);

            if (fTargetDistance*GAMEUNITS_TO_METERS > dist)
            {
                continue;
            }

            Han_SetZombieTarget(zombie, client, duration);
        }
    }
}

//========================================================================================
// STOCK
//========================================================================================

stock bool IsValidClient(int client, bool bAlive = false)    // 从sika那挪过来的常用函数
{
    return (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsClientSourceTV(client) && (!bAlive || IsPlayerAlive(client)));
}

stock void Shake(int client, float flAmplitude, float flFrequency, float flDuration)    // 只对活着的玩家有效
{
    Handle hBf = StartMessageOne("Shake", client);

    if (hBf != INVALID_HANDLE)
    {
        BfWriteByte(hBf, 0);
        BfWriteFloat(hBf, flAmplitude);
        BfWriteFloat(hBf, flFrequency);
        BfWriteFloat(hBf, flDuration);
        EndMessage();
    }
}