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
        // 这个0.1s间隔要和witch.inc里的递增同步
        g_hWitchNear[zombie] = CreateTimer(0.1, Timer_WitchNearThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        SetInvisible(zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_IMPOSTER))
    {
        g_hImposterSync[zombie] = CreateTimer(0.1, Timer_ImposterSyncThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    }
    else if (IsSameName(zombie, ZOMBIE_MYSTERY))
    {
        g_hMysteryNear[zombie] = CreateTimer(0.1, Timer_MysteryNearThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    }
    else if (IsSameName(zombie, BOSS_ANGELA))
    {
        // 主要是用于飞行期间的伤害计算，也有累积计数逻辑
        SDKHook(zombie, SDKHook_OnTakeDamage, OnFlyTakeDamage);
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
        // 清理逻辑比较简单，直接裸露出来了
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
        // 有限制的生成鬼手陷阱
        StartTrap(zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH))
    {
        if (g_hWitchNear[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hWitchNear[zombie]);
            g_hWitchNear[zombie] = INVALID_HANDLE;
        }

        /**
         * 入参 excludeEntity = zombie 排除死者后，发现还有活的女巫在范围内 → 不清理，保持 Phase 1 正确
         * 如果女巫全死光、没有 timer 再跑清理的时候，靠这个死亡事件来扫尾
         */
        CleanupOrphanedWitchPhase1(zombie);
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        SetVisible(zombie);    // 死的时候恢复渲染方式，显形
    }
    else if (IsSameName(zombie, ZOMBIE_IMPOSTER))
    {
        EndImposter(zombie);
        GiveClip(killer);
    }
    else if (IsSameName(zombie, ZOMBIE_MYSTERY))
    {
        if (g_hMysteryNear[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hMysteryNear[zombie]);
            g_hMysteryNear[zombie] = INVALID_HANDLE;
        }

        CleanupMysteryPortal(zombie);
    }
    else if (IsSameName(zombie, BOSS_ANGELA))
    {
        // 清理自愈timer
        if (g_hHeal[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hHeal[zombie]);
            g_hHeal[zombie] = INVALID_HANDLE;
        }

        /**
         * 飞行期间死亡
         * 注意，飞行think既控制飞行，也控制落地，即便落地后也还在继续
         */
        if (g_hFly[zombie] != INVALID_HANDLE)
        {
            KillTimer(g_hFly[zombie]);
            g_hFly[zombie] = INVALID_HANDLE;

            if (g_BossState[zombie] == STATE_FLYING)
            {
                // 说白了就是提前了 EnterGroundMode 的清理逻辑
                g_iFlightCount--;
                if (g_iFlightCount <= 0)
                {
                    RemovePoisonFog();
                    RemovePoisonDamage();
                }

                if (g_iDebrisOwner == zombie)
                    RemoveDebrisSystem(zombie);
                    
                OffsetAnimZ(zombie, false);
            }
        }

        SDKUnhook(zombie, SDKHook_OnTakeDamage, OnFlyTakeDamage);
    }
    else if (IsSameName(zombie, BOSS_PANGZI))
    {
        /**
         * pangzi.inc的几个repeat timer，在僵尸有效性不通过时
         * 都只是清空自己的handles数组，没有做完全的清理收尾
         * 这里清理函数并非冗余
         */
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
    else if (IsSameName(zombie, ZOMBIE_IMPOSTER))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_MYSTERY))
    {
        GiveHealth(attacker, 1);
        GiveArmor(attacker, 1);
        GiveMoney(attacker, 100);
    }
    else if (IsSameName(zombie, BOSS_ANGELA) && !g_bIsSkillUsed[zombie])
    {
        // 30发子弹，至少触发一次技能的概率是45.45%，目前测下来是很合适的值
        int rand = GetRandomInt(1, 50);

        // 差个技能
        if (rand == 1
        && GetEntProp(zombie, Prop_Data, "m_iHealth") > g_iMaxHealth[zombie] / 2)
        {
            // 最好是个远程技能
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
        && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie])
        {
            /**
             * g_bIsSkillUsed 的更新在 EnterFlyingMode
             */
            g_hFly[zombie] = CreateTimer(0.0, Timer_FlyThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
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
    else if (IsSameName(zombie, ZOMBIE_DEIMOS) && !g_bIsSkillUsed[zombie])
    {
        // 因为逻辑很简短，就直接裸露出来了
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
    else if (IsSameName(zombie, ZOMBIE_IMPOSTER))
    {
        // 什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_MYSTERY))
    {
        // 什么都不做
    }
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

void InitZombieState(int zombie)
{
    // 共用数组
    g_iMaxHealth[zombie] = GetEntProp(zombie, Prop_Data, "m_iHealth");
    g_bIsSkillUsed[zombie] = false;

    // 屠夫不需要，它没有 per-zombie 的状态变量

    // 治疗僵尸 / BOSS 安哥拉 — 自愈
    g_hHeal[zombie] = INVALID_HANDLE;

    // 嗜血女巫 — 靠近检测
    g_hWitchNear[zombie] = INVALID_HANDLE;

    // 伪人僵尸 — 模仿
    g_iImposterTarget[zombie] = -1;
    g_iFakeBody[zombie] = -1;
    g_hImposterSync[zombie] = INVALID_HANDLE;
    g_bImposterPaused[zombie] = false;

    // 神秘僵尸 — 传送门
    g_hMysteryNear[zombie] = INVALID_HANDLE;
    g_hMysterySwap[zombie] = INVALID_HANDLE;
    g_iPortal[zombie] = -1;

    // BOSS 安哥拉 — 飞行
    g_hFly[zombie] = INVALID_HANDLE;
    g_iBulletCount[zombie] = 0;
    g_BossState[zombie] = STATE_GROUND;
    g_Mode[zombie] = FLY_RANDOM;
    g_flLastFlyEnd[zombie] = 0.0;
    g_flLastFlySound[zombie] = 0.0;

    // BOSS 巨型狂暴形态僵尸 — 冲锋/擒抱/投掷
    g_hCharge[zombie] = INVALID_HANDLE;
    g_hChargePack[zombie] = INVALID_HANDLE;
    g_hMovetype[zombie] = INVALID_HANDLE;
    g_iGrappling[zombie] = -1;
    g_bIsPickup[zombie] = false;

    // BOSS 异形斗兽 — 冲锋（g_hCharge/g_hChargePack 已在上方初始化）
}