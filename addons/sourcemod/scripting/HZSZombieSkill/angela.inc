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
            if (GetMeterDistance(zombie, victim) > ANGELA_KNOCK_RADIUS)
            {
                continue;
            }

            // 状态管理
            g_bIsStuck[victim] = false;

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
    pos[2] += 20.0;
    TE_SetupBeamRingPoint(pos, 10.0, 1000.0, PrecacheModel("materials/sprites/lgtning.vmt"), PrecacheModel("materials/sprites/halo01.vmt"), 1, 1, 0.2, 100.0, 1.0, {255, 75, 75, 255}, 0, 0);
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
            if (GetMeterDistance(zombie, victim) > ANGELA_KNOCK_RADIUS)
            {
                continue;
            }

            if (!IsTargetForward(zombie, victim))
            {
                continue;
            }

            // 状态管理
            g_bIsStuck[victim] = false;

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

            // 呼唤对其他 Boss 无效
            if (IsSameName(zombie, BOSS_ANGELA)
            || IsSameName(zombie, BOSS_PANGZI)
            || IsSameName(zombie, BOSS_YIXING))
                continue;

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
    Han_SetZombieTarget(zombie, zombie, 0.0);                                                                       // 安哥拉自愈时杵着不动
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
    CreateTimer(3.0, Timer_SkillEnd, zombie);
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
    CreateTimer(3.0, Timer_SkillEnd, zombie);
    Han_UnlockZombie(zombie);

    // 文字音效反馈
    EmitAmbientSound(SFX_HEAL2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    CPrintToChatAll("{green}[华仔] {red}%s完全自愈了，小心！", ZombieName);
}

//========================================================================================
// 安哥拉技能：飞行逻辑基础
//========================================================================================

void EnterFlyingMode(int zombie)
{
    // 起飞基础
    g_BossState[zombie] = STATE_FLYING;                      // 切换飞行状态
    g_flLastFlyEnd[zombie] = GetEngineTime();                // 飞行开始计时（共用一个数组）

    g_bIsSkillUsed[zombie] = true;                           // 其它技能无法使用
    g_iBulletCount[zombie] = 0;                              // 重置命中计数

    // 飞行增强
    if (g_iFlightCount == 0)
    {
        CreatePoisonFog();                                   // 全场绿雾 + CC + 更换天空盒
        CreatePoisonDamage();                                // 每玩家独立毒伤 timer
    }
    g_iFlightCount++;

    if (g_iDebrisOwner == -1)
        CreateDebrisSystem(zombie);                          // 飞行垃圾效果

    // 修复补丁
    OffsetAnimZ(zombie, true);                               // 视觉模型下移，对齐碰撞箱
}

void EnterGroundMode(int zombie)
{
    // 落地基础
    g_BossState[zombie] = STATE_GROUND;                      // 切换地面状态
    g_flLastFlyEnd[zombie] = GetEngineTime();                // 冷却开始计时（共用一个数组）

    Han_SetCustomAnimationEx(zombie, "Walk_shoot_knife3", 4.5);        // 新api，强制播放，无视旧动画是否播完
    CreateTimer(1.5, Timer_CreateSmashKnock, zombie);                  // 2.0秒太长了
    CreateTimer(5.0, Timer_SkillEnd, zombie);

    // 飞行增强
    g_iFlightCount--;
    if (g_iFlightCount <= 0)
    {
        RemovePoisonFog();                                   // 还原绿雾 + CC + 天空盒
        RemovePoisonDamage();                                // 停止所有毒伤 timer
    }

    if (g_iDebrisOwner == zombie)
        RemoveDebrisSystem(zombie);                          // 停止飞行垃圾效果

    // 修复补丁
    OffsetAnimZ(zombie, false);                              // 视觉模型归位
}

void OffsetAnimZ(int zombie, bool bFlying)
{
    int anim = FindAnimEntity(zombie);
    if (anim == -1 || !IsValidEntity(anim))
        return;

    int effects = GetEntProp(anim, Prop_Send, "m_fEffects");
    float delta = ANGELA_FLY_ANIM_OFFSET / GAMEUNITS_TO_METERS;

    if (bFlying)
    {
        SetEntProp(anim, Prop_Send, "m_fEffects", effects & ~0x001);
        delta = -delta;
    }
    else
    {
        SetEntProp(anim, Prop_Send, "m_fEffects", effects | 0x001);
    }

    float pos[3];
    GetEntPropVector(anim, Prop_Send, "m_vecOrigin", pos);
    pos[2] += delta;
    SetEntPropVector(anim, Prop_Send, "m_vecOrigin", pos);
}

