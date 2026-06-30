//========================================================================================
// NPC僵尸生成事件
//========================================================================================

public void Han_OnZombieCreated(int zombie)
{
    InitZombieState(zombie);

    // 为致盲模型隐藏hook SetTransmit（只hook渲染实体）
    int animEnt = FindAnimEntity(zombie);
    if (animEnt != -1)
        SDKHook(animEnt, SDKHook_SetTransmit, OnWitchBlindSetTransmit);

    // 僵尸种类分支
    if (IsSameName(zombie, ZOMBIE_SMOKE))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_EXPLODE))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_HEAL))
    {
        g_hHeal[zombie] = CreateTimer(HEAL_INTERVAL, Timer_RepeatHealing, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    }
    else if (IsSameName(zombie, ZOMBIE_DEIMOS))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_BUTCHER))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH))
    {
        g_hWitchNear[zombie] = CreateTimer(0.1, Timer_WitchNearThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        SetInvisible(zombie);
    }
    else if (IsSameName(zombie, BOSS_ANGELA))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, BOSS_PANGZI))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, BOSS_YIXING))
    {
        // 什么都不做
    }
}

//========================================================================================
// NPC僵尸死亡事件
//========================================================================================

public void Han_OnZombieDeath(int zombie, int killer)
{
    // 僵尸种类分支
    if (IsSameName(zombie, ZOMBIE_SMOKE))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_EXPLODE))
    {
        StartExplosion(zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_HEAL))
    {
        if (g_hHeal[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hHeal[zombie]);
            g_hHeal[zombie] = INVALID_HANDLE;
        }
    }
    else if (IsSameName(zombie, ZOMBIE_DEIMOS))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_BUTCHER)
        && GetRandomFloat(0.0, 1.0) <= BUTCHER_TRAP_CHANCE
        && g_iTrapCount < BUTCHER_TRAP_MAX
    )
    {
        StartTrap(zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH))
    {
        if (g_hWitchNear[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hWitchNear[zombie]);
            g_hWitchNear[zombie] = INVALID_HANDLE;
        }

        // 最后一头女巫死亡时，清理所有失去覆盖的Phase 1玩家
        for (int victim = 1; victim <= MaxClients; victim++)
        {
            if (g_iWitchPhase[victim] != 1)
                continue;

            bool anyNear;
            for (int i = 1; i <= 2048; i++)
            {
                if (i != zombie && IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH) && GetMeterDistance(i, victim) <= WITCH_NEAR_RANGE)
                {
                    anyNear = true;
                    break;
                }
            }

            if (!anyNear)
            {
                g_iWitchPhase[victim] = 0;
                g_flWitchNearTime[victim] = 0.0;
                KillWitchCC(victim);
            }
        }
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        SetVisible(zombie);    // 死的时候恢复渲染方式，显形
    }
    else if (IsSameName(zombie, BOSS_ANGELA))
    {
        // 如果吸力过程不无敌
        if (g_hPull[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hPull[zombie]);
            g_hPull[zombie] = INVALID_HANDLE;
        }

        // 如果飞行过程不无敌
        if (g_hFly[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hFly[zombie]);
            g_hFly[zombie] = INVALID_HANDLE;
        }
    }
    else if (IsSameName(zombie, BOSS_PANGZI))
    {
        // 如果在冲锋就停止冲锋句柄
        ClearChargeHandles(zombie);

        int victim = g_iGrappling[zombie];

        // 如果有擒抱住人就停止擒抱和连按句柄
        if (victim != -1)
        {
            ClearGrappleHandles(victim);
            EndGrapple(zombie, victim);
        }
    }
    else if (IsSameName(zombie, BOSS_YIXING))
    {
        ClearChargeHandles(zombie);
    }
}

//========================================================================================
// NPC僵尸受伤事件
//========================================================================================

