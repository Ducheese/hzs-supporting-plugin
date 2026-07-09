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
// 安哥拉技能：飞行逻辑基础
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
    SetEntProp(zombie, Prop_Data, "m_takedamage", 0, 1);     // 飞行期间无敌，避免死在天上不好看
    SetEntityMoveType(zombie, MOVETYPE_STEP);                // 灾变插件伤害钩子设了 MOVETYPE_NONE，需恢复

    g_BossState[zombie] = STATE_FLYING;                      // 切换飞行状态
    g_flLastFlyEnd[zombie] = GetEngineTime();                // 飞行开始计时（共用一个数组）

    CreatePoisonFog(zombie);                                 // 全场绿雾 + CC + 更换天空盒
    CreateDebrisSystem(zombie);                              // 飞行垃圾效果
    StartPoisonDamage();                                     // 每玩家独立毒伤 timer

    OffsetAnimZ(zombie, true);                               // 视觉模型下移，对齐碰撞箱
}

void EnterGroundMode(int zombie)
{
    SetEntProp(zombie, Prop_Data, "m_takedamage", 2, 1);     // 恢复可受伤

    g_BossState[zombie] = STATE_GROUND;                      // 切换地面状态
    g_flLastFlyEnd[zombie] = GetEngineTime();                // 冷却开始计时（共用一个数组）

    RemovePoisonFog(zombie);                                 // 还原绿雾 + CC + 天空盒
    RemoveDebrisSystem(zombie);                              // 停止飞行垃圾效果
    StopPoisonDamage();                                      // 停止所有毒伤 timer

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

    // Debug: 画碰撞箱
    int white[4] = {255, 255, 255, 255};
    DebugDrawBBox(zombie, 0.8, white);
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
        EmitAmbientSound(SFX_FLY, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    }
}

//========================================================================================
// 安哥拉技能：飞行增强 — 毒雾
//========================================================================================

void CreatePoisonFog(int zombie)
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
            if (IsClientInGame(client))
                SetEntPropEnt(client, Prop_Send, "m_PlayerFog.m_hCtrl", fog);
        }
    }

    g_iFog[zombie] = fog;

    // 全场绿色 color_correction
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

    g_iPoisonCC[zombie] = cc;

    // 运行时切绿色天空盒
    if (g_hRLoadNamedSkys != INVALID_HANDLE)
    {
        FindConVar("sv_skyname").GetString(g_sOriginalSky, sizeof(g_sOriginalSky));
        SDKCall(g_hRLoadNamedSkys, "sky_green");
    }

    // 全场大风声
    EmitSoundToAll(SFX_WIND, 1, SNDCHAN_STATIC, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

void RemovePoisonFog(int zombie)
{
    // 还原天空盒
    if (g_sOriginalSky[0] != '\0' && g_hRLoadNamedSkys != INVALID_HANDLE)
        SDKCall(g_hRLoadNamedSkys, g_sOriginalSky);

    // 落地瞬间：停风声（风阻随 STATE_GROUND 自动停）
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
            StopSound(i, SNDCHAN_STATIC, SFX_WIND);
    }

    // CC 开始 fade out（fadeOutDuration=5.0 已在创建时设好）
    int cc = g_iPoisonCC[zombie];
    if (cc > 0 && IsValidEntity(cc))
        AcceptEntityInput(cc, "Disable");

    int fog = g_iFog[zombie];
    if (fog > 0 && IsValidEntity(fog))
    {
        // fog fade out: lerp fogstart/fogend 拉远到不可见
        SetVariantString("5000");
        AcceptEntityInput(fog, "SetStartDistLerpTo");
        SetVariantString("10000");
        AcceptEntityInput(fog, "SetEndDistLerpTo");
        AcceptEntityInput(fog, "StartFogTransition");
    }

    // 5s 后 fog+CC fade 完毕，统一 Kill
    DataPack pack = new DataPack();
    pack.WriteCell(EntIndexToEntRef(fog));
    pack.WriteCell(EntIndexToEntRef(cc));
    CreateTimer(5.0, Timer_CleanupPoison, pack);

    g_iFog[zombie] = -1;
    g_iPoisonCC[zombie] = -1;
}

public Action Timer_CleanupPoison(Handle timer, DataPack pack)
{
    pack.Reset();
    int fog = EntRefToEntIndex(pack.ReadCell());
    int cc = EntRefToEntIndex(pack.ReadCell());
    delete pack;

    if (fog != INVALID_ENT_REFERENCE && IsValidEntity(fog))
    {
        AcceptEntityInput(fog, "TurnOff");
        for (int client = 1; client <= MaxClients; client++)
        {
            if (IsClientInGame(client))
                SetEntPropEnt(client, Prop_Send, "m_PlayerFog.m_hCtrl", -1);
        }
        AcceptEntityInput(fog, "Kill");
    }

    if (cc != INVALID_ENT_REFERENCE && IsValidEntity(cc))
        AcceptEntityInput(cc, "Kill");

    return Plugin_Stop;
}

//========================================================================================
// 安哥拉技能：飞行增强 — 毒伤
//========================================================================================

void StartPoisonDamage()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsHumanAlive(client))
            continue;

        float interval = GetRandomFloat(ANGELA_FLY_POISON_MIN, ANGELA_FLY_POISON_MAX);
        g_hPoisonDmg[client] = CreateTimer(5.0+interval, Timer_PoisonDamage, client);
    }
}

void StopPoisonDamage()
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

void ApplyWindPush(int client, float vel[3])
{
    float clientPos[3];
    GetClientAbsOrigin(client, clientPos);

    float maxStrength = 0.0;
    float pushDir[3];

    int count = Han_GetZombieCount();
    for (int i = 0; i < count; i++)
    {
        int zombie = Han_GetZombieByIndex(i);
        if (!IsValidEntity(zombie)) continue;
        if (g_BossState[zombie] != STATE_FLYING) continue;

        float angelaPos[3];
        GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", angelaPos);

        float dist = GetVectorDistance(clientPos, angelaPos) * GAMEUNITS_TO_METERS;
        if (dist > ANGELA_FLY_PUSH_RANGE) continue;

        float strength = ANGELA_FLY_PUSH_POWER * (1.0 - dist / ANGELA_FLY_PUSH_RANGE);
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

    g_hDebris[zombie] = CreateTimer(DEBRIS_INTERVAL, Timer_DebrisTick, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

void RemoveDebrisSystem(int zombie)
{
    if (g_hDebris[zombie] != INVALID_HANDLE)
    {
        KillTimer(g_hDebris[zombie]);
        g_hDebris[zombie] = INVALID_HANDLE;
    }

    for (int i = 0; i < DEBRIS_SHOOTER_POOL; i++)
    {
        int shooter = EntRefToEntIndex(g_iDebrisShooter[i]);
        if (shooter != INVALID_ENT_REFERENCE && IsValidEntity(shooter))
            AcceptEntityInput(shooter, "Kill");
        g_iDebrisShooter[i] = -1;
    }
}

public Action Timer_DebrisTick(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        g_hDebris[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
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
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", angelaPos);

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