public Action Timer_FlyThink(Handle timer, int zombie)
{
    // 合法性检测
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(g_hFly[zombie]);
        g_hFly[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    float currentTime = GetEngineTime();

    // 如果当前是地面模式，检查冷却是否结束
    if (g_BossState[zombie] == STATE_GROUND)
    {
        TeleportOffSkybox(zombie);

        if (currentTime - g_flLastFlyEnd[zombie] >= ANGELA_FLY_CD     // 飞行的CD
        && !g_bIsSkillUsed[zombie]                                    // 正在使用其它技能时无法飞行，避免冲突
        )
        {
            EnterFlyingMode(zombie);                                  // 有CD立刻起飞
        }
        else
        {
            // 在地上可以用其他技能（开始飞行后的二阶段的感觉）
        }
    }
    // 如果当前是飞行模式，检查持续时间
    else if (g_BossState[zombie] == STATE_FLYING)
    {
        if (currentTime - g_flLastFlyEnd[zombie] >= ANGELA_FLY_TIME  // 飞行持续时间
        || g_iBulletCount[zombie] >= ANGELA_FLY_BULLET)              // 受击达到阈值
        {
            EnterGroundMode(zombie);
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
    // Han_Think 被翅膀动画阻塞（IsCustomAnim 每 0.8s 刷新）无机会更新目标，在这里补偿回来
    int target = FindNearestHuman(zombie);
    if (target != -1)
        SetEntDataEnt2(zombie, g_iLeaderOffset, target);
    else
        SetEntDataEnt2(zombie, g_iLeaderOffset, -1);

    // HAN 受击钩子会设 MOVETYPE_NONE，每帧恢复
    SetEntityMoveType(zombie, MOVETYPE_STEP);

    // 高度与飞行模式
    float dist = GetDistanceToGround(zombie);                  // 根据高度调整力度
    GetFlyingMode(zombie, target);                             // 根据水平距离决定高飞低飞

    float FlyForce[3];

    // 飞行模式分支
    if (g_Mode[zombie] == FLY_RANDOM)     // 没有目标或目标太远（看的是水平距离），高飞
    {
        if (dist <= 800.0)                // 高度小于 600.0 继续抬升
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
        if (dist <= 400.0)                      // 50.0碰撞箱快落地了，而且安哥拉可以打到人，如果不设置takedamage的话
        {
            FlyForce[2] = 800.0;
            TeleportEntity(zombie, NULL_VECTOR, NULL_VECTOR, FlyForce);
            // PrintToChatAll("低飞模式 +");
        }
        else
        {
            // PrintToChatAll("低飞模式 -");
        }
    }

    // 扇动翅膀
    FlapWing(zombie);

    // debug 输出
    // {
    //     int leader = GetEntDataEnt2(zombie, g_iLeaderOffset);
    //     if (IsHumanAlive(leader))
    //     {
    //         float zEye[3], lPos[3];
    //         GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", zEye);
    //         zEye[2] += 55.0;
    //         GetClientAbsOrigin(leader, lPos);
    //         lPos[2] += 36.0;
    //         TE_SetupBeamPoints(zEye, lPos, PrecacheModel(TOOL_BEAMSPRITE), 0, 0, 0, 0.15, 1.5, 1.5, 0, 0.0, {0, 255, 255, 255}, 0);
    //         TE_SendToAll();
    //     }
    //     else
    //     {
    //         static float lastWarn;
    //         float now = GetEngineTime();
    //         if (now - lastWarn > 2.0)
    //         {
    //             lastWarn = now;
    //             PrintToChatAll("[Angela] ⚠ m_leader = -1 (丢失跟随目标)");
    //         }
    //     }
    // }
}

void GetFlyingMode(int zombie, int target)
{
    // 血量低于1/x：持续高飞避险，忽略目标距离
    if (GetEntProp(zombie, Prop_Data, "m_iHealth") < g_iMaxHealth[zombie] / 4)
    {
        g_Mode[zombie] = FLY_RANDOM;
        return;
    }

    if (target > 0)
    {
        float zPos[3], tPos[3];
        GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", zPos);
        GetEntPropVector(target, Prop_Send, "m_vecOrigin", tPos);

        zPos[2] = 0.0;
        tPos[2] = 0.0;

        // 只看水平距离，没法用公共距离函数
        if (GetVectorDistance(zPos, tPos) <= 500.0)
            g_Mode[zombie] = FLY_TARGET;                // 执行低飞模式
        else
            g_Mode[zombie] = FLY_RANDOM;                // 执行高飞模式
    }
    else
    {
        g_Mode[zombie] = FLY_RANDOM;                    // 没有目标，也设置高飞模式
    }
}

void FlapWing(int zombie)
{
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    float currentTime = GetEngineTime();

    if (currentTime - g_flLastFlySound[zombie] >= 0.8)
    {
        g_flLastFlySound[zombie] = currentTime;

        Han_SetCustomAnimationEx(zombie, "run_upper_knife", 0.9);
        EmitAmbientSound(SFX_FLY, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    }
}

//========================================================================================
// 飞行受击拦截
//========================================================================================

public Action OnFlyTakeDamage(int zombie, int &attacker, int &inflictor, float &damage, int &damagetype)
{
    // 自愈期间：累计子弹数，伤害正常放行
    if (g_hHeal[zombie] != INVALID_HANDLE)
    {
        g_iBulletCount[zombie]++;
        return Plugin_Continue;
    }

    // 地面且非自愈模式放行
    if (g_BossState[zombie] != STATE_FLYING)
        return Plugin_Continue;

    // 飞行模式：伤害随高度衰减
    float groundDist = GetDistanceToGround(zombie);
    float factor = 1.0 - (groundDist / 800.0);
    if (factor < 0.0) factor = 0.0;
    // float original = damage;
    damage *= factor;

    // debug 输出
    // if (IsValidClient(attacker))
    // {
    //     int target = GetEntDataEnt2(zombie, g_iLeaderOffset);
    //     float targetDist = -1.0;
    //     if (target > 0)
    //     {
    //         float zPos[3], tPos[3];
    //         GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", zPos);
    //         GetEntPropVector(target, Prop_Send, "m_vecOrigin", tPos);
    //         zPos[2] = 0.0;
    //         tPos[2] = 0.0;
    //         // 只看水平距离，没法用公共距离函数
    //         targetDist = GetVectorDistance(zPos, tPos) * GAMEUNITS_TO_METERS;
    //     }
    //     PrintToChat(attacker, "高度: %.0f | 衰减: %.0f%% | 伤害: %.0f -> %.0f | 子弹: %d | 目标距离: %.1fm",
    //         groundDist, factor * 100, original, damage, g_iBulletCount[zombie] + 1, targetDist);
    // }

    g_iBulletCount[zombie]++;

    return Plugin_Changed;
}

//========================================================================================
// 安哥拉技能：飞行增强 — 毒雾
//========================================================================================

void CreatePoisonFog()
{
    // 全场雾
    int fog = CreateEntityByName("env_fog_controller");
    if (fog != -1)
    {
        // spawn 时关着，设好所有参数最后再 TurnOn，避免瞬间闪过默认值
        DispatchKeyValue(fog, "fogenable", "0");
        DispatchKeyValue(fog, "fogblend", "1");
        DispatchKeyValue(fog, "fogstart", "5000");
        DispatchKeyValue(fog, "fogend", "10000");
        DispatchKeyValue(fog, "fogcolor", "60 90 20");
        DispatchKeyValue(fog, "fogmaxdensity", "0.99");
        DispatchKeyValue(fog, "foglerptime", "5.0");
        DispatchSpawn(fog);
        ActivateEntity(fog);

        // lerp fogstart/fogend 到目标值，最后开雾
        SetVariantString("0");
        AcceptEntityInput(fog, "SetStartDistLerpTo");
        SetVariantString("1000");
        AcceptEntityInput(fog, "SetEndDistLerpTo");

        // 驱动 fogstart/fogend 从当前值（5000/10000）平滑 lerp 到目标值（0/1000）
        AcceptEntityInput(fog, "StartFogTransition");

        AcceptEntityInput(fog, "TurnOn");

        for (int client = 1; client <= MaxClients; client++)
        {
            if (IsValidClient(client))
                SetEntPropEnt(client, Prop_Send, "m_PlayerFog.m_hCtrl", fog);
        }
    }

    g_iFog = fog;

    // 全场绿色 color_correction
    if (g_iFogCC != -1 && IsValidEntity(g_iFogCC))
    {
        // 复用已有实体，避免客户端 LUT handle 冲突
        AcceptEntityInput(g_iFogCC, "Enable");
    }
    else
    {
        int cc = CreateEntityByName("color_correction");
        if (cc != -1)
        {
            DispatchKeyValue(cc, "filename", LUT_ANGELA_GREEN);
            DispatchKeyValue(cc, "maxweight", "1.0");
            DispatchKeyValue(cc, "fadeInDuration", "3.0");
            DispatchKeyValue(cc, "fadeOutDuration", "3.0");
            DispatchKeyValue(cc, "minfalloff", "-1");
            DispatchKeyValue(cc, "maxfalloff", "-1");
            DispatchKeyValue(cc, "StartDisabled", "1");
            DispatchSpawn(cc);
            ActivateEntity(cc);
            AcceptEntityInput(cc, "Enable");
        }

        g_iFogCC = cc;
    }

    // 递增代际，使之前的 RemovePoisonFog timer 无法再动 m_PlayerFog
    g_iFogGeneration++;

    // 运行时切绿色天空盒
    if (g_hRLoadNamedSkys != INVALID_HANDLE)
    {
        FindConVar("sv_skyname").GetString(g_sOriginalSky, sizeof(g_sOriginalSky));
        SDKCall(g_hRLoadNamedSkys, "sky_green");
    }

    // 全场大风声
    EmitSoundToAll(SFX_WIND, 1, SNDCHAN_STATIC, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

void RemovePoisonFog()
{
    // 还原天空盒
    if (g_sOriginalSky[0] != '\0' && g_hRLoadNamedSkys != INVALID_HANDLE)
        SDKCall(g_hRLoadNamedSkys, g_sOriginalSky);

    // 落地瞬间：停风声（风阻随 STATE_GROUND 自动停）
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsValidClient(i))
            StopSound(i, SNDCHAN_STATIC, SFX_WIND);
    }

    // CC 开始 fade out（fadeOutDuration=5.0 已在创建时设好）
    int cc = g_iFogCC;
    if (cc > 0 && IsValidEntity(cc))
        AcceptEntityInput(cc, "Disable");

    int fog = g_iFog;
    if (fog > 0 && IsValidEntity(fog))
    {
        // fog fade out: lerp fogstart/fogend 拉远到不可见
        SetVariantString("5000");
        AcceptEntityInput(fog, "SetStartDistLerpTo");
        SetVariantString("10000");
        AcceptEntityInput(fog, "SetEndDistLerpTo");
        AcceptEntityInput(fog, "StartFogTransition");
    }

    // 5s 后 fog fade 完毕，Kill 旧 fog 实体
    g_iFogGeneration++;
    DataPack pack = new DataPack();
    pack.WriteCell(g_iFogGeneration);
    pack.WriteCell(EntIndexToEntRef(fog));
    CreateTimer(5.0, Timer_CleanupPoison, pack);

    g_iFog = -1;
    // g_iFogCC = -1;
}

public Action Timer_CleanupPoison(Handle timer, DataPack pack)
{
    pack.Reset();
    int gen = pack.ReadCell();
    int fog = EntRefToEntIndex(pack.ReadCell());
    delete pack;

    if (fog != INVALID_ENT_REFERENCE && IsValidEntity(fog))
    {
        AcceptEntityInput(fog, "TurnOff");

        /**
         * 只有最新一代的清理 timer 才断开 m_PlayerFog 关联
         * 旧 timer 只杀实体，不动玩家 fog 控制器——防止打断新雾的显示
         */
        if (gen == g_iFogGeneration)
        {
            for (int client = 1; client <= MaxClients; client++)
            {
                if (IsValidClient(client))
                    SetEntPropEnt(client, Prop_Send, "m_PlayerFog.m_hCtrl", -1);
            }
        }
        
        AcceptEntityInput(fog, "Kill");
    }

    return Plugin_Stop;
}

//========================================================================================
// 安哥拉技能：飞行增强 — 毒伤
//========================================================================================

void CreatePoisonDamage()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsHumanAlive(client))
            continue;

        float interval = GetRandomFloat(ANGELA_FLY_POISON_MIN, ANGELA_FLY_POISON_MAX);
        g_hPoisonDmg[client] = CreateTimer(5.0+interval, Timer_PoisonDamage, client);
    }
}

void RemovePoisonDamage()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        if (g_hPoisonDmg[client] != INVALID_HANDLE)
        {
            KillTimer(g_hPoisonDmg[client]);
            g_hPoisonDmg[client] = INVALID_HANDLE;
        }
    }
}