public void Han_OnZombieHurt(int zombie, int attacker)
{
    // 僵尸种类分支
    if (IsSameName(zombie, ZOMBIE_SMOKE) && !g_bIsSkillUsed[zombie])
    {
        g_bIsSkillUsed[zombie] = true;
        StartSmoke(zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_EXPLODE))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_HEAL))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_DEIMOS))
    {
        // 跳跃技能已移除
    }
    else if (IsSameName(zombie, ZOMBIE_BUTCHER))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        // 什么都不做
    }
    // 这里还没改
    else if (IsSameName(zombie, BOSS_ANGELA))
    {
        if (g_hHeal[zombie] != INVALID_HANDLE)
        {
            g_iBulletCount[zombie]++;
        }
        
        if (g_hHeal[zombie] == INVALID_HANDLE && !g_bIsSkillUsed[zombie])
        {
            // 30发子弹，至少触发一次技能的概率是45.45%，目前测下来是很合适的值
            int rand = GetRandomInt(1, 50);

            // 吸力黑洞，BOSS容易被集火而死
            if (rand == 1 
            && GetEntProp(zombie, Prop_Data, "m_iHealth") > g_iMaxHealth[zombie] / 2)
            {
                g_bIsSkillUsed[zombie] = true;
                StartPull(zombie);
            }
            // 感觉还差个技能
            else if (rand == 2 
            && GetEntProp(zombie, Prop_Data, "m_iHealth") > g_iMaxHealth[zombie] / 2)
            {
                // 
            }
            // 一半生命值以下会尝试自愈
            else if (rand == 3
            && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2)
            {
                g_bIsSkillUsed[zombie] = true;
                StartSelfHealing(zombie);
            }
            // 没有概率要求，半血以下开始飞行think避险
            else if (g_hFly[zombie] == INVALID_HANDLE 
            && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2)
            {
                InitFlyingState(zombie);
                g_hFly[zombie] = CreateTimer(0.0, Timer_FlyThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
            }
        }
    }
    else if (IsSameName(zombie, BOSS_PANGZI) && !g_bIsSkillUsed[zombie])
    {
        // 半血以上投掷僵尸
        if (GetMeterDistance(zombie, attacker) > PANGZI_THROW_RANGE
        && IsTargetForward(zombie, attacker)                      // 在前方（防止子弹僵尸卡墙）
        && attacker == GetEntDataEnt2(zombie, g_iLeaderOffset)    // 且目标和攻击者是同一个
        && GetEntProp(zombie, Prop_Data, "m_iHealth") > g_iMaxHealth[zombie] / 2)
        {
            g_bIsSkillUsed[zombie] = true;
            StartPickupAndThrow(zombie, attacker);
        }
        // 半血以下冲锋擒抱
        else if (IsTargetForward(zombie, attacker)                // 在前方
        && attacker == GetEntDataEnt2(zombie, g_iLeaderOffset)    // 且目标和攻击者是同一个
        && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2)
        {
            g_bIsSkillUsed[zombie] = true;
            StartCharge(zombie, attacker);
        }
    }
    else if (IsSameName(zombie, BOSS_YIXING) && !g_bIsSkillUsed[zombie])
    {        
        // 冲撞
        if (IsTargetForward(zombie, attacker)                     // 在前方
        && attacker == GetEntDataEnt2(zombie, g_iLeaderOffset))   // 且目标和攻击者是同一个
        {
            g_bIsSkillUsed[zombie] = true;
            StartDash(zombie, attacker);    // 很巧也是lovelive的歌
        }
    }
    else if (IsSameName(zombie, "神秘胖子僵尸"))
    {
        GiveHealth(attacker, 1);
        GiveArmor(attacker, 1);
        GiveMoney(attacker, 100);
    }
}

//========================================================================================
// NPC僵尸攻击事件
//========================================================================================

public void Han_OnZombieAttack(int zombie, int victim)
{
    // 僵尸种类分支
    if (IsSameName(zombie, ZOMBIE_SMOKE))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_EXPLODE))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_HEAL))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_DEIMOS)
        && !g_bIsSkillUsed[zombie]
    )
    {
        g_bIsSkillUsed[zombie] = true;
        g_bIsStuck[victim] = false;        // 解除憎恶屠夫的控制
        StartKnockback(zombie, victim);
        CreateTimer(DEIMOS_KNOCK_CD, Timer_SkillEnd, zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_BUTCHER))
    {
        // 陷阱改由死亡时生成，攻击时不再触发
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH))
    {
        // 致幻已改为距离触发，攻击时不再触发
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        // 什么都不做
    }
    // 这里还没改
    else if (IsSameName(zombie, BOSS_ANGELA) && !g_bIsSkillUsed[zombie])
    {
        int rand = GetRandomInt(1, 3);

        // 呼唤僵尸集中攻击一个人类，完美的boss技能，效果太好了
        if (rand == 1)
        {
            g_bIsSkillUsed[zombie] = true;
            StartCall(zombie, victim);
        }
        // 群体击飞1 - 砸地动作
        else if (rand == 2)
        {
            g_bIsSkillUsed[zombie] = true;
            StartSmash(zombie);
        }
        // 群体击飞2 - 挥臂动作
        else if (rand == 3)
        {
            g_bIsSkillUsed[zombie] = true;
            StartSwing(zombie);
        }
    }
    else if (IsSameName(zombie, BOSS_PANGZI))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, BOSS_YIXING) && !g_bIsSkillUsed[zombie])
    {
        // 震荡波
        g_bIsSkillUsed[zombie] = true;
        StartShockWave(zombie);
    }
}

//========================================================================================
// TIMER
//========================================================================================

public Action Timer_SkillEnd(Handle timer, int zombie)
{
    g_bIsSkillUsed[zombie] = false;

    return Plugin_Stop;
}

//========================================================================================
//========================================================================================

void InitZombieState(zombie)
{
    // 共用数组
    g_iMaxHealth[zombie] = GetEntProp(zombie, Prop_Data, "m_iHealth");
    g_bIsSkillUsed[zombie] = false;

    // 句柄数组统一初始化
    g_hHeal[zombie] = INVALID_HANDLE;
    g_hPull[zombie] = INVALID_HANDLE;
    g_hFly[zombie] = INVALID_HANDLE;
    g_hCharge[zombie] = INVALID_HANDLE;
    g_hChargePack[zombie] = INVALID_HANDLE;
    g_hMovetype[zombie] = INVALID_HANDLE;

    // 安哥拉使用的数组
    g_iBulletCount[zombie] = 0;
    g_BossState[zombie] = STATE_GROUND;
    g_Mode[zombie] = FLY_RANDOM;
    g_flLastFlyEnd[zombie] = 0.0;
    g_flLastFlySound[zombie] = 0.0;

    // 巨型狂暴使用的数组
    g_iGrappling[zombie] = -1;
    g_bIsPickup[zombie] = false;
}