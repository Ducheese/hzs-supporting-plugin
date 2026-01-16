//========================================================================================
// 安哥拉技能：全场吸力，半血以上受击有概率使用
//========================================================================================

void CreatePull(int zombie, float pos[3])
{
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsValidClient(i, true) && !IsZeroPostion(i))    // 如果刚复活就不会被吸，毕竟还要买武器，降低难度吧
        {
            g_bIsStuck[i] = false;                // 解除憎恶屠夫的控制
            g_iZombiePull[i] = zombie;            // 记录安哥拉实体号

            CreateTimer(5.0, Timer_DePull, i);
        }
    }

    DataPack pack = new DataPack();
    pack.WriteCell(zombie);
    pack.WriteCell(pos[0]);
    pack.WriteCell(pos[1]);
    pack.WriteCell(pos[2]);

    RepeatTask2[zombie] = CreateTimer(1.0, Timer_PullParticle, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    EmitAmbientSound(SFX_BREATH_PULL, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

public Action Timer_PullParticle(Handle timer, DataPack pack)
{
    float pos[3];

    pack.Reset();
    int zombie = pack.ReadCell();
    pos[0] = pack.ReadCell();
    pos[1] = pack.ReadCell();
    pos[2] = pack.ReadCell();

    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        CloseHandle(pack);            // repeat timer不能提前closehandle

        KillTimer(timer);
        RepeatTask2[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 技能结束
    if (!g_bIsSkillUsed[zombie])
    {
        CloseHandle(pack);

        KillTimer(timer);
        RepeatTask2[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    WriteParticle("z_shield_buff", pos, 1.0);

    return Plugin_Continue;
}

public Action Timer_DePull(Handle timer, int client)
{
    g_iZombiePull[client] = -1;

    return Plugin_Continue;
}

//========================================================================================
// 安哥拉技能：群体击飞，一个砸地飞高，一个挥臂飞远
//========================================================================================

public Action Timer_CreateGroupKnockback(Handle timer, int zombie)
{
    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        return Plugin_Continue;
    }

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 挨个击飞
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsValidClient(i, true))
        {
            float fTargetOrigin[3];
            GetEntPropVector(i, Prop_Send, "m_vecOrigin", fTargetOrigin);

            float fTargetDistance = GetVectorDistance(pos, fTargetOrigin);

            if (fTargetDistance*GAMEUNITS_TO_METERS > GroupKnockRange)
            {
                continue;
            }

            if (IsZeroPostion(i))
            {
                continue;
            }

            g_bIsStuck[i] = false;

            CreateKnockback(pos, i, view_as<float>({400.0, 400.0, 800.0}));
            SDKHooks_TakeDamage(i, zombie, zombie, GroupKnockDamage, DMG_SLASH, _, _, _, false);

            // 补上音效和shake
            Shake(i, 30.0, 10.0, 3.0);
            EmitSoundToClient(i, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        }            
    }

    EmitAmbientSound(SFX_SMASH, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 特效？试试看————卧槽好帅
    pos[2]+= 20.0;
    TE_SetupBeamRingPoint(pos, 10.0, 1000.0, PrecacheModel("materials/sprites/lgtning.vmt"), PrecacheModel("materials/sprites/halo01.vmt"), 1, 1, 0.2, 100.0, 1.0, {255,75,75,255}, 0, 0);
    TE_SendToAll();

    return Plugin_Continue;
}

public Action Timer_CreateGroupKnockback2(Handle timer, DataPack pack)
{
    float ang[3];

    pack.Reset();
    int zombie = pack.ReadCell();
    ang[0] = pack.ReadCell();
    ang[1] = pack.ReadCell();
    ang[2] = pack.ReadCell();
    CloseHandle(pack);

    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        return Plugin_Continue;
    }

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 挨个击飞
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsValidClient(i, true))
        {
            float fTargetOrigin[3];
            GetEntPropVector(i, Prop_Send, "m_vecOrigin", fTargetOrigin);

            float fTargetDistance = GetVectorDistance(pos, fTargetOrigin);

            if (fTargetDistance*GAMEUNITS_TO_METERS > GroupKnockRange)
            {
                continue;
            }

            if (!IsTargetForward(pos, fTargetOrigin, ang))
            {
                continue;
            }

            if (IsZeroPostion(i))
            {
                continue;
            }
            
            g_bIsStuck[i] = false;

            CreateKnockback(pos, i, view_as<float>({800.0, 800.0, 400.0}));
            SDKHooks_TakeDamage(i, zombie, zombie, GroupKnockDamage, DMG_SLASH, _, _, _, false);

            // 补上音效和shake
            Shake(i, 30.0, 10.0, 3.0);
            EmitSoundToClient(i, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        }            
    }

    EmitAmbientSound(SFX_SWING, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

//========================================================================================
// 安哥拉技能：呼唤僵尸集中攻击一个人类
//========================================================================================

void CreateZombieCall(int victim, float dist, float duration)
{
    float fTargetOrigin[3], fZombieOrigin[3];
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", fTargetOrigin);

    float fTargetDistance;

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

            GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", fZombieOrigin);
            fTargetDistance = GetVectorDistance(fTargetOrigin, fZombieOrigin);

            if (fTargetDistance*GAMEUNITS_TO_METERS > dist)
            {
                continue;
            }

            Han_SetZombieTarget(zombie, victim, duration);   // 不需要特别Han_UnlockZombie
        }
    }

    CreateTimer(duration, Timer_DeZombieCall, victim);

    // 发出声音的实体填了1，如果写横线，大灾变插件里的soundhook会报错
    EmitSoundToAll(SFX_CALL, 1, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

public Action Timer_DeZombieCall(Handle timer, int client)
{
    g_iZombieCall[client] = -1;

    return Plugin_Continue;
}

//========================================================================================
// 安哥拉技能：可以被打断的自愈，半血以下受击有概率使用
//========================================================================================

public Action Timer_ZombieSelfHealing(Handle timer, int zombie)
{
    // 僵尸状态验证
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        RepeatTask[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 基础信息，结束repeat时要用到
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 自愈过程被打断
    if (g_iBulletCount[zombie] >= BulletRequirement)
    {
        g_bIsSkillUsed[zombie] = false;

        Han_UnlockZombie(zombie);

        EmitAmbientSound(SFX_BREATH_HEAL_FAIL, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChatAll("{green}[华仔] {red}%s的自愈被打断，快乘胜追击！", ZombieName);

        KillTimer(timer);
        RepeatTask[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 自愈以至于回满血了
    if (GetEntProp(zombie, Prop_Data, "m_iHealth") == g_iMaxHealth[zombie])
    {
        g_bIsSkillUsed[zombie] = false;

        Han_UnlockZombie(zombie);

        EmitAmbientSound(SFX_BREATH_HEAL_FULL, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChatAll("{green}[华仔] {red}%s完全自愈了，小心！", ZombieName);

        KillTimer(timer);
        RepeatTask[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 自愈过程中
    SetEntProp(zombie, Prop_Data, "m_iHealth", GetEntProp(zombie, Prop_Data, "m_iHealth") + SelfHealDamage < g_iMaxHealth[zombie] ? GetEntProp(zombie, Prop_Data, "m_iHealth") + SelfHealDamage : g_iMaxHealth[zombie]);

    pos[2] += 100.0;
    WriteParticle("z_group_healer_zombie_buff", pos, 1.0);
    EmitAmbientSound(SFX_BREATH_HEAL_ING, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

//========================================================================================
// 安哥拉技能：飞行避险喷毒，半血以下有飞行CD就使用
//========================================================================================

void InitFlyingState(int zombie)
{
    g_BossState[zombie] = STATE_GROUND;           // 默认在地上
    g_Mode[zombie] = FLY_RANDOM;                  // 默认高飞模式

    g_flLastFlySound[zombie] = GetEngineTime();
}

public Action Timer_ZombieFlyingThink(Handle timer, int zombie)
{
    // 实体无效或死亡，终止timer
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        RepeatTask3[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }
    
    float currentTime = GetEngineTime();    

    // 如果当前是地面模式，检查冷却是否结束
    if (g_BossState[zombie] == STATE_GROUND)
    {
        if (currentTime - g_flLastFlyEnd[zombie] >= FlyCDTime)      // 飞行的CD
        {
            if (!g_bIsSkillUsed[zombie])                            // 正在使用其它技能时无法飞行，避免冲突
                EnterFlyingMode(zombie);                            // 有CD立刻起飞
        }
        else
        {
            // 去掉了在地面上的喷毒
        }
    }
    // 如果当前是飞行模式，检查持续时间
    else if (g_BossState[zombie] == STATE_FLYING)
    {
        if (currentTime - g_flLastFlyEnd[zombie] >= FlyDuration)    // 飞行持续时间
        {
            EnterGroundMode(zombie);
        }
        else
        {
            ZombieFlyingLogic(zombie);
        }
    }

    return Plugin_Continue;
}

void EnterFlyingMode(int zombie)
{
    g_bIsSkillUsed[zombie] = true;                           // 其它技能无法使用
    
    SetEntProp(zombie, Prop_Data, "m_takedamage", 0, 1);     // 飞的时候无敌，避免死在天上不好看

    g_BossState[zombie] = STATE_FLYING;
    g_flLastFlyEnd[zombie] = GetEngineTime();                  // 飞行开始计时（共用一个数组）
}

void EnterGroundMode(int zombie)
{
    Han_SetCustomAnimationEx(zombie, "Walk_shoot_knife3", 4.5);        // 新api，强制播放，无视旧动画是否播完
    SetEntProp(zombie, Prop_Data, "m_takedamage", 2, 1);

    CreateTimer(1.5, Timer_CreateGroupKnockback, zombie);              // 2.0秒太长了
    CreateTimer(5.0, Timer_SkillEnd, zombie);

    g_BossState[zombie] = STATE_GROUND;
    g_flLastFlyEnd[zombie] = GetEngineTime();                            // 冷却开始计时（共用一个数组）
}

void ZombieFlyingLogic(int zombie)
{
    int target = GetEntDataEnt2(zombie, g_iLeaderOffset);      // target可能为0

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 射线检测
    float dist, hitPos[3], FlyForce[3];
    GetDistanceToGround(zombie, dist, hitPos);          // 得到dist和hitPos，都有用
    GetFlyingMode(zombie, target, hitPos);              // 根据距离决定高飞低飞，允许target为0

    // PrintToChatAll("高度：%f", dist);
    
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
    float currentTime = GetEngineTime();

    if (currentTime - g_flLastFlySound[zombie] >= 0.8)
    {
        g_flLastFlySound[zombie] = currentTime;
        
        Han_SetCustomAnimationEx(zombie, "run_upper_knife", 0.9);
        EmitAmbientSound(SFX_FLY, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL)
    }
}

void GetDistanceToGround(int zombie, float &dist, float fRayEndPoint[3])
{
    float fRayStartPoint[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", fRayStartPoint);

    Handle trace = TR_TraceRayFilterEx(fRayStartPoint, {90.0, 0.0, 0.0}, MASK_PLAYERSOLID, RayType_Infinite, TracePlayerFilter, zombie);      // 方向向下
    TR_GetEndPosition(fRayEndPoint, trace);

    dist = fRayStartPoint[2] - fRayEndPoint[2];
}

public bool TracePlayerFilter(int entity, int mask, any data)
{
    if (entity == data || (entity >= 1 && entity <= MaxClients))     // 忽略自身，以及人类client实体
    {
        return false;
    }

    return true;
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

/* void CreatePoison(float pos[3])
{
    int ent = CreateEntityByName("env_smokestack");        // 参考https://forums.alliedmods.net/showthread.php?p=1280363

    if (ent != -1)
    {
        char Origin[32];
        Format(Origin, sizeof(Origin), "%f %f %f", pos[0], pos[1], pos[2]);

        DispatchKeyValue(ent, "SmokeMaterial", "particle/particle_smokegrenade.vmt");
        DispatchKeyValue(ent, "Origin", Origin);
        DispatchKeyValue(ent, "BaseSpread", "100");        // Amount of random spread in the origins of the smoke particles when they're spawned.
        DispatchKeyValue(ent, "SpreadSpeed", "70");        // Amount of random spread in the velocity of the smoke particles after they're spawned.
        DispatchKeyValue(ent, "Speed", "80");              // The speed at which the smoke particles move after they're spawned.
        DispatchKeyValue(ent, "StartSize", "200");         // Size of the smoke particles when they're first emitted.
        DispatchKeyValue(ent, "EndSize", "2");             // Size of the smoke particles at the point they fade out completely.
        DispatchKeyValue(ent, "Rate", "30");               // Rate at which to emit smoke particles (i.e. particles to emit per second).
        DispatchKeyValue(ent, "JetLength", "400");         // Length of the smokestack. Lifetime of the smoke particles is derived from this & particle speed.
        DispatchKeyValue(ent, "Twist", "20");              // The amount, in degrees per second, that the smoke particles twist around the origin.
        DispatchKeyValue(ent, "RenderColor", "0 255 0");   // 颜色
        DispatchKeyValue(ent, "RenderAmt", "128");         // 透明度

        DispatchKeyValue(ent, "WindAngle", Origin);

        DispatchSpawn(ent);
        AcceptEntityInput(ent, "TurnOn");
    }

    EmitAmbientSound(SFX_POISON, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
} */

//========================================================================================
// 巨型狂暴形态僵尸技能：冲刺擒抱
//========================================================================================

void CreateCharge(int zombie, int victim)
{
    // npc僵尸能能冲刺的关键
    SetEntityMoveType(zombie, MOVETYPE_FLYGRAVITY);

    // 冲刺方向实时调节，脚步声同步 (每0.5秒执行一次)
    DataPack pack = new DataPack();
    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    RepeatTask4[zombie] = CreateTimer(0.5, Timer_ChargeThink, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    
    // 时间到了且未擒抱到人，自动结束冲刺
    CreateTimer(ChargeDuration, Timer_EndCharge, zombie);

    // 吼叫音效
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    EmitAmbientSound(SFX_CHARGE_HOWL, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

public Action Timer_EndCharge(Handle timer, int zombie)
{
    if (RepeatTask4[zombie] != INVALID_HANDLE)    // 没有抱到人，关闭冲刺句柄，进入CD冷却
    {
        CreateTimer(ChargeCDTime, Timer_SkillEnd, zombie);   // 擒抱完后再进CD

        CloseHandle(RepeatTask4[zombie]);
        RepeatTask4[zombie] = INVALID_HANDLE;

        SetEntityMoveType(zombie, MOVETYPE_STEP);
    }

    return Plugin_Continue;
}

public Action Timer_ChargeThink(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();

    // 验证合法性
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0 || !IsValidClient(victim, true))     // 没抱到人，关闭冲刺句柄即可
    {
        CloseHandle(pack);            // repeat timer不能提前closehandle

        KillTimer(timer);
        RepeatTask4[zombie] = INVALID_HANDLE;

        SetEntityMoveType(zombie, MOVETYPE_STEP);
        return Plugin_Continue;
    }

    float pos[3], fTargetOrigin[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", fTargetOrigin);
    float fTargetDistance = GetVectorDistance(pos, fTargetOrigin);

    // 触发擒抱
    if (fTargetDistance*GAMEUNITS_TO_METERS <= GrappleRange)     // 关闭冲刺，开始擒抱逻辑
    {
        StartGrapple(zombie, victim);

        CloseHandle(pack);            // repeat timer不能提前closehandle

        KillTimer(timer);
        RepeatTask4[zombie] = INVALID_HANDLE;

        SetEntityMoveType(zombie, MOVETYPE_STEP);
        return Plugin_Continue;
    }

    // 如果不是，持续施加向目标的推力
    float dir[3], force[3];
    MakeVectorFromPoints(pos, fTargetOrigin, dir);
    NormalizeVector(dir, force);
    ScaleVector(force, ChargeForce);

    TeleportEntity(zombie, NULL_VECTOR, NULL_VECTOR, force);

    // 震地音效
    EmitAmbientSound(SFX_CHARGE_SHAKE, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    
    return Plugin_Continue;
}

void StartGrapple(int zombie, int victim)
{
    // 数组更新
    g_iGrappling[zombie] = victim;
    g_bIsGrappled[victim] = true;

    // 更换受害者视角
    int viewControl = CreateEntityByName("point_viewcontrol");
    SetVariantString("!activator");
    AcceptEntityInput(viewControl, "SetParent", zombie);
    float viewOffset[3] = {150.0, 0.0, 250.0};    // 这个位置决定了“眼睛”在哪里，可以放在僵尸的肩膀或头顶
    float viewAngles[3] = {15.0, 180.0, 0.0};    // 这个角度决定了“眼睛”朝哪里看，{0, 180, 0} 大致是朝向后方，即僵尸的脸
    TeleportEntity(viewControl, viewOffset, viewAngles, NULL_VECTOR);
    DispatchSpawn(viewControl);
    ActivateEntity(viewControl);
    AcceptEntityInput(viewControl, "Enable");
    SetClientViewEntity(victim, viewControl);

    // 启动伤害计时器（无需管挣脱，只看生死）
    DataPack pack = new DataPack();
    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    RepeatTask5[victim] = CreateTimer(1.0, Timer_GrappleDamage, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 处理挣脱逻辑
    if (IsFakeClient(victim))
    {
        CreateTimer(GetRandomFloat(1.0, 5.0), Timer_BotEscape, pack);      // 这里重用pack逻辑需谨慎，建议重新打包
    }
    else
    {
        g_iUsePressCount[victim] = 0;   // 初始化
        RepeatTask6[victim] = CreateTimer(1.0, Timer_CheckPlayerEscape, pack, TIMER_REPEAT);

        CPrintToChat(victim, "{green}[华仔] {red}你被擒抱住了！快连按E键挣脱！");
    }

    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));
    CPrintToChatAll("{green}[华仔] {red}%s擒抱住了%s，快去帮忙掩护！", ZombieName, g_ClientName[victim]);
}

public Action Timer_GrappleDamage(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();

    // 不负责验证合法性，交给Timer_CheckPlayerEscape处理

    // 伤害和音效
    SDKHooks_TakeDamage(victim, zombie, zombie, GrappleDamage, DMG_SLASH, _, _, _, false);
    EmitSoundToClient(victim, SFX_GRAPPLE_HURT, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

public Action Timer_CheckPlayerEscape(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();

    // 验证合法性
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0 || !IsValidClient(victim, true))
    {
        EndGrapple(zombie, victim);

        CloseHandle(pack);            // repeat timer不能提前closehandle

        CloseHandle(RepeatTask5[victim]);
        RepeatTask5[victim] = INVALID_HANDLE;

        KillTimer(timer);
        RepeatTask6[victim] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 连按速率达标
    if (g_iUsePressCount[victim] >= EscapeKeyPressRate)
    {
        EndGrapple(zombie, victim);

        CloseHandle(pack);            // repeat timer不能提前closehandle

        CloseHandle(RepeatTask5[victim]);
        RepeatTask5[victim] = INVALID_HANDLE;

        KillTimer(timer);
        RepeatTask6[victim] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    g_iUsePressCount[victim] = 0;     // 每秒重置计数，意味着必须在1秒内按够次数

    return Plugin_Continue;
}

public Action Timer_BotEscape(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();

    CloseHandle(pack);            // repeat timer不能提前closehandle

    CloseHandle(RepeatTask5[victim]);
    RepeatTask5[victim] = INVALID_HANDLE;

    EndGrapple(zombie, victim);

    return Plugin_Continue;
}

void EndGrapple(int zombie, int victim)
{
    g_iGrappling[zombie] = -1;
    g_bIsGrappled[victim] = false;
    SetClientViewEntity(victim, victim);  // 恢复视角
    CreateTimer(ChargeCDTime, Timer_SkillEnd, zombie);
}
