//========================================================================================
// INCLUDES
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <dhooks>

#define GAMEDATA "hzs_botfollowfix.gamedata"

//========================================================================================
// HANDLES & VARIABLES
//========================================================================================

Handle g_hStopFollowingDetour;
Handle g_hFollowCall;
int g_iBotLeaderOffset;
bool g_bIs64Bit;

//========================================================================================
//========================================================================================

public Plugin myinfo =
{
    name = "HZS Bot Follow Fix",
    author = "Ducheese",
    description = "解除BOT跟随的自动中断与阵营限制，Follow Me无线电强制全员跟随",
    version = "1.0",
    url = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    PrepOffsets();
    PrepStopFollowingDetour();
    PrepFollowSDKCall();

    AddCommandListener(Command_Radio, "followme");
}

//========================================================================================
// Offsets
//========================================================================================

void PrepOffsets()
{
    Handle gc = LoadGameConfigFile(GAMEDATA);
    if (gc == INVALID_HANDLE)
        SetFailState("[BotFollowFix] Failed to load gamedata for offsets");

    g_iBotLeaderOffset = GameConfGetOffset(gc, "CCSBot_m_leader");
    g_bIs64Bit = GameConfGetOffset(gc, "IsWin64") == 1;
    delete gc;
}

//========================================================================================
// DHook: StopFollowing — 拦截无聊打断，放行 leader 死亡
//========================================================================================

void PrepStopFollowingDetour()
{
    Handle gc = LoadGameConfigFile(GAMEDATA);
    if (gc == INVALID_HANDLE)
        SetFailState("[BotFollowFix] Failed to load gamedata for StopFollowing detour");

    g_hStopFollowingDetour = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_CBaseEntity);
    DHookSetFromConf(g_hStopFollowingDetour, gc, SDKConf_Signature, "CCSBot_StopFollowing");
    if (g_bIs64Bit)
        DHookAddParam(g_hStopFollowingDetour, HookParamType_CBaseEntity);     // 64-bit 需要显式声明参数，否则进图闪退
    if (!DHookEnableDetour(g_hStopFollowingDetour, false, OnStopFollowing_Pre))
        SetFailState("[BotFollowFix] Failed to enable StopFollowing detour");

    delete gc;
}

//========================================================================================
// SDKCall: Follow — 强制 BOT 跟随指定玩家
//========================================================================================

void PrepFollowSDKCall()
{
    Handle gc = LoadGameConfigFile(GAMEDATA);
    if (gc == INVALID_HANDLE)
        SetFailState("[BotFollowFix] Failed to load gamedata for Follow SDKCall");

    StartPrepSDKCall(SDKCall_Entity);
    PrepSDKCall_SetFromConf(gc, SDKConf_Signature, "CCSBot_Follow");
    PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
    g_hFollowCall = EndPrepSDKCall();
    if (g_hFollowCall == INVALID_HANDLE)
        SetFailState("[BotFollowFix] Failed to prep Follow SDKCall");

    delete gc;
}

//========================================================================================
// DHook: StopFollowing — 拦截无聊打断，放行 leader 死亡
//========================================================================================

public MRESReturn OnStopFollowing_Pre(int bot)
{
    // 读 m_leader：活着 → bored 打断（拦截），死了/NULL → 正常停止（放行）
    int leader = GetEntDataEnt2(bot, g_iBotLeaderOffset);

    if (leader > 0 && IsClientInGame(leader) && IsPlayerAlive(leader))
        return MRES_Supercede;

    return MRES_Ignored;
}

//========================================================================================
// 无线电: Follow Me — 人类发出后强制所有 BOT 跟随（无视队伍）
//========================================================================================

public Action Command_Radio(int client, const char[] command, int argc)
{
    if (!IsClientInGame(client) || !IsPlayerAlive(client) || IsFakeClient(client))
        return Plugin_Continue;

    for (int i = 1; i <= MaxClients; i++)
    {
        if (!IsClientInGame(i) || !IsFakeClient(i) || !IsPlayerAlive(i))
            continue;

        SDKCall(g_hFollowCall, i, client);
    }

    return Plugin_Continue;
}
