//========================================================================================
// hzs_infiniteammo.sp
//
// 原理：
//   周期性把所有玩家的全部弹药类型（m_iAmmo[0..31]，CS:S 共 32 种）补到 9999，
//   当前武器弹匣（m_iClip1）也补到 9999，无需换弹。
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>

#define AMMO_TYPES 32        // CS:S m_iAmmo 数组长度（MAX_AMMO_TYPES）
#define AMMO_MAX 9999

public Plugin myinfo =
{
    name        = "HZS Infinite Ammo",
    author      = "Ducheese",
    description = "玩家所有种类子弹数补到9999，弹匣不空",
    version     = "1.0",
    url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    HookEvent("player_spawn", Event_PlayerSpawn);
}

public void Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (IsClientInGame(client) && IsPlayerAlive(client))
        RefillAmmo(client);
}

// 所有弹药类型补到 9999，当前武器弹匣也补满
void RefillAmmo(int client)
{
    for (int i = 0; i < AMMO_TYPES; i++)
        SetEntProp(client, Prop_Send, "m_iAmmo", AMMO_MAX, _, i);

    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (IsValidEntity(weapon))
        SetEntProp(weapon, Prop_Send, "m_iClip1", AMMO_MAX);
}
