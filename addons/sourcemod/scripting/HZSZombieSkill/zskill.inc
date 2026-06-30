//========================================================================================
// 迷雾僵尸技能：受击后立刻产生大量黑烟，阻碍真人玩家视线，一次性技能
//========================================================================================

void StartSmoke(int zombie)
{
    // 普通体型僵尸参考身高（来自[H-AN-CSS]ZombieScenario.sp）
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    pos[2] += 55.0;

    // 生成黑烟
    CreateSmoke(pos);

    // 抽烟音效
    EmitAmbientSound(SFX_SMOKE1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    EmitAmbientSound(SFX_SMOKE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

void CreateSmoke(float pos[3])
{
    // 生成爆炸实体
    int ent = CreateEntityByName("env_smokestack");        // 参考https://forums.alliedmods.net/showthread.php?p=1280363

    if (ent != -1)
    {
        char Origin[32];
        Format(Origin, sizeof(Origin), "%f %f %f", pos[0], pos[1], pos[2]);

        DispatchKeyValue(ent, "Origin", Origin);
        DispatchKeyValue(ent, "SmokeMaterial", "particle/particle_smokegrenade.vmt");
        DispatchKeyValue(ent, "BaseSpread", "100");        // Amount of random spread in the origins of the smoke particles when they're spawned.
        DispatchKeyValue(ent, "SpreadSpeed", "70");        // Amount of random spread in the velocity of the smoke particles after they're spawned.
        DispatchKeyValue(ent, "Speed", "80");              // The speed at which the smoke particles move after they're spawned.
        DispatchKeyValue(ent, "StartSize", "200");         // Size of the smoke particles when they're first emitted.
        DispatchKeyValue(ent, "EndSize", "2");             // Size of the smoke particles at the point they fade out completely.
        DispatchKeyValue(ent, "Rate", "30");               // Rate at which to emit smoke particles (i.e. particles to emit per second).
        DispatchKeyValue(ent, "JetLength", "400");         // Length of the smokestack. Lifetime of the smoke particles is derived from this & particle speed.
        DispatchKeyValue(ent, "Twist", "20");              // The amount, in degrees per second, that the smoke particles twist around the origin.
        DispatchKeyValue(ent, "RenderColor", "0 0 0");     // 颜色
        DispatchKeyValue(ent, "RenderAmt", "255");         // 透明度
        DispatchSpawn(ent);

        AcceptEntityInput(ent, "TurnOn");

        CreateTimer(SMOKE_FADETIME, Timer_StopSmoke, ent);
        CreateTimer(SMOKE_KILLTIME, Timer_KillSmoke, ent);
    }
}

public Action Timer_StopSmoke(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        AcceptEntityInput(entity, "TurnOff");        // 烟开始消散
    }

    return Plugin_Stop;
}

public Action Timer_KillSmoke(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        AcceptEntityInput(entity, "Kill");           // 烟彻底消散
    }

    return Plugin_Stop;
}

//========================================================================================
// 自爆僵尸技能：死亡后延时生成爆炸实体，伤害可观，一次性技能
//========================================================================================

void StartExplosion(int zombie)
{
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    DataPack pack = new DataPack();
    pack.WriteCell(zombie);
    pack.WriteCell(pos[0]);
    pack.WriteCell(pos[1]);
    pack.WriteCell(pos[2]);

    // 死了1.5秒后，不见得m_vecOrigin属性值还在，所以用pack包装了下参数
    CreateTimer(EXPLODE_TIME, Timer_CreateExplosion, pack);

    // 自爆僵尸死亡时的音效，我用的是低吼音效
    EmitAmbientSound(SFX_EXPLODE1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

public Action Timer_CreateExplosion(Handle timer, DataPack pack)
{
    float pos[3];

    pack.Reset();
    int zombie = pack.ReadCell();
    pos[0] = pack.ReadCell();
    pos[1] = pack.ReadCell();
    pos[2] = pack.ReadCell();
    delete pack;

    // 生成爆炸实体
    int ent = CreateEntityByName("env_explosion");

    if (ent != -1)
    {
        char Origin[32];
        Format(Origin, sizeof(Origin), "%f %f %f", pos[0], pos[1], pos[2]);

        DispatchKeyValue(ent, "Origin", Origin);
        DispatchKeyValue(ent, "iMagnitude", EXPLODE_DAMAGE);
        DispatchKeyValue(ent, "iRadiusOverride", EXPLODE_RANGE);
        DispatchSpawn(ent);

        AcceptEntityInput(ent, "Explode");                                // 实体会自己删除，不需要特别去kill

        TE_SetupSparks(pos, view_as<float>({0.0, 0.0, 2.0}), 600, 8);     // 更多的火花效果（参考Rocket HE_ZDT.sp）
        TE_SendToAll();

        if (IsValidEntity(zombie))
            SetEntPropEnt(ent, Prop_Send, "m_hOwnerEntity", zombie);      // 希望爆炸杀伤能返回有效的killer（实际无用）
    }

    // 爆炸音效
    EmitAmbientSound(SFX_EXPLODE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Stop;
}

//========================================================================================
// 治疗僵尸技能：自出生起，每隔几秒为周围僵尸恢复固定血量，无限次数技能
//========================================================================================

public Action Timer_RepeatHealing(Handle timer, int entity)
{
    if (!IsValidEntity(entity) || GetEntProp(entity, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        g_hHeal[entity] = INVALID_HANDLE;
        return Plugin_Stop;
    }

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

            if (GetMeterDistance(zombie, entity) > HEAL_RANGE)
            {
                continue;
            }

            SetEntProp(zombie, Prop_Data, "m_iHealth", 
                GetEntProp(zombie, Prop_Data, "m_iHealth") + HEAL_DAMAGE < g_iMaxHealth[zombie] ? 
                GetEntProp(zombie, Prop_Data, "m_iHealth") + HEAL_DAMAGE : 
                g_iMaxHealth[zombie]
            );
        }
    }

    float pos[3];
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", pos);
    pos[2] += 50.0;
    WriteParticle("z_group_healer_zombie_buff", pos, 0.1);     // 即便缩短时间，依然有点太密集了

    return Plugin_Continue;
}

//========================================================================================
// 恶魔猎手技能：击飞玩家，无限次数技能
//========================================================================================

void StartKnockback(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    // 生成击退
    Shake(victim, 30.0, 10.0, 3.0);
    CreateKnockback(zombie, victim, view_as<float>({DEIMOS_KNOCK_POWER, DEIMOS_KNOCK_POWER, DEIMOS_KNOCK_POWER}));

    // 击退音效
    EmitSoundToClient(victim, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}你被%s击飞了！", ZombieName);
}

void CreateKnockback(int zombie, int target, float strength[3])
{
    float pos[3], fTargetOrigin[3], fTempPoints[3], fTempAngles[3];
    float vReturn[3];

    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    GetEntPropVector(target, Prop_Send, "m_vecOrigin", fTargetOrigin);     // target指的是人类

    MakeVectorFromPoints(pos, fTargetOrigin, fTempPoints);
    GetVectorAngles(fTempPoints, fTempAngles);

    vReturn[0] = Cosine(DegToRad(fTempAngles[1])) * strength[0];
    vReturn[1] = Sine(DegToRad(fTempAngles[1])) * strength[1];
    vReturn[2] = strength[2];

    TeleportEntity(target, NULL_VECTOR, NULL_VECTOR, vReturn);
}

//========================================================================================
// 憎恶屠夫技能：死后生成鬼手陷阱
//========================================================================================

void StartTrap(int zombie)
{
    // 基础信息
    float deathPos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", deathPos);

    // 射线检测地面坐标（z坐标可能不可信，如半空中死亡）
    float groundPos[3];
    Handle trace = TR_TraceRayFilterEx(deathPos, {90.0, 0.0, 0.0}, MASK_SOLID, RayType_Infinite, TraceEntityFilter_Ground, 0);
    TR_GetEndPosition(groundPos, trace);
    delete trace;

    // 生成陷阱
    CreateTrap(groundPos);
}

void CreateTrap(float pos[3])
{
    // 视觉模型
    int visual = CreateEntityByName("prop_dynamic");
    if (visual == -1)
        return;

    DispatchKeyValue(visual, "model", ZOMBIE_TRAP);
    DispatchKeyValue(visual, "disableshadows", "1");
    DispatchKeyValue(visual, "disableshadowdepth", "1");
    DispatchSpawn(visual);
    SetVariantString("idle");
    AcceptEntityInput(visual, "SetAnimation");

    TeleportEntity(visual, pos, NULL_VECTOR, NULL_VECTOR);

    // 受击体
    int phys = CreateEntityByName("prop_physics_override");
    if (phys == -1)
    {
        AcceptEntityInput(visual, "Kill");
        return;
    }

    DispatchKeyValue(phys, "model", TOOL_TRAPPHYS);
    DispatchKeyValue(phys, "modelscale", "0.5");
    DispatchSpawn(phys);
    AcceptEntityInput(phys, "DisableMotion");
    SetEntProp(phys, Prop_Send, "m_fEffects", GetEntProp(phys, Prop_Send, "m_fEffects") | 32);
    SetEntProp(phys, Prop_Data, "m_CollisionGroup", 1);

    float physPos[3];
    physPos[0] = pos[0]; physPos[1] = pos[1]; physPos[2] = pos[2] - 16.0;   // 半嵌入地底，不然碰撞区域太高了
    TeleportEntity(phys, physPos, NULL_VECTOR, NULL_VECTOR);

    SetEntProp(phys, Prop_Data, "m_iHealth", BUTCHER_TRAP_HEALTH);
    SetEntProp(phys, Prop_Data, "m_takedamage", 2);
    SDKHook(phys, SDKHook_OnTakeDamage, OnTrapTakeDamage);

    // 触发器
    int trigger = CreateEntityByName("trigger_multiple");
    if (trigger == -1)
    {
        AcceptEntityInput(visual, "Kill");
        AcceptEntityInput(phys, "Kill");
        return;
    }

    DispatchKeyValue(trigger, "spawnflags", "1");
    DispatchKeyValue(trigger, "wait", "0");
    DispatchSpawn(trigger);
    ActivateEntity(trigger);

    SetEntProp(trigger, Prop_Send, "m_nSolidType",   2);    // 必要的
    SetEntProp(trigger, Prop_Send, "m_usSolidFlags", 12);   // 必要的

    float trigMins[3] = {-24.0, -24.0,  0.0};
    float trigMaxs[3] = { 24.0,  24.0, 16.0};
    SetEntPropVector(trigger, Prop_Send, "m_vecMins", trigMins);
    SetEntPropVector(trigger, Prop_Send, "m_vecMaxs", trigMaxs);

    float trigPos[3];
    trigPos[0] = pos[0]; trigPos[1] = pos[1]; trigPos[2] = pos[2] + 8.0;    // 解决斜坡上陷阱不触发的问题
    TeleportEntity(trigger, trigPos, NULL_VECTOR, NULL_VECTOR);

    SDKHook(trigger, SDKHook_Touch, OnTrapTriggerTouch);

    // 映射
    g_iTrapVisual[phys]  = visual;
    g_iTrapTrigger[phys] = trigger;
    g_iTriggerOwner[trigger] = phys;
    g_iTrapVictim[phys] = -1;
    g_iTrapCount++;
    g_hTrapLifetime[phys] = CreateTimer(BUTCHER_TRAP_LIFETIME, Timer_DestroyTrap, phys);

    // DebugDrawBBox(phys,    BUTCHER_TRAP_LIFETIME, {255, 0, 0, 255});
    // DebugDrawBBox(trigger, BUTCHER_TRAP_LIFETIME, {0, 255, 0, 255});
}

void KillTrapVisualAndTrigger(int phys)
{
    int visual = g_iTrapVisual[phys];
    if (visual > 0 && IsValidEntity(visual))
        AcceptEntityInput(visual, "Kill");
    g_iTrapVisual[phys] = -1;

    int trigger = g_iTrapTrigger[phys];
    if (trigger > 0 && IsValidEntity(trigger))
    {
        SDKUnhook(trigger, SDKHook_Touch, OnTrapTriggerTouch);
        AcceptEntityInput(trigger, "Kill");
        g_iTriggerOwner[trigger] = -1;
    }
    g_iTrapTrigger[phys] = -1;
}

public Action OnTrapTriggerTouch(int trigger, int victim)
{
    int phys = g_iTriggerOwner[trigger];
    if (phys < 1 || !IsValidEntity(phys))
        return Plugin_Continue;

    if (!IsHumanAlive(victim) || g_iTrapVictim[phys] != -1 || g_bIsStuck[victim])   // 只对人类生效，且一个陷阱一次只控一人
        return Plugin_Continue;

    SDKUnhook(trigger, SDKHook_Touch, OnTrapTriggerTouch);

    // 清理前序状态（与 StartGrapple 保持一致）
    ClearGrappleHandles(victim);

    // 重设自毁时间：空闲30秒 → 触发后15秒
    if (g_hTrapLifetime[phys] != INVALID_HANDLE)
    {
        KillTimer(g_hTrapLifetime[phys]);
        g_hTrapLifetime[phys] = INVALID_HANDLE;
    }
    g_hTrapLifetime[phys] = CreateTimer(BUTCHER_TRAP_STUCKTIME, Timer_DestroyTrap, phys);

    // 播放触发动画
    int visual = g_iTrapVisual[phys];
    if (visual > 0 && IsValidEntity(visual))
    {
        SetVariantString("trap");
        AcceptEntityInput(visual, "SetAnimation");
    }

    // 阻止惯性位移
    TeleportEntity(victim, NULL_VECTOR, NULL_VECTOR, view_as<float>({0.0, 0.0, 0.0}));

    // 更新数组
    g_iTrapVictim[phys] = victim;
    g_bIsStuck[victim] = true;

    // 陷阱伤害
    SDKHooks_TakeDamage(victim, phys, phys, BUTCHER_TRAP_DAMAGE, DMG_SLASH, _, _, _, false);

    // 屏幕抖动
    Shake(victim, 10.0, 10.0, 1.0);

    // 挣脱句柄
    g_iUsePressStep[victim] = 0;

    DataPack pack = new DataPack();
    g_hGrapplePack[victim] = pack;
    pack.WriteCell(phys);
    pack.WriteCell(victim);
    g_hGrapple[victim] = CreateTimer(1.0, Timer_TrapEscapeThink, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 音效
    EmitSoundToClient(victim, SFX_BUTCHER, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}你踩中了鬼手陷阱！快交替按AD键挣脱！");

    return Plugin_Continue;
}

public Action Timer_TrapEscapeThink(Handle timer, DataPack pack)
{
    pack.Reset();
    int phys = pack.ReadCell();
    int victim = pack.ReadCell();

    if (!IsValidEntity(phys))           // 陷阱被其他方式摧毁（比如射击或超时）
    {
        ClearGrappleHandles(victim);
        g_bIsStuck[victim] = false;
        return Plugin_Stop;
    }

    // 被其他方式释放（如Deimos击退），只停止挣脱句柄，陷阱保留
    if (!g_bIsStuck[victim])
    {
        ClearGrappleHandles(victim);
        return Plugin_Stop;
    }

    // 人没了，停止挣脱句柄，陷阱留着
    if (!IsHumanAlive(victim))
    {
        ClearGrappleHandles(victim);
        g_bIsStuck[victim] = false;
        return Plugin_Stop;
    }

    // 连按速率达标，只解除stuck，不消除陷阱模型
    if (g_iUsePressStep[victim] >= BUTCHER_TRAP_KEYRATE * 2)
    {
        ClearGrappleHandles(victim);
        g_bIsStuck[victim] = false;
        return Plugin_Stop;
    }

    // 每秒重置计数，意味着必须在1秒内按够次数
    g_iUsePressStep[victim] = 0;

    return Plugin_Continue;
}

public Action OnTrapTakeDamage(int phys, int &attacker, int &inflictor, float &damage, int &damagetype)
{
    // 受击特效：未被踩中 +16，被踩中 +32
    float hitPos[3];
    GetEntPropVector(phys, Prop_Send, "m_vecOrigin", hitPos);
    hitPos[2] += (g_iTrapVictim[phys] != -1) ? 32.0 : 16.0;
    TE_SetupBloodSprite(hitPos, view_as<float>({0.0, 0.0, 0.0}), {255, 0, 0, 255}, 3, g_iBloodSpray, g_iBloodDrop);
    TE_SendToAll();

    // 只关心是否会被打死，血量交给引擎处理
    int health = GetEntProp(phys, Prop_Data, "m_iHealth");

    if (health - RoundToFloor(damage) <= 0)
    {
        // 清理自毁句柄
        if (g_hTrapLifetime[phys] != INVALID_HANDLE)
        {
            KillTimer(g_hTrapLifetime[phys]);
            g_hTrapLifetime[phys] = INVALID_HANDLE;
        }

        // 释放被控制的玩家
        int victim = g_iTrapVictim[phys];
        if (victim != -1)
        {
            g_bIsStuck[victim] = false;
            ClearGrappleHandles(victim);
            g_iTrapVictim[phys] = -1;
        }

        KillTrapVisualAndTrigger(phys);
        if (g_iTrapCount > 0) g_iTrapCount--;
    }

    return Plugin_Continue;
}

public Action Timer_DestroyTrap(Handle timer, int phys)
{
    g_hTrapLifetime[phys] = INVALID_HANDLE;
    if (g_iTrapCount > 0) g_iTrapCount--;

    if (IsValidEntity(phys))
    {
        // 释放被控制的玩家
        int victim = g_iTrapVictim[phys];
        if (victim != -1)
        {
            g_bIsStuck[victim] = false;
            ClearGrappleHandles(victim);
            g_iTrapVictim[phys] = -1;
        }

        KillTrapVisualAndTrigger(phys);
        AcceptEntityInput(phys, "Kill");
    }

    return Plugin_Stop;
}

//========================================================================================
// 嗜血女巫技能：致幻（两阶段：接近→灰度，5秒后→致盲）
//========================================================================================

// 创建per-player CC实体（通用）
int CreateWitchCC(int victim, const char[] lutFile)
{
    int cc = CreateEntityByName("color_correction");
    if (cc == -1)
        return -1;

    DispatchKeyValue(cc, "filename", lutFile);
    DispatchKeyValue(cc, "maxweight", "1.0");
    DispatchKeyValue(cc, "fadeInDuration", "0.3");
    DispatchKeyValue(cc, "fadeOutDuration", "0.3");
    DispatchKeyValue(cc, "minfalloff", "-1");
    DispatchKeyValue(cc, "maxfalloff", "-1");
    DispatchKeyValue(cc, "StartDisabled", "1");
    DispatchSpawn(cc);
    ActivateEntity(cc);
    g_iWitchCCOwner[cc] = victim;
    SDKHook(cc, SDKHook_SetTransmit, OnWitchCCSetTransmit);
    AcceptEntityInput(cc, "Enable");
    return cc;
}

// 销毁玩家所有CC实体
void KillWitchCC(int client)
{
    int cc1 = g_iWitchMildCC[client];
    if (cc1 > 0 && IsValidEntity(cc1))
    {
        SDKUnhook(cc1, SDKHook_SetTransmit, OnWitchCCSetTransmit);
        g_iWitchCCOwner[cc1] = -1;
        AcceptEntityInput(cc1, "Kill");
    }
    g_iWitchMildCC[client] = -1;

    int cc2 = g_iWitchSevereCC[client];
    if (cc2 > 0 && IsValidEntity(cc2))
    {
        SDKUnhook(cc2, SDKHook_SetTransmit, OnWitchCCSetTransmit);
        g_iWitchCCOwner[cc2] = -1;
        AcceptEntityInput(cc2, "Kill");
    }
    g_iWitchSevereCC[client] = -1;
}

// 第一阶段：接近女巫→普通灰度
void StartWitchPhase1(int victim)
{
    if (g_iWitchPhase[victim] >= 1)
        return;

    g_iWitchPhase[victim] = 1;
    int cc = CreateWitchCC(victim, LUT_WITCH_MILD);
    if (cc != -1)
        g_iWitchMildCC[victim] = cc;
}

// 第二阶段：持续5秒→极端灰度+致盲+随机按键+耳鸣
void StartWitchPhase2(int victim)
{
    if (g_iWitchPhase[victim] == 2)
        return;

    g_iWitchPhase[victim] = 2;
    g_bIsBlind[victim] = true;
    g_iWitchKeyRot[victim] = GetRandomInt(0, 2);

    // 关闭第一阶段CC
    KillWitchCC(victim);

    // 开启第二阶段CC（极端灰度+色阶量化）
    int cc2 = CreateWitchCC(victim, LUT_WITCH_SEVERE);
    if (cc2 != -1)
        g_iWitchSevereCC[victim] = cc2;

    // 高频切除：交替33↔34，每次在1.6s过期前（1.4s）切到另一个
    // 33和34链路完全相同（DFR + FLT LP 1kHz），切换听不出区别
    g_bWitchDSPFlip[victim] = false;
    ClientCommand(victim, "dsp_player 33");
    if (g_hWitchDSPToggle[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchDSPToggle[victim]);
        g_hWitchDSPToggle[victim] = INVALID_HANDLE;
    }
    g_hWitchDSPToggle[victim] = CreateTimer(1.4, Timer_WitchDSPToggle, victim, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 致幻音乐（wav自带循环标记才会loop）
    EmitSoundToClient(victim, SFX_WITCH2, _, SNDCHAN_STATIC);

    // 提示
    char ZombieName[32];
    for (int i = 1; i <= 2048; i++)
    {
        if (IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH))
        {
            Han_GetZombieName(i, ZombieName, sizeof(ZombieName));
            break;
        }
    }
    CPrintToChat(victim, "{green}[华仔] {red}你被%s的幻术吞噬了！", ZombieName);
    EmitSoundToClient(victim, SFX_WITCH1, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 10秒自毁（先kill旧的防泄露）
    if (g_hWitchBlind[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchBlind[victim]);
        g_hWitchBlind[victim] = INVALID_HANDLE;
    }
    g_hWitchBlind[victim] = CreateTimer(WITCH_BLIND_TIME, Timer_DeBlind, victim);
}

// 解除致盲（正常到期或被震荡波打断）
void EndWitchBlind(int victim)
{
    g_bIsBlind[victim] = false;
    g_iWitchPhase[victim] = 0;
    g_flWitchNearTime[victim] = 0.0;

    // 停止DSP和音乐
    ClientCommand(victim, "dsp_player 0");
    StopSound(victim, SNDCHAN_STATIC, SFX_WITCH2);

    // 清理timer
    if (g_hWitchBlind[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchBlind[victim]);
        g_hWitchBlind[victim] = INVALID_HANDLE;
    }
    if (g_hWitchDSPToggle[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchDSPToggle[victim]);
        g_hWitchDSPToggle[victim] = INVALID_HANDLE;
    }

    // 重置按键映射
    g_iWitchKeyRot[victim] = -1;

    // 杀所有CC
    KillWitchCC(victim);
}

public Action Timer_DeBlind(Handle timer, int client)
{
    g_hWitchBlind[client] = INVALID_HANDLE;
    if (g_bIsBlind[client])
        EndWitchBlind(client);
    return Plugin_Stop;
}

// 交替33↔34防DSP过期（两者DFR+FLT LP 1kHz链路完全相同）
public Action Timer_WitchDSPToggle(Handle timer, int victim)
{
    if (!IsHumanAlive(victim) || !g_bIsBlind[victim])
    {
        g_hWitchDSPToggle[victim] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    g_bWitchDSPFlip[victim] = !g_bWitchDSPFlip[victim];
    ClientCommand(victim, "dsp_player %d", g_bWitchDSPFlip[victim] ? 34 : 33);
    return Plugin_Continue;
}

// 每只Witch自己的near检查timer
public Action Timer_WitchNearThink(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie))
    {
        g_hWitchNear[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    for (int victim = 1; victim <= MaxClients; victim++)
    {
        if (!IsHumanAlive(victim))
            continue;

        float dist = GetMeterDistance(zombie, victim);

        if (dist <= WITCH_NEAR_RANGE)
        {
            if (g_iWitchPhase[victim] == 0)
            {
                StartWitchPhase1(victim);
            }
            else if (g_iWitchPhase[victim] == 1)
            {
                g_flWitchNearTime[victim] += 0.1;
                if (g_flWitchNearTime[victim] >= WITCH_NEAR_TIME)
                    StartWitchPhase2(victim);
            }
        }
        // out-of-range 不在此timer处理，Phase 1/2清理见下方
    }

    // 对每个人类：如果phase=1且没被任何witch覆盖 → 取消phase 1
    // 只在索引最小的witch回调中执行清理，避免重复
    int firstWitch = -1;
    for (int i = 1; i <= 2048; i++)
    {
        if (IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH)) { firstWitch = i; break; }
    }
    if (zombie == firstWitch)
    {
        for (int victim = 1; victim <= MaxClients; victim++)
        {
            if (g_iWitchPhase[victim] != 1)
                continue;

            bool anyNear;
            for (int i = 1; i <= 2048; i++)
            {
                if (IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH) && GetMeterDistance(i, victim) <= WITCH_NEAR_RANGE)
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

    return Plugin_Continue;
}

// 致盲状态下的模型隐藏：SetTransmit回调（不隐藏受害者自己）
public Action OnWitchBlindSetTransmit(int entity, int client)
{
    if (g_bIsBlind[client] && entity != client)
        return Plugin_Handled;
    return Plugin_Continue;
}

public Action OnWitchCCSetTransmit(int cc, int client)
{
    // 只传给受害者
    if (client == g_iWitchCCOwner[cc])
        return Plugin_Continue;
    return Plugin_Handled;
}

//========================================================================================
// 幽灵僵尸技能：身形几乎不可见
//========================================================================================

void SetInvisible(int zombie)
{
    int AnimEnt = FindAnimEntity(zombie);

    if (AnimEnt != -1)
    {
        SetEntityRenderMode(AnimEnt, RENDER_TRANSCOLOR);
        SetEntityRenderFx(AnimEnt, RENDERFX_STROBE_FAST);     // https://sm.alliedmods.net/api/index.php?fastload=show&id=827&
        SetEntityRenderColor(AnimEnt, 0, 0, 0, 1);
    }
}

void SetVisible(int zombie)
{
    int AnimEnt = FindAnimEntity(zombie);

    if (AnimEnt != -1)
    {
        SetEntityRenderMode(AnimEnt, RENDER_NORMAL);
        SetEntityRenderFx(AnimEnt, RENDERFX_NONE);
        SetEntityRenderColor(AnimEnt, 255, 255, 255, 255);
    }
}

//========================================================================================
// 奖励僵尸技能：攻击它可以恢复血量和护甲，奖励金钱
//========================================================================================

void GiveHealth(int client, int value)
{
    SetEntProp(client, Prop_Data, "m_iHealth", 
        GetEntProp(client, Prop_Data, "m_iHealth") + value < MAX_HEALTH ? 
        GetEntProp(client, Prop_Data, "m_iHealth") + value : 
        MAX_HEALTH
    );
}

void GiveArmor(int client, int value)
{
    SetEntProp(client, Prop_Send, "m_ArmorValue", 
        GetEntProp(client, Prop_Send, "m_ArmorValue") + value < 100 ? 
        GetEntProp(client, Prop_Send, "m_ArmorValue") + value : 
        100
    );
}

void GiveMoney(int client, int value)
{
    SetEntProp(client, Prop_Send, "m_iAccount", 
        GetEntProp(client, Prop_Send, "m_iAccount") + value < MAX_MONEY ? 
        GetEntProp(client, Prop_Send, "m_iAccount") + value : 
        MAX_MONEY
    );
}