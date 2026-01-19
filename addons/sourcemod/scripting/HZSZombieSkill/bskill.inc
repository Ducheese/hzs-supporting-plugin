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

    g_hPull[zombie] = CreateTimer(1.0, Timer_PullParticle, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
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
        CloseHandle(pack);

        KillTimer(timer);
        g_hPull[zombie] = INVALID_HANDLE;
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

            if (fTargetDistance*GAMEUNITS_TO_METERS > ANGELA_KNOCK_RANGE)
            {
                continue;
            }

            if (IsZeroPostion(i))
            {
                continue;
            }

            g_bIsStuck[i] = false;

            CreateKnockback(pos, i, view_as<float>({400.0, 400.0, 800.0}));
            SDKHooks_TakeDamage(i, zombie, zombie, ANGELA_KNOCK_DAMAGE, DMG_SLASH, _, _, _, false);

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

            if (fTargetDistance*GAMEUNITS_TO_METERS > ANGELA_KNOCK_RANGE)
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
            SDKHooks_TakeDamage(i, zombie, zombie, ANGELA_KNOCK_DAMAGE, DMG_SLASH, _, _, _, false);

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
        g_hHeal[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 基础信息，结束repeat时要用到
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 自愈过程被打断
    if (g_iBulletCount[zombie] >= ANGELA_HEAL_BULLET)
    {
        g_bIsSkillUsed[zombie] = false;

        Han_UnlockZombie(zombie);

        EmitAmbientSound(SFX_HEAL3, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChatAll("{green}[华仔] {red}%s的自愈被打断，快乘胜追击！", ZombieName);

        KillTimer(timer);
        g_hHeal[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 自愈以至于回满血了
    if (GetEntProp(zombie, Prop_Data, "m_iHealth") == g_iMaxHealth[zombie])
    {
        g_bIsSkillUsed[zombie] = false;

        Han_UnlockZombie(zombie);

        EmitAmbientSound(SFX_HEAL2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
        CPrintToChatAll("{green}[华仔] {red}%s完全自愈了，小心！", ZombieName);

        KillTimer(timer);
        g_hHeal[zombie] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    // 自愈过程中
    SetEntProp(zombie, Prop_Data, "m_iHealth", 
        GetEntProp(zombie, Prop_Data, "m_iHealth") + ANGELA_HEAL_DAMAGE < g_iMaxHealth[zombie] ? 
        GetEntProp(zombie, Prop_Data, "m_iHealth") + ANGELA_HEAL_DAMAGE : 
        g_iMaxHealth[zombie]
    );

    pos[2] += 100.0;
    WriteParticle("z_group_healer_zombie_buff", pos, 1.0);
    EmitAmbientSound(SFX_HEAL1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
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
            CreateTimer(1.5, Timer_CreateGroupKnockback, zombie);              // 2.0秒太长了
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
// 巨型狂暴形态僵尸技能：冲锋
//========================================================================================

void StartCharge(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 冲锋（应该擒抱完后再计CD）
    Han_SetCustomAnimation(zombie, "run_upper_knife", PANGZI_CHARGE_TIME);
    CreateCharge(zombie, victim);

    // 吼叫音效
    EmitAmbientSound(SFX_CHARGE1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}小心，%s向你冲过来了！", ZombieName);
}

void CreateCharge(int zombie, int victim)
{
    // 以下两个句柄的关系比较复杂

    // 冲锋句柄，方向实时调节，震地声同步
    DataPack pack = new DataPack();
    g_hChargePack[zombie] = pack;    // 全局管理，方便清理

    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    pack.WriteCell(0);    // 新增：初始状态位 (0: STEP, 1: FLYGRAVITY)
    g_hCharge[zombie] = CreateTimer(0.5, Timer_ChargeThink, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    
    // 时间到了且未擒抱到人，自动结束冲锋
    CreateTimer(PANGZI_CHARGE_TIME, Timer_EndCharge, zombie);
}

public Action Timer_ChargeThink(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();
    int movetype = pack.ReadCell();   // 读取当前movetype

    // 人没了，关闭冲锋句柄即可（僵尸死亡的处理移到event）
    if (IsZeroPostion(victim) || !IsPlayerAlive(victim))
    {
        ClearChargeHandles(zombie);

        SetEntityMoveType(zombie, MOVETYPE_STEP);
        return Plugin_Stop;
    }

    // 人和僵尸的位置都是时刻在变的
    float pos[3], fTargetOrigin[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", fTargetOrigin);
    float fTargetDistance = GetVectorDistance(pos, fTargetOrigin);

    // 距离够近，则关闭冲锋，开始擒抱逻辑
    if (fTargetDistance*GAMEUNITS_TO_METERS <= PANGZI_GRAPPLE_RANGE)
    {
        ClearChargeHandles(zombie);
        
        SetEntityMoveType(zombie, MOVETYPE_STEP);
        StartGrapple(zombie, victim);
        return Plugin_Stop;
    }

    // NPC僵尸能冲锋的关键，交替变换movetype防止升天
    if (movetype == 0)
    {
        SetEntityMoveType(zombie, MOVETYPE_FLYGRAVITY);
        movetype = 1;
    }
    else
    {
        SetEntityMoveType(zombie, MOVETYPE_STEP);
        movetype = 0;
    }

    // 将更新后的状态写回 DataPack 的第三个位置
    pack.Reset();
    pack.ReadCell();           // 跳过 zombie
    pack.ReadCell();           // 跳过 victim
    pack.WriteCell(movetype);  // 覆盖写入新的movetype

    // 如果不是以上情况，则持续施加向目标的推力
    CreateKnockback(fTargetOrigin, zombie, view_as<float>({PANGZI_CHARGE_FORCE, PANGZI_CHARGE_FORCE, 0.0}));

    // 辅助线（记得删）
    TE_SetupBeamPoints(pos, fTargetOrigin, g_iBeamSprite, 0, 0, 0, 5.0, 2.0, 2.0, 1, 0.0, {255, 0, 0, 255}, 15);
    TE_SendToAll();

    // 震地音效
    EmitAmbientSound(SFX_CHARGE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    
    return Plugin_Continue;
}

public Action Timer_EndCharge(Handle timer, int zombie)
{
    // 没有抱到人，关闭冲锋句柄，进入CD冷却
    if (g_hCharge[zombie] != INVALID_HANDLE) 
    {
        ClearChargeHandles(zombie);    // 清理计时器和 DataPack
        CreateTimer(PANGZI_CHARGE_CD, Timer_SkillEnd, zombie);
    }

    return Plugin_Stop;
}

void ClearChargeHandles(int zombie)
{
    if (g_hCharge[zombie] != INVALID_HANDLE)
    {
        KillTimer(g_hCharge[zombie]);
        g_hCharge[zombie] = INVALID_HANDLE; 
    }

    if (g_hChargePack[zombie] != INVALID_HANDLE)
    {
        delete g_hChargePack[zombie];   // 'delete' 比 'CloseHandle' 更现代安全
        g_hChargePack[zombie] = INVALID_HANDLE;
    }
}

//========================================================================================
// 巨型狂暴形态僵尸技能：擒抱
//========================================================================================

void StartGrapple(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    // 更新数组
    g_iGrappling[zombie] = victim;
    g_bIsGrappled[victim] = true;

    // 更换受害者视角（如果要把人物模型也已经举起来，就要考虑怎么安全的放下来）
    GrappleView(zombie, victim);

    // 擒抱伤害和挣脱共用句柄（目前Bot无法自己挣脱）
    g_iUsePressCount[victim] = 0;    // 初始化按键计数

    DataPack pack = new DataPack();
    g_hGrapplePack[victim] = pack;    // 全局管理，方便清理

    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    
    g_hGrapple[victim] = CreateTimer(1.0, Timer_GrappleThink, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}你被擒抱住了！快连按E键挣脱！");
    CPrintToChatAll("{green}[华仔] {red}%s擒抱住了%s，快去帮忙掩护！", ZombieName, g_ClientName[victim]);
}

void GrappleView(int zombie, int victim)
{
    int viewControl = CreateEntityByName("point_viewcontrol");
    SetVariantString("!activator");
    AcceptEntityInput(viewControl, "SetParent", zombie);
    float viewOffset[3] = {150.0, 0.0, 250.0};    // 这个位置决定了“眼睛”在哪里，可以放在僵尸的肩膀或头顶
    float viewAngles[3] = {15.0, 180.0, 0.0};     // 这个角度决定了“眼睛”朝哪里看，{0, 180, 0} 大致是朝向后方，即僵尸的脸
    TeleportEntity(viewControl, viewOffset, viewAngles, NULL_VECTOR);
    DispatchSpawn(viewControl);
    ActivateEntity(viewControl);
    AcceptEntityInput(viewControl, "Enable");
    SetClientViewEntity(victim, viewControl);

    // SetVariantString("!activator");
    // AcceptEntityInput(victim, "SetParent", zombie);
    // TeleportEntity(victim, viewOffset, viewAngles, NULL_VECTOR);
}

public Action Timer_GrappleThink(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();
    
    if (IsZeroPostion(victim) || !IsPlayerAlive(victim)      // 人没了，关闭擒抱句柄即可（僵尸死亡的处理移到event）
    || g_iUsePressCount[victim] >= PANGZI_GRAPPLE_KEYRATE)   // 或者连按速率达标
    {
        ClearGrappleHandles(victim);
        EndGrapple(zombie, victim);
        CreateTimer(PANGZI_GRAPPLE_CD, Timer_SkillEnd, zombie);
        return Plugin_Stop;
    }

    // 每秒重置计数，意味着必须在1秒内按够次数
    g_iUsePressCount[victim] = 0;

    // 生成擒抱伤害
    SDKHooks_TakeDamage(victim, zombie, zombie, PANGZI_GRAPPLE_DAMAGE, DMG_SLASH, _, _, _, false);

    // 音效
    EmitSoundToClient(victim, SFX_GRAPPLE, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

void ClearGrappleHandles(int victim)
{
    if (g_hGrapple[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hGrapple[victim]);
        g_hGrapple[victim] = INVALID_HANDLE; 
    }

    if (g_hGrapplePack[victim] != INVALID_HANDLE)
    {
        delete g_hGrapplePack[victim];   // 'delete' 比 'CloseHandle' 更现代安全
        g_hGrapplePack[victim] = INVALID_HANDLE;
    }
}

void EndGrapple(int zombie, int victim)
{
    // 数组重置（要在句柄清理之后）
    g_iGrappling[zombie] = -1;
    g_bIsGrappled[victim] = false;

    // 恢复视角
    SetClientViewEntity(victim, victim);

    // 还要解除父子关系，清理实体
}