public Action Timer_PoisonDamage(Handle timer, int client)
{
    g_hPoisonDmg[client] = INVALID_HANDLE;

    // 玩家已死或离开 → 停止
    if (!IsHumanAlive(client))
        return Plugin_Stop;

    // 毒伤
    float damage = GetRandomFloat(ANGELA_FLY_POISON_MIN, ANGELA_FLY_POISON_MAX);
    SDKHooks_TakeDamage(client, 0, 0, damage, DMG_POISON, _, _, _, false);

    // 随机咳嗽音效
    char sCough[PLATFORM_MAX_PATH];
    Format(sCough, sizeof(sCough), "%s", GetRandomInt(0, 1) ? SFX_COUGH1 : SFX_COUGH2);
    EmitSoundToClient(client, sCough, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 眼前一黑（瞬间黑屏 → fade out）
    Fade(client, 1024, 512, 17, {0, 0, 0, 240});

    // 随机间隔后再次触发
    float interval = GetRandomFloat(ANGELA_FLY_POISON_MIN, ANGELA_FLY_POISON_MAX);
    g_hPoisonDmg[client] = CreateTimer(interval, Timer_PoisonDamage, client);

    return Plugin_Stop;
}

//========================================================================================
// 安哥拉技能：飞行增强 — 刮风推动玩家
//========================================================================================

void WindPush(int client, float vel[3])
{
    float clientPos[3];
    GetClientAbsOrigin(client, clientPos);

    float maxStrength = 0.0;
    float pushDir[3];

    int count = Han_GetZombieCount();
    for (int i = 0; i < count; i++)
    {
        int zombie = Han_GetZombieByIndex(i);
        if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0) continue;
        if (g_BossState[zombie] != STATE_FLYING) continue;

        float angelaPos[3];
        GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", angelaPos);

        float dist = GetVectorDistance(clientPos, angelaPos) * GAMEUNITS_TO_METERS;
        if (dist > ANGELA_FLY_PUSH_RADIUS) continue;

        float strength = ANGELA_FLY_PUSH_POWER * (1.0 - dist / ANGELA_FLY_PUSH_RADIUS);
        if (strength <= maxStrength) continue;

        maxStrength = strength;
        pushDir[0] = clientPos[0] - angelaPos[0];
        pushDir[1] = clientPos[1] - angelaPos[1];
        pushDir[2] = 0.0;
    }

    if (maxStrength <= 0.0) return;

    NormalizeVector(pushDir, pushDir);

    float angles[3];
    GetClientEyeAngles(client, angles);
    angles[0] = 0.0;
    angles[2] = 0.0;

    float fwd[3], right[3];
    GetAngleVectors(angles, fwd, right, NULL_VECTOR);

    float fwdPush = GetVectorDotProduct(pushDir, fwd) * maxStrength;
    float rightPush = GetVectorDotProduct(pushDir, right) * maxStrength;

    vel[0] += fwdPush;
    vel[1] += rightPush;
}

