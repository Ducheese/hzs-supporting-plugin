//========================================================================================
// NPC僵尸生成事件
//========================================================================================

public void Han_OnZombieCreated(int zombie)
{
    InitZombieState(zombie);

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
        // 什么都不做
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
    else if (IsSameName(zombie, ZOMBIE_BUTCHER))
    {
        // 暂时什么都不做
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH))
    {
        // 暂时什么都不做
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
}

//========================================================================================
// NPC僵尸受伤事件
//========================================================================================

public void Han_OnZombieHurt(int zombie, int attacker)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

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
        // 什么都不做
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

            // 一半生命值以下会尝试自愈
            if (rand == 1 && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2)
            {
                // 改成start
                g_bIsSkillUsed[zombie] = true;
                g_iBulletCount[zombie] = 0;                                                                                        // 安哥拉在场上可能自愈多次，所以要初始化

                Han_SetZombieTarget(zombie, zombie, 0.0);                                                                          // 安哥拉自愈时杵着不动
                g_hHeal[zombie] = CreateTimer(1.0, Timer_ZombieSelfHealing, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);    // 每秒回一定量血，直到被打断或者满血为止

                CPrintToChatAll("{green}[华仔] {red}%s正在尝试自愈，快集中火力！", ZombieName);
            }
            // 吸力黑洞，BOSS容易被集火而死
            else if (rand == 2 && GetEntProp(zombie, Prop_Data, "m_iHealth") > g_iMaxHealth[zombie] / 2)
            {
                // 改成start
                g_bIsSkillUsed[zombie] = true;

                Han_SetCustomAnimation(zombie, "run_upper_knife", 5.0);   // 飞起来的动作
                ProtectZombie(zombie, 5.0);
                CreatePull(zombie, pos);
                CreateTimer(5.0, Timer_SkillEnd, zombie);

                CPrintToChatAll("{green}[华仔] {red}小心，%s使用了吸力！", ZombieName);
            }
            // 没有概率要求，半血以下开始飞行think避险
            else if (g_hFly[zombie] == INVALID_HANDLE 
                && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2
            )
            {
                InitFlyingState(zombie);
                g_hFly[zombie] = CreateTimer(0.0, Timer_FlyThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
            }
        }
    }
    else if (IsSameName(zombie, BOSS_PANGZI) && !g_bIsSkillUsed[zombie])
    {
        int target = GetEntDataEnt2(zombie, g_iLeaderOffset);

        if (target == attacker)    // 如果目标和攻击者是同一个，才触发冲锋擒抱
        {
            g_bIsSkillUsed[zombie] = true;
            StartCharge(zombie, attacker);
        }
    }
    // 这里还没改
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
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    
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
    else if (IsSameName(zombie, ZOMBIE_DEIMOS))
    {
        g_bIsStuck[victim] = false;        // 解除憎恶屠夫的控制
        g_bIsInvert[victim] = false;       // 解除嗜血女巫的干扰
        StartKnockback(zombie, victim);
    }
    else if (IsSameName(zombie, ZOMBIE_BUTCHER) 
        && !g_bIsSkillUsed[zombie]         // 一次性技能
        && !g_bIsStuck[victim]             // 一个人不会同时中多个陷阱
    )
    {
        g_bIsSkillUsed[zombie] = true;
        g_bIsStuck[victim] = true;
        StartTrap(zombie, victim);
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH) 
        && !g_bIsSkillUsed[zombie] 
        && !g_bIsInvert[victim]
    )
    {
        g_bIsSkillUsed[zombie] = true;
        g_bIsInvert[victim] = true;
        StartInvert(zombie, victim);
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
            g_iZombieCall[victim] = zombie;                    // 如果死了要提前unlock

            CreateZombieCall(victim, 9999.0, ANGELA_CALL_TIME);            // 如果victim在这期间死了，还需要再处理
            CreateTimer(5.0, Timer_SkillEnd, zombie);

            CPrintToChatAll("{green}[华仔] {red}%s正在呼唤所有僵尸攻击%s，快去帮忙掩护！", ZombieName, g_ClientName[victim]);
        }
        // 群体击飞1 - 砸地动作
        else if (rand == 2)
        {
            g_bIsSkillUsed[zombie] = true;

            Han_SetCustomAnimation(zombie, "Walk_shoot_knife3", 4.5);
            CreateTimer(2.0, Timer_CreateGroupKnockback, zombie);       // 这里不能1.5，太短
            CreateTimer(5.0, Timer_SkillEnd, zombie);
        }
        // 群体击飞2 - 挥臂动作
        else if (rand == 3)
        {
            g_bIsSkillUsed[zombie] = true;

            float fTargetPosition[3], fTempPoints[3], fTempAngles[3];
            GetEntPropVector(victim, Prop_Send, "m_vecOrigin", fTargetPosition);        // 试过m_angRotation完全不准确
            MakeVectorFromPoints(pos, fTargetPosition, fTempPoints);
            GetVectorAngles(fTempPoints, fTempAngles);

            DataPack pack = new DataPack();
            pack.WriteCell(zombie);
            pack.WriteCell(fTempAngles[0]);
            pack.WriteCell(fTempAngles[1]);
            pack.WriteCell(fTempAngles[2]);

            Han_SetCustomAnimation(zombie, "Walk_shoot_knife2", 3.0);
            CreateTimer(1.3, Timer_CreateGroupKnockback2, pack);       // 只有boss朝向才会被击飞，而且水平力度大，垂直力度小，和群体击飞1正好相反
            CreateTimer(3.0, Timer_SkillEnd, zombie);
        }
    }
    else if (IsSameName(zombie, BOSS_PANGZI))
    {
        // 什么都不做
    }
    // 这里还没改
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
    g_bIsSkillUsed[zombie] = false;
    g_iMaxHealth[zombie] = GetEntProp(zombie, Prop_Data, "m_iHealth");

    // 安哥拉使用的数组在自己的if分支初始化

    // 巨型狂暴使用的数组
    g_iGrappling[zombie] = -1;
}