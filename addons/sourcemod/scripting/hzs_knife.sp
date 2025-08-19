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

ConVar cvarKnifeDamage;
ConVar cvarKnifeRange;

//========================================================================================
//========================================================================================

public Plugin myinfo =
{
    name = "HZS Knife",
    author = "Ducheese",
    description = "人类被动技能，让近战武器具有范围伤害",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
}

public void OnPluginStart()
{
    CreateConVar("sm_hzs_knife_version", VERSION, "插件版本", FCVAR_PROTECTED);
    cvarKnifeDamage = CreateConVar("sm_hzs_knife_damage", "100", "近战武器范围伤害值", FCVAR_NOTIFY, true, 1.0);
    cvarKnifeRange = CreateConVar("sm_hzs_knife_range", "2.0", "近战武器有效攻击范围（单位：米）", FCVAR_NOTIFY, true, 0.0);

    HookEvent("weapon_fire", Event_WeaponFire);
}

//========================================================================================
// HOOK
//========================================================================================

public void Event_WeaponFire(Handle event, const char[] name, bool dontBroadcast)
{
    int attacker = GetClientOfUserId(GetEventInt(event, "userid"));

    if (!CheckEquipKnife(attacker)) return;
    
    // 数组准备
    float fClientOrigin[3], fTargetOrigin[3];
    GetEntPropVector(attacker, Prop_Send, "m_vecOrigin", fClientOrigin);

    float fTargetDistance;

    // 范围伤害
    int count = Han_GetZombieCount();

    for (int i = 0; i < count; i++)
    {
        int zombie = Han_GetZombieByIndex(i);

        if (Han_IsZombie(zombie))
        {
            // NPC合法验证
            if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
            {
                continue;
            }

            // 距离验证
            GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", fTargetOrigin);
            fTargetDistance = GetVectorDistance(fClientOrigin, fTargetOrigin);

            if (fTargetDistance*GAMEUNITS_TO_METERS > GetConVarFloat(cvarKnifeRange))   // 范围以外作用不到
            {
                continue;
            }

            // 方向验证
            if (!IsTargetForward(attacker, zombie))
            {
                continue;
            }

            Han_SafeDamageZombie(attacker, zombie, GetConVarInt(cvarKnifeDamage));    // 对僵尸生成伤害
        }
    }
}

//========================================================================================
// FUCTIONS
//========================================================================================

bool CheckEquipKnife(int client)
{
    int weapon = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
    
    if (weapon != -1 && weapon == GetPlayerWeaponSlot(client, 2))
    {
        return true;
    }

    return false;
}

bool IsTargetForward(int client, int target)
{
    float fClientAngles[3];
    float fClientPosition[3];
    float fTargetPosition[3];
    float fTempPoints[3];
    float fTempAngles[3];

    GetClientEyeAngles(client, fClientAngles);
    GetClientAbsOrigin(client, fClientPosition);
    GetEntPropVector(target, Prop_Send, "m_vecOrigin", fTargetPosition);

    // Angles from origin
    MakeVectorFromPoints(fClientPosition, fTargetPosition, fTempPoints);
    GetVectorAngles(fTempPoints, fTempAngles);

    // Differenz&x
    float fDiffz = fClientAngles[1] - fTempAngles[1];     // （z管水平方向转动）眼睛看的方向，与连线方向的夹角，欧拉角里叫做“偏航角”

    // Correct it
    if (fDiffz < -180)
        fDiffz = 360 + fDiffz;     // 调整到在-180到+180之间

    if (fDiffz > 180)
        fDiffz = 360 - fDiffz;

    if (fDiffz >= -67.5 && fDiffz <= 67.5)
    {
        return true;
    }

    return false;
}

//========================================================================================
// STOCK
//========================================================================================

stock bool IsValidClient(int client, bool bAlive = false)    // 从sika那挪过来的常用函数（要求InGame）
{
    return (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsClientSourceTV(client) && (!bAlive || IsPlayerAlive(client)));
}