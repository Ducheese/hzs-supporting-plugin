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
#include <dhooks>

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

    HookEvent("round_start", Event_RoundStart);

    PrepWitchCCDetour();
    PrepSkySDKCall();
}

public void OnMapStart()
{
    // 为方便round exec插件加载/卸载，因此初始化就不写这了
}

public void OnClientPutInServer(int client)
{
    GetClientName(client, g_ClientName[client], 32);   // 伪复活插件会更换玩家名字，所以要提前保存
    SDKHook(client, SDKHook_SetTransmit, OnWitchBlindSetTransmit);
}

public void OnClientDisconnect_Post(int client)
{
    g_ClientName[client] = NULL_STRING;
}

public void Event_RoundStart(Event event, const char[] name, bool dontBroadcast)
{
    g_iTrapCount = 0;
}

//========================================================================================
// HOOK 玩家输入
//========================================================================================

public Action OnPlayerRunCmd(int client, int &buttons, int &impulse, float vel[3], float angles[3], int &weapon, int &subtype, int &cmdnum, int &tickcount, int &seed, int mouse[2])
{
    // 干扰人类移动的僵尸技能
    if (IsHumanAlive(client))
    {
        // if分支有优先级
        if (g_bIsGrappled[client])
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
        else if (g_bIsStuck[client])
        {
            vel[0] = vel[1] = vel[2] = 0.0;     // 其实可以不管垂直速度，禁止跳即可

            buttons &= ~IN_JUMP;

            // 检测AD键交替连打
            if (!IsFakeClient(client))
            {
                if ((buttons & IN_MOVELEFT)
                && !(g_iLastButtons[client] & IN_MOVELEFT)
                && g_iUsePressStep[client] % 2 == 0)
                {
                    g_iUsePressStep[client]++;
                }

                if ((buttons & IN_MOVERIGHT)
                && !(g_iLastButtons[client] & IN_MOVERIGHT)
                && g_iUsePressStep[client] % 2 == 1)
                {
                    g_iUsePressStep[client]++;
                }

                SetHudTextParams(0.50, 0.45, 0.1, 255, 0, 0, 255);
                ShowHudText(client, -1, "  快交替按AD键挣脱!!!");
            }
            else
            {
                if (GetRandomInt(1, 20) == 1)
                {
                    g_iUsePressStep[client]++;
                }
            }
        }
        else if (g_bIsShock[client])
        {
            vel[0] = 0.1 * vel[0];
            vel[1] = 0.1 * vel[1];

            buttons &= ~IN_JUMP;
            buttons &= ~IN_DUCK;     // 也没法蹲

            TeleportEntity(client, NULL_VECTOR, g_flAng[client], NULL_VECTOR);
        }
        else if (g_bIsBlind[client])            // 女巫致盲：WASD旋转映射
        {
            int btn = buttons & ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
            int rot = g_iWitchKeyRot[client];

            // rot 0=DWAS(逆时针1位) 1=ASDW(顺时针1位) 2=SDWA(顺时针2位)
            if (buttons & IN_FORWARD)
            {
                if (rot == 0)      btn |= IN_MOVERIGHT;
                else if (rot == 1) btn |= IN_MOVELEFT;
                else               btn |= IN_BACK;
            }
            if (buttons & IN_BACK)
            {
                if (rot == 0)      btn |= IN_MOVELEFT;
                else if (rot == 1) btn |= IN_MOVERIGHT;
                else               btn |= IN_FORWARD;
            }
            if (buttons & IN_MOVELEFT)
            {
                if (rot == 0)      btn |= IN_FORWARD;
                else if (rot == 1) btn |= IN_BACK;
                else               btn |= IN_MOVERIGHT;
            }
            if (buttons & IN_MOVERIGHT)
            {
                if (rot == 0)      btn |= IN_BACK;
                else if (rot == 1) btn |= IN_FORWARD;
                else               btn |= IN_MOVELEFT;
            }

            buttons = btn;

            // 根据映射后的方向flag设置速度
            float fmove = 0.0, smove = 0.0;
            if (btn & IN_FORWARD)   fmove += 200.0;
            if (btn & IN_BACK)      fmove -= 200.0;
            if (btn & IN_MOVELEFT)  smove -= 200.0;
            if (btn & IN_MOVERIGHT) smove += 200.0;
            vel[0] = fmove;
            vel[1] = smove;

            SetHudTextParams(0.50, 0.45, 0.1, 255, 0, 0, 255);
            ShowHudText(client, -1, "  方向键旋转了!!!");
        }

        // 安哥拉飞行刮风（不影响被擒抱/陷阱控制的玩家）
        if (!g_bIsGrappled[client] && !g_bIsStuck[client])
            ApplyWindPush(client, vel);

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
        g_bIsStuck[i] = false;

        g_iUsePressStep[i] = 0;

        g_iZombieCall[i] = -1;

        g_bIsGrappled[i] = false;

        g_iUsePressCount[i] = 0;
        g_iLastButtons[i] = -1;

        // 这两个没有必要初始化，都是在使用前赋值的
        // g_iViewControl
        // g_flEscapePos[]

        ClearGrappleHandles(i);

        g_bIsShock[i] = false;

        g_bIsBlind[i] = false;
        g_iWitchPhase[i] = 0;
        g_flWitchNearTime[i] = 0.0;
        g_iWitchKeyRot[i] = -1;
        g_bWitchDSPFlip[i] = false;

        // 在使用前赋值的
        // g_flAng[]
    }
}

