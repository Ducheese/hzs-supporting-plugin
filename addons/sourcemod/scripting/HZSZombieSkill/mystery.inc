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

    Handle trace = TR_TraceRayFilterEx(targetPos, {90.0, 0.0, 0.0}, MASK_SOLID, RayType_Infinite, TraceEntityFilter_World);
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
    SetVariantString("fasteridle");
    AcceptEntityInput(portal, "SetAnimation");

    // 传送门模型得翻面才能看见
    groundPos[2] += 5.0;
    TeleportEntity(portal, groundPos, view_as<float>({180.0, 0.0, 0.0}), NULL_VECTOR);

    // 传送门环境音
    EmitAmbientSound(SFX_MYSTERY2, groundPos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 有效范围指示
    // DebugDrawRadius(groundPos, MYSTERY_PORTAL_RADIUS / GAMEUNITS_TO_METERS, {255, 0, 0, 255}, MYSTERY_PORTAL_TIME);

    // 记录实体号
    g_iPortal[zombie] = portal;

    // 延时起效
    g_hMysterySwap[zombie] = CreateTimer(MYSTERY_PORTAL_TIME, Timer_SwapPosition, zombie);

    // 文字提示
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));
    CPrintToChat(target, "{green}[华仔] {red}脚下有%s的传送门！", ZombieName);
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

    float zombiePos[3], portalPos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", zombiePos);
    GetEntPropVector(portal, Prop_Send, "m_vecOrigin", portalPos);

    // 找传送门范围内可传送的人类
    bool anyValid = false;
    for (int i = 1; i <= MaxClients; i++)
    {
        if (!IsHumanAlive(i))
            continue;
        if (g_bIsGrappled[i] || g_bIsStuck[i])
            continue;
        if (GetMeterDistance(portal, i) >= MYSTERY_PORTAL_RADIUS)
            continue;

        anyValid = true;

        // 人类传到僵尸位置
        TeleportEntity(i, zombiePos, NULL_VECTOR, NULL_VECTOR);
        StartUnstuck(i);
        Fade(i, 512, 128, 17, {100, 180, 255, 200});
        EmitSoundToClient(i, SFX_MYSTERY1, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    }

    if (!anyValid)
    {
        // 范围内无人（或被控），不清除技能占位，重新开始检测
        CleanupMysteryPortal(zombie);
        g_hMysteryNear[zombie] = CreateTimer(0.1, Timer_MysteryNearThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
        return Plugin_Stop;
    }

    // 僵尸传到传送门位置
    TeleportEntity(zombie, portalPos, NULL_VECTOR, NULL_VECTOR);

    // 电弧
    CreateTesla(zombiePos);

    CleanupMysteryPortal(zombie);
    return Plugin_Stop;
}

void CreateTesla(float pos[3])
{
    int tesla = CreateEntityByName("point_tesla");
    if (tesla == -1)
        return;

    DispatchKeyValue(tesla, "texture", "sprites/lgtning.vmt");
    DispatchKeyValue(tesla, "m_Color", "255 255 255 200");     // 不可以删，少了就不渲染电弧，真是奇葩
    DispatchKeyValue(tesla, "m_SoundName", "ambient/energy/spark2.wav");
    DispatchKeyValue(tesla, "interval_min", "0.1");
    DispatchKeyValue(tesla, "interval_max", "0.2");
    DispatchKeyValue(tesla, "m_flRadius", "50");
    DispatchSpawn(tesla);
    ActivateEntity(tesla);

    pos[2] += 5.0;
    TeleportEntity(tesla, pos, NULL_VECTOR, NULL_VECTOR);
    AcceptEntityInput(tesla, "TurnOn");

    CreateTimer(1.0, Timer_KillTesla, EntIndexToEntRef(tesla));
}

public Action Timer_KillTesla(Handle timer, int ref)
{
    int entity = EntRefToEntIndex(ref);
    if (entity != INVALID_ENT_REFERENCE && IsValidEntity(entity))
        AcceptEntityInput(entity, "Kill");
    return Plugin_Stop;
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
