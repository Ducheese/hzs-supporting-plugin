//========================================================================================
// 神秘僵尸技能：交换位置
//========================================================================================

public Action Timer_MysteryNearThink(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        g_hMysteryNear[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    int target = GetEntDataEnt2(zombie, g_iLeaderOffset);
    if (!IsHumanAlive(target))
        return Plugin_Continue;

    if (GetMeterDistance(zombie, target) <= MYSTERY_PORTAL_RANGE)
    {
        StartPortal(zombie, target);
        g_hMysteryNear[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    return Plugin_Continue;
}

void StartPortal(int zombie, int target)
{
    // 发动传送门期间定在原地不动
    Han_SetZombieTarget(zombie, zombie, 0.0);

    // 在目标脚下创建传送门
    float targetPos[3], groundPos[3];
    GetEntPropVector(target, Prop_Send, "m_vecOrigin", targetPos);

    Handle trace = TR_TraceRayFilterEx(targetPos, {90.0, 0.0, 0.0}, MASK_SOLID, RayType_Infinite, TraceEntityFilter_Ground, 0);
    TR_GetEndPosition(groundPos, trace);
    delete trace;

    // 传送门模型
    int portal = CreateEntityByName("prop_dynamic");
    if (portal == -1)
        return;

    DispatchKeyValue(portal, "model", MODEL_HL2PORTAL);
    DispatchKeyValue(portal, "modelscale", "0.15");
    DispatchKeyValue(portal, "disableshadows", "1");
    DispatchKeyValue(portal, "disableshadowdepth", "1");
    DispatchSpawn(portal);
    SetVariantString("idle");
    AcceptEntityInput(portal, "SetAnimation");

    // 传送门模型得翻面才能看见
    TeleportEntity(portal, groundPos, view_as<float>({180.0, 0.0, 0.0}), NULL_VECTOR);

    // 有效范围指示
    // DebugDrawRadius(groundPos, MYSTERY_PORTAL_RADIUS / GAMEUNITS_TO_METERS, {255, 0, 0, 255}, MYSTERY_PORTAL_TIME);

    // 记录实体号
    g_iPortal[zombie] = portal;

    // 延时起效
    g_hMysterySwap[zombie] = CreateTimer(MYSTERY_PORTAL_TIME, Timer_SwapPosition, zombie);
}

public Action Timer_SwapPosition(Handle timer, int zombie)
{
    g_hMysterySwap[zombie] = INVALID_HANDLE;

    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        CleanupMysteryPortal(zombie);
        return Plugin_Stop;
    }

    int portal = g_iPortal[zombie];
    if (portal == -1 || !IsValidEntity(portal))
    {
        CleanupMysteryPortal(zombie);
        return Plugin_Stop;
    }

    int nearest = FindNearestHuman(portal);
    if (nearest != -1 && GetMeterDistance(portal, nearest) < MYSTERY_PORTAL_RADIUS
    && !g_bIsGrappled[nearest]
    && !g_bIsStuck[nearest]
    )
        SwapPosition(zombie, nearest);

    CleanupMysteryPortal(zombie);
    return Plugin_Stop;
}

void SwapPosition(int zombie, int human)
{
    float zombiePos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", zombiePos);

    float humanPos[3];
    GetEntPropVector(human, Prop_Send, "m_vecOrigin", humanPos);

    TeleportEntity(zombie, humanPos, NULL_VECTOR, NULL_VECTOR);
    TeleportEntity(human, zombiePos, NULL_VECTOR, NULL_VECTOR);

    EmitSoundToClient(human, SFX_MYSTERY, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

void CleanupMysteryPortal(int zombie)
{
    if (g_iPortal[zombie] != -1)
    {
        if (IsValidEntity(g_iPortal[zombie]))
            AcceptEntityInput(g_iPortal[zombie], "Kill");
        g_iPortal[zombie] = -1;
    }

    if (g_hMysterySwap[zombie] != INVALID_HANDLE)
    {
        KillTimer(g_hMysterySwap[zombie]);
        g_hMysterySwap[zombie] = INVALID_HANDLE;
    }

    Han_UnlockZombie(zombie);
}