void InitModelCache()
{
    // 模型预缓存
    PrecacheModel(MODEL_ZOMBIETRAP, true);   // 鬼手陷阱
    PrecacheModel(TOOL_BEAMSPRITE, true);	 // 辅助线
    PrecacheModel(TOOL_TRAPPHYS, true);      // 受击体

    // 用于鬼手陷阱受击反馈
    g_iBloodSpray = PrecacheModel("sprites/bloodspray.vmt");
    g_iBloodDrop  = PrecacheModel("sprites/blooddrop.vmt");

    AddFileToDownloadsTable(LUT_WITCH_MILD);
    AddFileToDownloadsTable(LUT_WITCH_SEVERE);
    AddFileToDownloadsTable(LUT_ANGELA_GREEN);

    for (int i = 0; i < DEBRIS_MODEL_COUNT; i++)
    {
        PrecacheModel(g_sDebrisModels[i], true);
    }
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
    PrecacheSound(SFX_WITCH1, true);
    PrecacheSound(SFX_WITCH2, true);

    // 音频预缓存（BOSS安哥拉）
    PrecacheSound(SFX_CALL, true);
    PrecacheSound(SFX_SMASH, true);
    PrecacheSound(SFX_SWING, true);
    PrecacheSound(SFX_HEAL1, true);
    PrecacheSound(SFX_HEAL2, true);
    PrecacheSound(SFX_HEAL3, true);
    PrecacheSound(SFX_FLY, true);
    PrecacheSound(SFX_WIND, true);
    PrecacheSound(SFX_COUGH1, true);
    PrecacheSound(SFX_COUGH2, true);

    // 音频预缓存（BOSS巨型狂暴形态僵尸）
    PrecacheSound(SFX_CHARGE1, true);
    PrecacheSound(SFX_CHARGE2, true);
    PrecacheSound(SFX_GRAPPLE1, true);
    PrecacheSound(SFX_GRAPPLE2, true);

    // 音频预缓存（BOSS异形斗兽）
    PrecacheSound(SFX_DASH1, true);
    PrecacheSound(SFX_DASH2, true);
    PrecacheSound(SFX_DASH3, true);
    PrecacheSound(SFX_DASH4, true);
    PrecacheSound(SFX_SHOCK1, true);
    PrecacheSound(SFX_SHOCK2, true);
}