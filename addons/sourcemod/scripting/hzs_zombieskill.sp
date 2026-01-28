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
#include "HZSZombieSkill/angela"       // BOSS技能函数（安哥拉）
#include "HZSZombieSkill/pangzi"       // BOSS技能函数（巨型狂暴形态僵尸）
#include "HZSZombieSkill/yixing"       // BOSS技能函数（异形斗兽）

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
    g_iLeaderOffset = FindSendPropInfo("CHostage", "m_leader");

    InitHumanState();
    InitModelCache();
    InitSoundCache();
}

public void OnMapStart()
{
    // 为方便round exec插件加载/卸载，因此初始化就不写这了
}

public void OnClientPutInServer(int client)
{
    GetClientName(client, g_ClientName[client], 32);   // 伪复活插件会更换玩家名字，所以要提前保存
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
    if (IsHumanAlive(client))
    {
        if (g_bIsStuck[client])                 // if分支有优先级，如果被定住了，就无法被女巫和吸力影响
        {
            vel[0] = vel[1] = vel[2] = 0.0;     // 其实可以不管垂直速度，禁止跳即可

            buttons &= ~IN_JUMP;
        }
        else if (g_bIsInvert[client])           // 应当增加一个视觉反馈（如果能做一些视觉扭曲就更好了）
        {
            vel[0] = -vel[0];
            vel[1] = -vel[1];
            vel[2] = -vel[2];

            SetHudTextParams(0.50, 0.45, 0.1, 255, 0, 0, 255);
            ShowHudText(client, -1, "  方向键取反了!!!");
        }
        else if (g_iZombiePull[client] != -1)   // 在0 0 0点等待复活的人类不会被吸
        {
            int zombie = g_iZombiePull[client];

            if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
                return Plugin_Continue;

            CreateKnockback(zombie, client, view_as<float>({-ANGELA_PULL_POWER, -ANGELA_PULL_POWER, -ANGELA_PULL_POWER}));      // 击退的反方向就是吸力

            EmitSoundToClient(client, SFX_PULL1, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);    // 这样写，音效比较有魄力
        }
        else if (g_bIsGrappled[client])
        {
            // 擒抱状态下，玩家无法移动和攻击
            vel[0] = vel[1] = vel[2] = 0.0;

            // 全都做不了
            buttons &= ~IN_ATTACK;
            buttons &= ~IN_ATTACK2;
            buttons &= ~IN_RELOAD;
            buttons &= ~IN_JUMP;

            // 检测E键连打
            if (!IsFakeClient(client))
            {
                if ((buttons & IN_USE) && !(g_iLastButtons[client] & IN_USE))   // 毕竟有松开键位才算按一次
                {
                    g_iUsePressCount[client]++;
                }

                SetHudTextParams(0.50, 0.45, 0.1, 255, 0, 0, 255);
                ShowHudText(client, -1, "  快连按E键挣脱!!!");
            }
            else
            {
                if (GetRandomInt(1, 20) == 1)   // 大概率会被抱上两秒，这概率还可以
                {
                    g_iUsePressCount[client]++;
                    // PrintToChatAll("按键计数：%d", g_iUsePressCount[client]);
                }
            }
        }

        g_iLastButtons[client] = buttons;       // 虽然多了个数组，但这样写确实简洁
    }
    else
    {
        // 呼唤僵尸攻击人类，死后处理（草，忘了真死的情况）
        if (g_iZombieCall[client] != -1)
        {
            g_iZombieCall[client] = -1;         // 防止这里重复执行

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

                    Han_UnlockZombie(zombie);   // 解除所有僵尸的强制目标（这里对多个安哥拉的考虑欠佳）
                }
            }
        }   
    }

    return Plugin_Continue;
}

//========================================================================================
//========================================================================================

void InitHumanState()
{
    // 玩家相关数组初始化（不爽的位置）
    for (int i = 1; i <= MaxClients; i++)
    {
        g_bIsStuck[i] = g_bIsInvert[i] = false;

        g_iZombieCall[i] = g_iZombiePull[i] = -1;

        g_bIsGrappled[i] = false;

        g_iUsePressCount[i] = 0;
        g_iLastButtons[i] = -1;

        // 这两个没有必要初始化，都是在使用前赋值的
        // g_iViewControl
        // g_flEscapePos[]

        ClearGrappleHandles(i);
    }
}

void InitModelCache()
{
    // 模型预缓存
    g_iBeamSprite = PrecacheModel("materials/sprites/physbeam.vmt", true);	       // 辅助线
    g_iZombieTrap = PrecacheModel("models/heavyzombietrap/zombitrap.mdl", true);   // 鬼手陷阱
}

void InitSoundCache()
{
    // 音频预缓存
    PrecacheSound(SFX_SMOKE1, true);
    PrecacheSound(SFX_SMOKE2, true);
    PrecacheSound(SFX_EXPLODE1, true);
    PrecacheSound(SFX_EXPLODE2, true);
    PrecacheSound(SFX_KNOCKBACK, true);
    PrecacheSound(SFX_BUTCHER, true);
    PrecacheSound(SFX_WITCH, true);

    // 音频预缓存（BOSS安哥拉）
    PrecacheSound(SFX_CALL, true);
    PrecacheSound(SFX_PULL1, true);
    PrecacheSound(SFX_PULL2, true);
    PrecacheSound(SFX_SMASH, true);
    PrecacheSound(SFX_SWING, true);
    PrecacheSound(SFX_HEAL1, true);
    PrecacheSound(SFX_HEAL2, true);
    PrecacheSound(SFX_HEAL3, true);
    PrecacheSound(SFX_FLY, true);
    PrecacheSound(SFX_POISON, true);

    // 音频预缓存（BOSS巨型狂暴形态僵尸）
    PrecacheSound(SFX_CHARGE1, true);
    PrecacheSound(SFX_CHARGE2, true);
    PrecacheSound(SFX_GRAPPLE1, true);
    PrecacheSound(SFX_GRAPPLE2, true);

    // 音频预缓存（BOSS异形斗兽）
    PrecacheSound(SFX_DASH1, true);
    PrecacheSound(SFX_DASH2, true);
    PrecacheSound(SFX_DASH3, true);
}