//========================================================================================
// hzs_sameteam.sp
//
// 补丁原理：
//   DHook CBaseEntity::InSameTeam，无条件返回 true，
//   使所有实体（含 BOT）相互视为同阵营 → CT/T 互不攻击。
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <dhooks>

#define GAMEDATA "hzs_sameteam.gamedata"

Handle g_hInSameTeamDetour;

public Plugin myinfo =
{
    name        = "HZS SameTeam",
    author      = "Ducheese",
    description = "DHook InSameTeam 强制返回 true，BOT 不再攻击人类",
    version     = "1.0",
    url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    PrepInSameTeamDetour();
}

//========================================================================================
// DHook: InSameTeam — 无条件视为同阵营
//========================================================================================

void PrepInSameTeamDetour()
{
    Handle gc = LoadGameConfigFile(GAMEDATA);
    if (gc == INVALID_HANDLE)
        SetFailState("[SameTeam] 无法加载 gamedata");

    g_hInSameTeamDetour = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Bool, ThisPointer_CBaseEntity);
    DHookSetFromConf(g_hInSameTeamDetour, gc, SDKConf_Signature, "CBaseEntity_InSameTeam");
    DHookAddParam(g_hInSameTeamDetour, HookParamType_CBaseEntity);
    if (!DHookEnableDetour(g_hInSameTeamDetour, false, OnInSameTeam_Pre))
        SetFailState("[SameTeam] 无法启用 InSameTeam detour");

    delete gc;
}

public MRESReturn OnInSameTeam_Pre(int entity, Handle hReturn, Handle hParams)
{
    DHookSetReturn(hReturn, true);
    return MRES_Supercede;
}
