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

#include "HZSZombieSkill/global"       // 全局变量定义
#include "HZSZombieSkill/event"        // 僵尸事件
#include "HZSZombieSkill/helper"       // 功能函数
#include "HZSZombieSkill/zskill"       // 特殊僵尸技能函数
#include "HZSZombieSkill/bskill"       // BOSS技能函数

//========================================================================================
//========================================================================================

public Plugin myinfo =
{
    name = "HZS Zombie Skill",
    author = "Ducheese",
    description = "实现NPC僵尸技能和BOSS技能",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
}

public void OnPluginStart()
{
    // 获取NPC在追随什么目标
    g_LeaderOffset = FindSendPropInfo("CHostage", "m_leader");

    // 玩家相关数组初始化
    for (int i = 1; i <= MaxClients; i++)
    {
        g_iZombieCall[i] = g_iZombiePull[i] = -1;
        g_bIsStuck[i] = g_bIsDisorder[i] = false;

        g_iGrappledBy[i] = -1;
        g_iUsePressCount[i] = 0;
        g_iLastButtons[i] = 0;
    }

    // 贴图缓存
    // PrecacheModel("particle/particle_smokegrenade.vmt", true);           // 迷雾僵尸所用的烟雾贴图，不用预缓存
    PrecacheModel("models/heavyzombietrap/zombitrap.mdl", true);         // 憎恶屠夫鬼手陷阱

    // 音频缓存
    PrecacheSound(SFX_SMOKE1, true);
    PrecacheSound(SFX_SMOKE2, true);
    PrecacheSound(SFX_EXPLODE1, true);
    PrecacheSound(SFX_EXPLODE2, true);
    PrecacheSound(SFX_KNOCKBACK, true);
    PrecacheSound(SFX_STUCK, true);
    PrecacheSound(SFX_DISORDER, true);

    PrecacheSound(SFX_PULL, true);
    PrecacheSound(SFX_BREATH_PULL, true);
    PrecacheSound(SFX_SMASH, true);
    PrecacheSound(SFX_SWING, true);
    PrecacheSound(SFX_ZOMBIECALL, true);
    PrecacheSound(SFX_BREATH_HEAL_ING, true);
    PrecacheSound(SFX_BREATH_HEAL_FULL, true);
    PrecacheSound(SFX_BREATH_HEAL_FAIL, true);
    PrecacheSound(SFX_FLY, true);
    PrecacheSound(SFX_POISON, true);

    PrecacheSound(SFX_CHARGE_HOWL, true);
    PrecacheSound(SFX_CHARGE_SHAKE, true);
    PrecacheSound(SFX_GRAPPLE_HURT, true);
}

public void OnMapStart()
{
    // 为方便round exec插件加载/卸载，因此初始化就不写这了
}

public void OnClientPutInServer(int client)
{
    GetClientName(client, g_ClientName[client], 32);
}

public void OnClientDisconnect_Post(int client)
{
    g_ClientName[client] = NULL_STRING;
}

//========================================================================================
// HOOK 玩家输入
//========================================================================================

public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{
    // 干扰人类移动的僵尸技能
    if (IsValidClient(client, true))
    {
        if (g_bIsStuck[client])
        {
            vel[0] = vel[1] = vel[2] = 0.0;

            if (buttons & IN_JUMP)
                buttons &= ~IN_JUMP;      // 禁止跳跃
        }
        else if (g_bIsDisorder[client])
        {
            vel[0] = -vel[0];
            vel[1] = -vel[1];
            vel[2] = -vel[2];
        }
        else if (g_iZombiePull[client] != -1 && !IsZeroPostion(client))                              // 在0 0 0点等待复活的人类不会被吸
        {
            int zombie = g_iZombiePull[client];

            if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
                return Plugin_Continue;

            float pos[3];
            GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

            CreateKnockback(pos, client, view_as<float>({-PullPower, -PullPower, -PullPower}));      // 击退的反方向就是吸力

            EmitSoundToClient(client, SFX_PULL, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        }
        else if (g_iGrappledBy[client] != -1)
        {
            // 擒抱状态下，玩家无法移动和攻击
            vel[0] = vel[1] = vel[2] = 0.0;

            // 全都做不了
            buttons &= ~IN_ATTACK;
            buttons &= ~IN_ATTACK2;
            buttons &= ~IN_RELOAD;
            buttons &= ~IN_JUMP;

            // 检测E键连打
            if ((buttons & IN_USE) && !(g_iLastButtons[client] & IN_USE))   // 毕竟有松开键位才算按一次
            {
                g_iUsePressCount[client]++;
            }
        }

        g_iLastButtons[client] = buttons;   // 虽然多个数组，但这样写确实简洁
    }

    // 呼唤僵尸攻击人类，死后处理（草，忘了真死的情况）
    if (g_iZombieCall[client] != -1 && (IsZeroPostion(client) || !IsPlayerAlive(client)))
    {
        g_iZombieCall[client] = -1;            // 防止这里重复执行

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

                Han_UnlockZombie(zombie);      // 解除所有僵尸的强制目标
            }
        }
    }

    return Plugin_Continue;
}