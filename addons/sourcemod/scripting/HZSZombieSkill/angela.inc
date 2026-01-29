//========================================================================================
// 安哥拉技能：全场吸力，半血以上受击有概率使用
//========================================================================================

void StartPull(int zombie)
{
    // 常规信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 吸力效果
    Han_SetCustomAnimation(zombie, "run_upper_knife", ANGELA_PULL_TIME);   // 飞起来的动作
    ProtectZombie(zombie, ANGELA_PULL_TIME);
    CreatePull(zombie, pos);

    // 延时开放技能占位
    CreateTimer(ANGELA_PULL_TIME, Timer_SkillEnd, zombie);

    // 文字提示
    CPrintToChatAll("{green}[华仔] {red}小心，%s使用了吸力！", ZombieName);
}

void CreatePull(int zombie, float pos[3])
{
    // 状态管理
    for (int victim = 1; victim <= MaxClients; victim++)
    {
        if (IsHumanAlive(victim))    // 如果刚复活就不会被吸，毕竟还要买武器，降低难度吧
        {
            g_iZombiePull[victim] = zombie;                 // 记录安哥拉实体号
            CreateTimer(ANGELA_PULL_TIME, Timer_DePull, victim);
        }
    }

    // 特效
    DataPack pack = new DataPack();
    pack.WriteCell(zombie);
    pack.WriteCell(pos[0]);
    pack.WriteCell(pos[1]);
    pack.WriteCell(pos[2]);
    g_hPull[zombie] = CreateTimer(1.0, Timer_PullParticle, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 音效
    EmitAmbientSound(SFX_PULL2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

public Action Timer_PullParticle(Handle timer, DataPack pack)
{
    float pos[3];

    pack.Reset();
    int zombie = pack.ReadCell();
    pos[0] = pack.ReadCell();
    pos[1] = pack.ReadCell();
    pos[2] = pack.ReadCell();

    // 技能结束
    if (!g_bIsSkillUsed[zombie])
    {
        delete pack;

        KillTimer(timer);
        g_hPull[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    WriteParticle("z_shield_buff", pos, 1.0);

    return Plugin_Continue;
}

public Action Timer_DePull(Handle timer, int client)
{
    g_iZombiePull[client] = -1;

    return Plugin_Stop;
}

//========================================================================================
// 安哥拉技能：群体击飞，一个砸地飞高，一个挥臂飞远
//========================================================================================

void StartSmash(int zombie)
{
    // 砸地
    Han_SetCustomAnimation(zombie, "Walk_shoot_knife3", 4.5);
    CreateTimer(2.0, Timer_CreateSmashKnock, zombie);       // 这里不能1.5，太短
    
    // 延时开放技能占位
    CreateTimer(5.0, Timer_SkillEnd, zombie);
}

void StartSwing(int zombie)
{
    // 再试一次m_angRotation

    // 挥臂
    Han_SetCustomAnimation(zombie, "Walk_shoot_knife2", 3.0);
    CreateTimer(1.3, Timer_CreateSwingKnock, zombie);       // 只有boss朝向才会被击飞，而且水平力度大，垂直力度小，和群体击飞1正好相反

    // 延时开放技能占位
    CreateTimer(3.0, Timer_SkillEnd, zombie);
}

public Action Timer_CreateSmashKnock(Handle timer, int zombie)
{
    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        return Plugin_Stop;
    }

    // 挨个击飞
    for (int victim = 1; victim <= MaxClients; victim++)
    {
        if (IsHumanAlive(victim))
        {
            if (GetMeterDistance(zombie, victim) > ANGELA_KNOCK_RANGE)
            {
                continue;
            }

            // 状态管理
            g_bIsStuck[victim] = false;
            g_bIsInvert[victim] = false;

            // 生成击退
            Shake(victim, 30.0, 10.0, 3.0);
            CreateKnockback(zombie, victim, view_as<float>({400.0, 400.0, 800.0}));
            
            // 生成伤害
            SDKHooks_TakeDamage(victim, zombie, zombie, ANGELA_KNOCK_DAMAGE, DMG_SLASH, _, _, _, false);

            // 音效
            EmitSoundToClient(victim, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        }            
    }

    // 环境音效
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    EmitAmbientSound(SFX_SMASH, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 特效？试试看————卧槽好帅
    pos[2]+= 20.0;
    TE_SetupBeamRingPoint(pos, 10.0, 1000.0, PrecacheModel("materials/sprites/lgtning.vmt"), PrecacheModel("materials/sprites/halo01.vmt"), 1, 1, 0.2, 100.0, 1.0, {255,75,75,255}, 0, 0);
    TE_SendToAll();

    return Plugin_Continue;
}

public Action Timer_CreateSwingKnock(Handle timer, int zombie)
{
    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        return Plugin_Stop;
    }

    // 挨个击飞
    for (int victim = 1; victim <= MaxClients; victim++)
    {
        if (IsHumanAlive(victim))
        {
            if (GetMeterDistance(zombie, victim) > ANGELA_KNOCK_RANGE)
            {
                continue;
            }

            if (!IsTargetForward(zombie, victim))
            {
                continue;
            }
            
            // 状态管理
            g_bIsStuck[victim] = false;
            g_bIsInvert[victim] = false;

            // 生成击退
            Shake(victim, 30.0, 10.0, 3.0);
            CreateKnockback(zombie, victim, view_as<float>({800.0, 800.0, 400.0}));

            // 生成伤害
            SDKHooks_TakeDamage(victim, zombie, zombie, ANGELA_KNOCK_DAMAGE, DMG_SLASH, _, _, _, false);

            // 音效
            EmitSoundToClient(victim, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        }            
    }

    // 环境音效
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    EmitAmbientSound(SFX_SWING, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

//========================================================================================
// 安哥拉技能：呼唤僵尸集中攻击一个人类
//========================================================================================

void StartCall(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    // 僵尸集中攻击
    CreateCall(victim, 9999.0, ANGELA_CALL_TIME);      // 如果victim在这期间死了，还需要再处理

    // 状态管理
    g_iZombieCall[victim] = zombie;                    // 如果死了要提前unlock
    CreateTimer(ANGELA_CALL_TIME, Timer_DeCall, victim);

    // 延时开放技能占位    
    CreateTimer(5.0, Timer_SkillEnd, zombie);

    // 发出声音的实体填了1，如果写横线，大灾变插件里的soundhook会报错
    EmitSoundToAll(SFX_CALL, 1, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChatAll("{green}[华仔] {red}%s正在呼唤所有僵尸攻击%s，快去帮忙掩护！", ZombieName, g_ClientName[victim]);
}

void CreateCall(int victim, float dist, float duration)
{
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

            if (GetMeterDistance(zombie, victim) > dist)
            {
                continue;
            }

            Han_SetZombieTarget(zombie, victim, duration);   // 不需要特别Han_UnlockZombie
        }
    }
}

public Action Timer_DeCall(Handle timer, int client)
{
    g_iZombieCall[client] = -1;

    return Plugin_Stop;
}

//========================================================================================
// 安哥拉技能：可以被打断的自愈，半血以下受击有概率使用
//========================================================================================

void StartSelfHealing(int zombie)
{
    // 常规信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    // 安哥拉在场上可能自愈多次，所以要初始化数组
    g_iBulletCount[zombie] = 0;

    // 自愈句柄                                                                   
    Han_SetZombieTarget(zombie, zombie, 0.0);                                                                          // 安哥拉自愈时杵着不动
    g_hHeal[zombie] = CreateTimer(1.0, Timer_RepeatSelfHealing, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);    // 每秒回一定量血，直到被打断或者满血为止

    // 文字提示
    CPrintToChatAll("{green}[华仔] {red}%s正在尝试自愈，快集中火力！", ZombieName);
}

public Action Timer_RepeatSelfHealing(Handle timer, int zombie)
{
    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        g_hHeal[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    // 自愈过程被打断
    if (g_iBulletCount[zombie] >= ANGELA_HEAL_BULLET)
    {
        EndSelfHealingFail(zombie);

        KillTimer(timer);
        g_hHeal[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    // 自愈以至于回满血了
    if (GetEntProp(zombie, Prop_Data, "m_iHealth") == g_iMaxHealth[zombie])
    {
        EndSelfHealingSuccess(zombie);

        KillTimer(timer);
        g_hHeal[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    // 自愈过程中
    SetEntProp(zombie, Prop_Data, "m_iHealth", 
        GetEntProp(zombie, Prop_Data, "m_iHealth") + ANGELA_HEAL_DAMAGE < g_iMaxHealth[zombie] ? 
        GetEntProp(zombie, Prop_Data, "m_iHealth") + ANGELA_HEAL_DAMAGE : 
        g_iMaxHealth[zombie]
    );

    // 基础信息
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 特效
    pos[2] += 100.0;
    WriteParticle("z_group_healer_zombie_buff", pos, 1.0);

    // 音效
    EmitAmbientSound(SFX_HEAL1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

void EndSelfHealingFail(int zombie)
{
    // 基础信息，结束repeat时要用到
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 还原状态
    g_bIsSkillUsed[zombie] = false;
    Han_UnlockZombie(zombie);

    // 文字音效反馈
    EmitAmbientSound(SFX_HEAL3, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    CPrintToChatAll("{green}[华仔] {red}%s的自愈被打断，快乘胜追击！", ZombieName);
}

void EndSelfHealingSuccess(int zombie)
{
    // 基础信息，结束repeat时要用到
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 还原状态
    g_bIsSkillUsed[zombie] = false;
    Han_UnlockZombie(zombie);

    // 文字音效反馈
    EmitAmbientSound(SFX_HEAL2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    CPrintToChatAll("{green}[华仔] {red}%s完全自愈了，小心！", ZombieName);
}

//========================================================================================
// 安哥拉技能：飞行避险喷毒，半血以下有飞行CD就使用
//========================================================================================

void InitFlyingState(int zombie)
{
    g_BossState[zombie] = STATE_GROUND;           // 默认在地上
    g_Mode[zombie] = FLY_RANDOM;                  // 默认高飞模式

    g_flLastFlyEnd[zombie] = 0.0;
    g_flLastFlySound[zombie] = 0.0;
}

void EnterFlyingMode(int zombie)
{
    SetEntProp(zombie, Prop_Data, "m_takedamage", 0, 1);     // 飞的时候无敌，避免死在天上不好看

    g_BossState[zombie] = STATE_FLYING;
    g_flLastFlyEnd[zombie] = GetEngineTime();                // 飞行开始计时（共用一个数组）
}

void EnterGroundMode(int zombie)
{
    SetEntProp(zombie, Prop_Data, "m_takedamage", 2, 1);

    g_BossState[zombie] = STATE_GROUND;
    g_flLastFlyEnd[zombie] = GetEngineTime();                // 冷却开始计时（共用一个数组）
}

public Action Timer_FlyThink(Handle timer, int zombie)
{    
    float currentTime = GetEngineTime();    

    // 如果当前是地面模式，检查冷却是否结束
    if (g_BossState[zombie] == STATE_GROUND)
    {
        if (currentTime - g_flLastFlyEnd[zombie] >= ANGELA_FLY_CD     // 飞行的CD
        && !g_bIsSkillUsed[zombie]                                    // 正在使用其它技能时无法飞行，避免冲突
        )
        {
            EnterFlyingMode(zombie);                                  // 有CD立刻起飞
            g_bIsSkillUsed[zombie] = true;                            // 其它技能无法使用
        }
        else
        {
            // 在地上可以用其他技能
        }
    }
    // 如果当前是飞行模式，检查持续时间
    else if (g_BossState[zombie] == STATE_FLYING)
    {
        if (currentTime - g_flLastFlyEnd[zombie] >= ANGELA_FLY_TIME)  // 飞行持续时间
        {
            EnterGroundMode(zombie);

            Han_SetCustomAnimationEx(zombie, "Walk_shoot_knife3", 4.5);        // 新api，强制播放，无视旧动画是否播完
            CreateTimer(1.5, Timer_CreateSmashKnock, zombie);                  // 2.0秒太长了
            CreateTimer(5.0, Timer_SkillEnd, zombie);
        }
        else
        {
            FlyingLogic(zombie);
        }
    }

    return Plugin_Continue;
}

void FlyingLogic(int zombie)
{
    int target = GetEntDataEnt2(zombie, g_iLeaderOffset);      // target可能为0

    // 射线检测
    float dist, hitPos[3], FlyForce[3];
    GetDistanceToGround(zombie, dist, hitPos);          // 得到dist和hitPos，都有用
    GetFlyingMode(zombie, target, hitPos);              // 根据距离决定高飞低飞，允许target为0
    
    // 飞行模式分支
    if (g_Mode[zombie] == FLY_RANDOM)     // 没有目标或目标太远（看的是水平距离），高飞
    {
        if (dist <= 400.0)                // 高度小于 600.0 继续抬升
        {
            FlyForce[2] = 800.0;
            TeleportEntity(zombie, NULL_VECTOR, NULL_VECTOR, FlyForce);   // 不能理解为速度，更像是力，需要高频给与才有效果
            // PrintToChatAll("高飞模式 +");
        } 
        else
        {
            // PrintToChatAll("高飞模式 -");
        }
    }
    else if (g_Mode[zombie] == FLY_TARGET)      // 目标在范围内，低飞，由于人质本身的特性，应该会实时朝向目标
    {
        if (dist <= 50.0)                       // 50.0碰撞箱快落地了，而且安哥拉可以打到人，如果不设置takedamage的话
        {
            FlyForce[2] = 300.0;
            TeleportEntity(zombie, NULL_VECTOR, NULL_VECTOR, FlyForce);
            // PrintToChatAll("低飞模式 +");
        }
        else
        {
            // PrintToChatAll("低飞模式 -");
        }
    }

    // 扇动翅膀的声音
    PlayWingSound(zombie);
}

void GetFlyingMode(int zombie, int target, float hitPos[3])
{
    if (target > 0) 
    {
        float fTargetPosition[3];
        GetEntPropVector(target, Prop_Send, "m_vecOrigin", fTargetPosition);

        if (GetVectorDistance(hitPos, fTargetPosition) <= 600.0)    // 大概8米 目标离近了，就低飞并展开喷毒，看的是水平距离小于400.0（引擎单位）
        {
            g_Mode[zombie] = FLY_TARGET;                // 执行低飞模式
        }
        else
        {
            g_Mode[zombie] = FLY_RANDOM;                // 执行高飞模式
        }
    }
    else
    {
        g_Mode[zombie] = FLY_RANDOM;                    // 没有目标 也设置高飞模式
    }
}

void PlayWingSound(int zombie)
{
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    float currentTime = GetEngineTime();

    if (currentTime - g_flLastFlySound[zombie] >= 0.8)
    {
        g_flLastFlySound[zombie] = currentTime;
        
        Han_SetCustomAnimationEx(zombie, "run_upper_knife", 0.9);
        EmitAmbientSound(SFX_FLY, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL)
    }
}