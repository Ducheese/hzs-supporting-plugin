//========================================================================================
// NPC僵尸生成事件
//========================================================================================

public void Han_OnZombieCreated(int zombie)
{
    // 安哥拉使用的数组
    g_iBulletCount[zombie] = 0;

    // 共用数组
    g_bIsSkillUsed[zombie] = false;
    g_iMaxHealth[zombie] = GetEntProp(zombie, Prop_Data, "m_iHealth");

    // 僵尸种类分支
    if (IsSameName(zombie, ZOMBIE_GHOST))
    {
        int AnimEnt = FindAnimEntity(zombie);

        if (AnimEnt != -1)
        {
            SetEntityRenderMode(AnimEnt, RENDER_TRANSCOLOR);
            SetEntityRenderFx(AnimEnt, RENDERFX_STROBE_FAST);     // https://sm.alliedmods.net/api/index.php?fastload=show&id=827&
            SetEntityRenderColor(AnimEnt, 0, 0, 0, 1);
        }
    }
    else if (IsSameName(zombie, ZOMBIE_HEAL))
    {
        RepeatTask[zombie] = CreateTimer(1.0, Timer_ZombieHealing, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    }
}

//========================================================================================
// NPC僵尸死亡事件
//========================================================================================

public void Han_OnZombieDeath(int zombie, int killer)
{
    // 计时器关闭
    if (RepeatTask[zombie] != INVALID_HANDLE)
    {
        CloseHandle(RepeatTask[zombie]);
        RepeatTask[zombie] = INVALID_HANDLE;
    }

    if (RepeatTask2[zombie] != INVALID_HANDLE)
    {
        CloseHandle(RepeatTask2[zombie]);
        RepeatTask2[zombie] = INVALID_HANDLE;
    }

    if (RepeatTask3[zombie] != INVALID_HANDLE)
    {
        CloseHandle(RepeatTask3[zombie]);
        RepeatTask3[zombie] = INVALID_HANDLE;
    }

    // 僵尸种类分支
    if (IsSameName(zombie, ZOMBIE_EXPLODE))
    {
        float pos[3];
        GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

        DataPack pack = new DataPack();
        pack.WriteCell(zombie);
        pack.WriteCell(pos[0]);
        pack.WriteCell(pos[1]);
        pack.WriteCell(pos[2]);

        // 死了1.5秒后，不见得m_vecOrigin属性值还在，所以用pack包装了下参数
        CreateTimer(1.5, Timer_CreateExplosion, pack);
        EmitAmbientSound(SFX_EXPLODE1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    }
    else if (IsSameName(zombie, ZOMBIE_GHOST))                // 死的时候恢复渲染方式，显形
    {
        int AnimEnt = FindAnimEntity(zombie);

        if (AnimEnt != -1)
        {
            SetEntityRenderMode(AnimEnt, RENDER_NORMAL);
            SetEntityRenderFx(AnimEnt, RENDERFX_NONE);
            SetEntityRenderColor(AnimEnt, 255, 255, 255, 255);
        }
    }
    // else if (IsSameName(zombie, ZOMBIE_BUTCHER))
    // {
    //     // 死后取消影响？
    // }
    // else if (IsSameName(zombie, ZOMBIE_WITCH))
    // {
    //     // 死后取消影响？
    // }
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

        CreateSmoke(pos);
    }
    else if (IsSameName(zombie, "神秘胖子僵尸"))
    {
        SetEntProp(attacker, Prop_Data, "m_iHealth", GetEntProp(attacker, Prop_Data, "m_iHealth") + 1 < MAX_HEALTH ? GetEntProp(attacker, Prop_Data, "m_iHealth") + 1 : MAX_HEALTH);
        SetEntProp(attacker, Prop_Send, "m_iAccount", GetEntProp(attacker, Prop_Send, "m_iAccount") + 100 < MAX_MONEY ? GetEntProp(attacker, Prop_Send, "m_iAccount") + 100 : MAX_MONEY);
    }
    else if (IsSameName(zombie, BOSS_ANGELA))
    {
        if (RepeatTask[zombie] != INVALID_HANDLE)
        {
            g_iBulletCount[zombie]++;
        }
        
        if (RepeatTask[zombie] == INVALID_HANDLE && !g_bIsSkillUsed[zombie])
        {
            // 30发子弹，至少触发一次技能的概率是45.45%，目前测下来是很合适的值
            int rand = GetRandomInt(1, 50);

            // 一半生命值以下会尝试自愈
            if (rand == 1 && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2)
            {
                g_bIsSkillUsed[zombie] = true;
                g_iBulletCount[zombie] = 0;                                                                                        // 安哥拉在场上可能自愈多次，所以要初始化

                Han_SetZombieTarget(zombie, zombie, 0.0);                                                                          // 安哥拉自愈时杵着不动
                RepeatTask[zombie] = CreateTimer(1.0, Timer_ZombieSelfHealing, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);    // 每秒回一定量血，直到被打断或者满血为止

                CPrintToChatAll("{green}[华仔] {red}%s正在尝试自愈，快集中火力！", ZombieName);
            }
            // 吸力黑洞，BOSS容易被集火而死
            else if (rand == 2 && GetEntProp(zombie, Prop_Data, "m_iHealth") > g_iMaxHealth[zombie] / 2)
            {
                g_bIsSkillUsed[zombie] = true;

                Han_SetCustomAnimation(zombie, "run_upper_knife", 5.0);   // 飞起来的动作
                ProtectZombie(zombie, 5.0);
                CreatePull(zombie, pos);
                CreateTimer(5.0, Timer_SkillEnd, zombie);

                CPrintToChatAll("{green}[华仔] {red}小心，%s使用了吸力！", ZombieName);
            }
            // 没有概率要求，半血以下开始飞行think避险
            else if (RepeatTask3[zombie] == INVALID_HANDLE && GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 2)
            {
                // g_bIsSkillUsed[zombie] = true;

                InitFlyingState(zombie);
                RepeatTask3[zombie] = CreateTimer(0.0, Timer_ZombieFlyingThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
            }
        }
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
    if (IsSameName(zombie, ZOMBIE_DEIMOS))
    {
        g_bIsStuck[victim] = false;   // 解除憎恶屠夫的控制

        Shake(victim, 30.0, 10.0, 3.0);
        CreateKnockback(pos, victim, view_as<float>({KnockBackPower, KnockBackPower, KnockBackPower}));

        EmitSoundToClient(victim, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChat(victim, "{green}[华仔] {red}你被%s击飞了！", ZombieName);
    }
    else if (IsSameName(zombie, ZOMBIE_BUTCHER) && !g_bIsSkillUsed[zombie] && !g_bIsStuck[victim] && g_iZombiePull[victim] == -1)
    {
        g_bIsSkillUsed[zombie] = true;
        g_bIsStuck[victim] = true;

        CreateTrap(victim);
        CreateTimer(StuckTime, Timer_DeStuck, victim);

        EmitSoundToClient(victim, SFX_STUCK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChat(victim, "{green}[华仔] {red}你被%s的鬼手控制住了！", ZombieName);
    }
    else if (IsSameName(zombie, ZOMBIE_WITCH) && !g_bIsSkillUsed[zombie] && !g_bIsDisorder[victim])
    {
        g_bIsSkillUsed[zombie] = true;
        g_bIsDisorder[victim] = true;

        CreateTimer(DisorderTime, Timer_DeDisorder, victim);

        EmitSoundToClient(victim, SFX_DISORDER, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChat(victim, "{green}[华仔] {red}你中了%s的幻术！", ZombieName);
    }
    else if (IsSameName(zombie, BOSS_ANGELA) && !g_bIsSkillUsed[zombie])
    {
        int rand = GetRandomInt(1, 3);

        // 呼唤僵尸集中攻击一个人类，完美的boss技能，效果太好了
        if (rand == 1)
        {
            g_bIsSkillUsed[zombie] = true;
            g_iZombieCall[victim] = zombie;                    // 如果死了要提前unlock

            CreateZombieCall(victim, 9999.0, 15.0);            // 如果victim在这期间死了，还需要再处理
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
}

//========================================================================================
// TIMER
//========================================================================================

public Action Timer_SkillEnd(Handle timer, int entity)
{
    g_bIsSkillUsed[entity] = false;

    return Plugin_Continue;
}

public Action Timer_DeStuck(Handle timer, int client)
{
    g_bIsStuck[client] = false;

    return Plugin_Continue;
}

public Action Timer_DeDisorder(Handle timer, int client)
{
    g_bIsDisorder[client] = false;

    return Plugin_Continue;
}