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
    Handle trace = TR_TraceRayFilterEx(deathPos, {90.0, 0.0, 0.0}, MASK_SOLID, RayType_Infinite, TraceEntityFilter_World);
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

    DispatchKeyValue(visual, "model", MODEL_ZOMBIETRAP);
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