//========================================================================================
// 安哥拉技能：飞行增强 — 垃圾乱飞视觉效果
//========================================================================================

void CreateDebrisSystem(int zombie)
{
    for (int i = 0; i < DEBRIS_SHOOTER_POOL; i++)
    {
        int shooter = CreateEntityByName("env_shooter");
        if (shooter == -1) continue;

        DispatchKeyValue(shooter, "shootmodel", g_sDebrisModels[GetRandomInt(0, DEBRIS_MODEL_COUNT - 1)]);
        DispatchKeyValue(shooter, "shootsounds", "-1");
        DispatchKeyValue(shooter, "spawnflags", "5");
        DispatchKeyValue(shooter, "m_iGibs", "1");
        DispatchKeyValue(shooter, "Simulation", "0");
        DispatchKeyValue(shooter, "m_flVariance", "0");
        DispatchKeyValue(shooter, "gibgravityscale", "1.0");

        char buf[16];
        FloatToString(DEBRIS_VELOCITY, buf, sizeof(buf));
        DispatchKeyValue(shooter, "m_flVelocity", buf);
        FloatToString(DEBRIS_LIFETIME, buf, sizeof(buf));
        DispatchKeyValue(shooter, "m_flGibLife", buf);

        DispatchSpawn(shooter);
        ActivateEntity(shooter);

        g_iDebrisShooter[i] = EntIndexToEntRef(shooter);
    }

    g_iDebrisOwner = zombie;
    g_hDebris = CreateTimer(DEBRIS_INTERVAL, Timer_DebrisTick, _, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

// owner 落地时转移或清理
void RemoveDebrisSystem(int zombie)
{
    int other = FindFlyingAngela(zombie);
    if (other != -1)
    {
        g_iDebrisOwner = other;  // 移交给另一只飞着的安哥拉
        return;
    }

    CleanupDebris();  // 没有其他安哥拉了，关闭 debris 系统
}

// 找另一只飞着的安哥拉（排除 excludeZombie，传 -1 不排除）
int FindFlyingAngela(int excludeZombie)
{
    int count = Han_GetZombieCount();
    for (int i = 0; i < count; i++)
    {
        int other = Han_GetZombieByIndex(i);
        if (other == excludeZombie) continue;
        if (!IsValidEntity(other)) continue;
        if (g_BossState[other] == STATE_FLYING)
            return other;
    }
    return -1;
}

// 销毁所有 shooter + 停止 timer + 重置 owner
void CleanupDebris()
{
    for (int i = 0; i < DEBRIS_SHOOTER_POOL; i++)
    {
        int shooter = EntRefToEntIndex(g_iDebrisShooter[i]);
        if (shooter != INVALID_ENT_REFERENCE && IsValidEntity(shooter))
            AcceptEntityInput(shooter, "Kill");
        g_iDebrisShooter[i] = -1;
    }
    if (g_hDebris != INVALID_HANDLE)
    {
        KillTimer(g_hDebris);
        g_hDebris = INVALID_HANDLE;
    }
    g_iDebrisOwner = -1;
}

public Action Timer_DebrisTick(Handle timer)
{
    if (g_iDebrisOwner == -1)
        return Plugin_Stop;

    // owner 死亡或提前落地（被子弹打下来），找其他安哥拉接替
    if (!IsValidEntity(g_iDebrisOwner) || GetEntProp(g_iDebrisOwner, Prop_Data, "m_iHealth") <= 0
    || g_BossState[g_iDebrisOwner] != STATE_FLYING)
    {
        int other = FindFlyingAngela(-1);
        if (other != -1)
        {
            g_iDebrisOwner = other;
        }
        else
        {
            CleanupDebris();
            return Plugin_Stop;
        }
    }

    // 收集可用 shooter
    int pool[DEBRIS_SHOOTER_POOL];
    int poolSize = 0;
    for (int i = 0; i < DEBRIS_SHOOTER_POOL; i++)
    {
        int s = EntRefToEntIndex(g_iDebrisShooter[i]);
        if (s != INVALID_ENT_REFERENCE && IsValidEntity(s))
            pool[poolSize++] = i;
    }

    float angelaPos[3];
    GetEntPropVector(g_iDebrisOwner, Prop_Send, "m_vecOrigin", angelaPos);

    float range = DEBRIS_SIDE_LENGTH / GAMEUNITS_TO_METERS;
    float base  = DEBRIS_BASE_HEIGHT / GAMEUNITS_TO_METERS;
    float half  = range / 2.0;

    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsHumanAlive(client)) continue;
        if (poolSize == 0) break;

        int r = GetRandomInt(0, poolSize - 1);
        int idx = pool[r];
        pool[r] = pool[--poolSize];

        int shooter = EntRefToEntIndex(g_iDebrisShooter[idx]);
        if (shooter == INVALID_ENT_REFERENCE || !IsValidEntity(shooter))
            continue;

        float center[3];
        GetClientAbsOrigin(client, center);

        // 玩家前方空间（基于 eye yaw），非轴对齐
        float eyeAng[3];
        GetClientEyeAngles(client, eyeAng);
        eyeAng[0] = 0.0;

        float fwd[3], right[3];
        GetAngleVectors(eyeAng, fwd, right, NULL_VECTOR);

        float pos[3];
        pos[0] = center[0] + fwd[0] * GetRandomFloat(0.0, range)
                           + right[0] * GetRandomFloat(-half, half);
        pos[1] = center[1] + fwd[1] * GetRandomFloat(0.0, range)
                           + right[1] * GetRandomFloat(-half, half);
        pos[2] = center[2] + base + GetRandomFloat(0.0, range);

        // 方向 = 安哥拉 → 玩家（水平）
        float ang[3];
        ang[0] = 0.0;
        ang[1] = RadToDeg(ArcTangent2(center[1] - angelaPos[1], center[0] - angelaPos[0]));
        ang[2] = 0.0;

        TeleportEntity(shooter, pos, ang, NULL_VECTOR);
        AcceptEntityInput(shooter, "Shoot");
    }

    return Plugin_Continue;
}
