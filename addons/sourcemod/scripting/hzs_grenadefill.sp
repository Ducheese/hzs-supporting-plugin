//========================================================================================
// HZS Grenade Auto Fill
// 监听玩家购买行为：买任意一种雷，自动补齐该种雷到各自上限
// 购买检测走 cstrike 的 CS_OnBuyCommand 转发（此引擎无 item_purchase 事件）
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <cstrike>

#define VERSION "1.0"

ConVar g_cvHeMax;
ConVar g_cvFlashMax;
ConVar g_cvSmokeMax;

public Plugin myinfo =
{
    name = "HZS Grenade Auto Fill",
    author = "Ducheese",
    description = "买任意一种雷自动补齐该种雷到上限",
    version = VERSION,
    url = ""
};

public void OnPluginStart()
{
    CreateConVar("sm_hzs_grenadefill_version", VERSION, "插件版本", FCVAR_PROTECTED);

    g_cvHeMax = FindConVar("ammo_hegrenade_max");
    g_cvFlashMax = FindConVar("ammo_flashbang_max");
    g_cvSmokeMax = FindConVar("ammo_smokegrenade_max");
}

public Action CS_OnBuyCommand(int client, const char[] weapon)
{
    if (client < 1 || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Continue;

    char classname[32];
    ConVar cvar = null;
    int fallback = 1;

    if (StrEqual(weapon, "hegrenade"))
    {
        strcopy(classname, sizeof(classname), "weapon_hegrenade");
        cvar = g_cvHeMax;
    }
    else if (StrEqual(weapon, "flashbang"))
    {
        strcopy(classname, sizeof(classname), "weapon_flashbang");
        cvar = g_cvFlashMax;
        fallback = 2;
    }
    else if (StrEqual(weapon, "smokegrenade"))
    {
        strcopy(classname, sizeof(classname), "weapon_smokegrenade");
        cvar = g_cvSmokeMax;
    }
    else
    {
        return Plugin_Continue;
    }

    // 延迟一帧：CS_OnBuyCommand 触发时购买的雷尚未入包，等购买结算后再补齐
    DataPack dp = new DataPack();
    dp.WriteCell(GetClientUserId(client));
    dp.WriteString(classname);
    dp.WriteCell(view_as<int>(cvar));
    dp.WriteCell(fallback);
    CreateTimer(0.1, Timer_FillGrenade, dp, TIMER_FLAG_NO_MAPCHANGE);

    return Plugin_Continue;
}

public Action Timer_FillGrenade(Handle timer, DataPack dp)
{
    dp.Reset();
    int userid = dp.ReadCell();
    char classname[32];
    dp.ReadString(classname, sizeof(classname));
    ConVar cvar = view_as<ConVar>(dp.ReadCell());
    int fallback = dp.ReadCell();
    delete dp;

    int client = GetClientOfUserId(userid);
    if (client < 1 || !IsClientInGame(client) || !IsPlayerAlive(client))
        return Plugin_Stop;

    int cnt = CountGrenade(client, classname);
    int max = cvar != null ? GetConVarInt(cvar) : fallback;

    for (int i = cnt; i < max; i++)
        GivePlayerItem(client, classname);

    return Plugin_Stop;
}

int CountGrenade(int client, const char[] classname)
{
    int count = 0;
    int size = GetEntPropArraySize(client, Prop_Data, "m_hMyWeapons");

    for (int i = 0; i < size; i++)
    {
        int weapon = GetEntPropEnt(client, Prop_Data, "m_hMyWeapons", i);
        if (weapon == -1 || !IsValidEntity(weapon))
            continue;

        char cls[64];
        GetEntityClassname(weapon, cls, sizeof(cls));
        if (StrEqual(cls, classname))
            count++;
    }

    return count;
}
