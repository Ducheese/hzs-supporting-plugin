//========================================================================================
// 恶魔僵尸技能：预瞄阶段
//========================================================================================

void StartProjectile(int zombie, int attacker)
{
    g_iDeimosAimTarget[zombie] = attacker;
    g_flDeimosAimStart[zombie] = GetEngineTime();

    // 停止移动
    Han_SetZombieTarget(zombie, zombie, 0.0);

    // 预瞄think
    g_hDeimosAim[zombie] = CreateTimer(0.1, Timer_DeimosAimThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

public Action Timer_DeimosAimThink(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        CleanupProjectile(zombie);
        return Plugin_Stop;
    }

    // 预瞄期间目标死亡，进入短CD
    int target = g_iDeimosAimTarget[zombie];
    if (!IsHumanAlive(target))
    {
        CleanupProjectile(zombie);
        CreateTimer(DEIMOS_PROJ_FAIL_CD, Timer_SkillEnd, zombie);
        return Plugin_Stop;
    }

    bool blocked = !IsAimClear(zombie, target);

    // 预瞄计时
    if (GetEngineTime() - g_flDeimosAimStart[zombie] < DEIMOS_PROJ_AIMTIME)
    {
        FaceTarget(zombie, target);
        DrawAimLaser(zombie, target, blocked);
        return Plugin_Continue;
    }

    // 预瞄结束
    g_hDeimosAim[zombie] = INVALID_HANDLE;

    // 预瞄结尾视线还是被阻挡，进入短CD
    if (blocked)
    {
        CleanupProjectile(zombie);
        CreateTimer(DEIMOS_PROJ_FAIL_CD, Timer_SkillEnd, zombie);
        return Plugin_Stop;
    }

    // 进入下一阶段
    LaunchProjectile(zombie, target);

    return Plugin_Stop;
}

//========================================================================================
// 工具函数
//========================================================================================

void DrawAimLaser(int zombie, int target, bool blocked)
{
    float start[3], end[3];
    GetEntityTopCenter(zombie, start);
    GetEntityTopCenter(target, end);

    int color[4];
    if (blocked)
    {
        color[0] = 255; color[1] = 0; color[2] = 0; color[3] = 128;       // 红色（遮挡）
    }
    else
    {
        color[0] = 255; color[1] = 255; color[2] = 0; color[3] = 128;     // 黄色（清晰）
    }

    TE_SetupBeamPoints(start, end, PrecacheModel(TOOL_BEAMSPRITE), 0, 0, 0, 0.15, 1.5, 1.5, 0, 0.0, color, 0);
    TE_SendToAll();
}

void FaceTarget(int zombie, int target)
{
    float pos[3], targetPos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    GetEntPropVector(target, Prop_Send, "m_vecOrigin", targetPos);

    float dir[3], ang[3];
    MakeVectorFromPoints(pos, targetPos, dir);
    GetVectorAngles(dir, ang);

    ang[0] = 0.0;   // 不看俯仰角，只看偏航角
    ang[2] = 0.0;

    TeleportEntity(zombie, NULL_VECTOR, ang, NULL_VECTOR);
}

bool IsAimClear(int zombie, int target)
{
    float start[3], end[3];
    GetEntityTopCenter(zombie, start);
    GetEntityTopCenter(target, end);

    /**
     * 和其他地方TR_TraceRayFilterEx参数不一样
     * 最后得好好检查一下
     */
    Handle trace = TR_TraceRayFilterEx(start, end, MASK_SHOT, RayType_EndPoint, TraceEntityFilter_Projectile, zombie);
    bool hit = TR_DidHit(trace);
    delete trace;

    return !hit;
}

//========================================================================================
// 恶魔僵尸技能：飞行阶段
//========================================================================================

void LaunchProjectile(int zombie, int target)
{
    float startPos[3], targetPos[3];
    GetEntityTopCenter(zombie, startPos);
    GetEntityTopCenter(target, targetPos);

    // 创建飞行物模型
    int proj = CreateEntityByName("prop_dynamic");
    if (proj == -1)
        return;

    DispatchKeyValue(proj, "model", MODEL_ZOMBIEPROJ);
    DispatchSpawn(proj);
    TeleportEntity(proj, startPos, NULL_VECTOR, NULL_VECTOR);

    g_iDeimosProjectile[zombie] = proj;
    g_iDeimosProjOwner[proj] = zombie;

    // 用实体闲置属性暂存起点和初速度
    SetEntPropVector(proj, Prop_Send, "m_vecMins", startPos);

    float time = DEIMOS_PROJ_FLYTIME;
    float gravity = GetConVarFloat(FindConVar("sv_gravity"));
    float vel[3];
    vel[0] = (targetPos[0] - startPos[0]) / time;
    vel[1] = (targetPos[1] - startPos[1]) / time;
    vel[2] = (targetPos[2] - startPos[2]) / time + (0.5 * gravity * time);

    SetEntPropVector(proj, Prop_Send, "m_vecMaxs", vel);

    // 复用 g_flDeimosAimStart 存发射时间
    g_flDeimosAimStart[zombie] = GetEngineTime();

    // 后续用 SND_CHANGEVOL|SND_CHANGEPITCH 原地改参数，复用同一声道而非重建
    EmitSoundToAll(SFX_DEIMOS3, proj, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_LOOPING, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 飞行think（离散teleport，同时负责超时判断）
    g_hDeimosProj[zombie] = CreateTimer(0.01, Timer_DeimosProjThink, proj, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

// vel 为发射初速度常量（取自 proj 的 m_vecMaxs），elapsed 为已飞行时间，函数内自行推导瞬时速度，不依赖跨帧状态
void UpdateDeimosWind(int proj, float vel[3], float gravity, float elapsed)
{
    // 瞬时速度 = 初速度 - 重力·t（竖直分量衰减），抛物线真实速度大小
    float curVel[3];
    curVel[0] = vel[0];
    curVel[1] = vel[1];
    curVel[2] = vel[2] - gravity * elapsed;

    // 照搬 GMod：speed 取三轴绝对值平均（非矢量长度）；实测飞行速度 475~725，均<1000
    float speed = (FloatAbs(curVel[0]) + FloatAbs(curVel[1]) + FloatAbs(curVel[2])) / 3.0;
    // 照搬 GMod 线性映射：[100,1000] → [0,1]，loudness 默认 1.0
    float speedClamped = speed;
    if (speedClamped < 100.0) speedClamped = 100.0;
    if (speedClamped > 1000.0) speedClamped = 1000.0;
    float vol = (speedClamped - 100.0) / (1000.0 - 100.0);

    // 照搬 GMod pitch 策略：仅尺寸项（物体越大越低沉）。距离衰减由引擎 3D 空间化负责，不照搬 dist 项。
    // 投掷物尺寸恒定，固定 shapevolume（GMod 对玩家档取 125，典型小物体）；此 SourcePawn 无 GetCollisionBounds
    float shapeClamped = 150.0;
    if (shapeClamped < 1.0)   shapeClamped = 1.0;     // minshapevolume
    if (shapeClamped > 300.0) shapeClamped = 300.0;   // maxshapevolume
    int pitch = RoundToNearest((1.0 - (shapeClamped - 1.0) / (300.0 - 1.0)) * 200.0);
    if (pitch < 10) pitch = 10;

    // PrintToServer("vol:%f pitch:%i", vol, pitch);

    EmitSoundToAll(SFX_DEIMOS3, proj, SNDCHAN_STATIC, SNDLEVEL_NORMAL,
        SND_CHANGEVOL | SND_CHANGEPITCH, vol, pitch);
}

public Action Timer_DeimosProjThink(Handle timer, int proj)
{
    int zombie = g_iDeimosProjOwner[proj];
    if (zombie == -1)
        return Plugin_Stop;       // 如果zombie都获取不到，那什么清理也做不了

    if (!IsValidEntity(proj))
    {
        CleanupProjectile(zombie);
        CreateTimer(DEIMOS_PROJ_FAIL_CD, Timer_SkillEnd, zombie);
        return Plugin_Stop;
    }

    // 超时判断
    float elapsed = GetEngineTime() - g_flDeimosAimStart[zombie];
    if (elapsed >= DEIMOS_PROJ_FLYTIME + 0.5)     // 多飞行一段时间
    {
        CleanupProjectile(zombie);
        CreateTimer(DEIMOS_PROJ_FAIL_CD, Timer_SkillEnd, zombie);
        return Plugin_Stop;
    }

    // 离散抛物线：pos = start + vel*t + ½g*t²
    float startPos[3], vel[3];
    GetEntPropVector(proj, Prop_Send, "m_vecMins", startPos);
    GetEntPropVector(proj, Prop_Send, "m_vecMaxs", vel);
    float gravity = GetConVarFloat(FindConVar("sv_gravity"));

    float pos[3];
    pos[0] = startPos[0] + vel[0] * elapsed;
    pos[1] = startPos[1] + vel[1] * elapsed;
    pos[2] = startPos[2] + vel[2] * elapsed - 0.5 * gravity * elapsed * elapsed;

    TeleportEntity(proj, pos, NULL_VECTOR, NULL_VECTOR);

    // 飞行风声调制
    UpdateDeimosWind(proj, vel, gravity, elapsed);

    // 命中检测
    int target = FindNearestHuman(proj, DEIMOS_PROJ_RANGE / GAMEUNITS_TO_METERS);
    if (target != -1)
    {
        // 屏幕震动
        Shake(target, 10.0, 10.0, 1.0);

        // 命中音效
        EmitSoundToClient(target, SFX_DEIMOS1, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

        // 强制缴械 + 文字提示
        if (DropActiveWeapon(target))
        {
            char ZombieName[32];
            Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));
            CPrintToChat(target, "{green}[华仔] {red}你被%s的投掷物击落了武器！", ZombieName);
        }

        // 成功后的清理
        CleanupProjectile(zombie);
        CreateTimer(DEIMOS_PROJ_CD, Timer_SkillEnd, zombie);
        return Plugin_Stop;
    }

    return Plugin_Continue;
}

//========================================================================================
// 恶魔僵尸技能：人类弹反机制
//========================================================================================

void CheckProjParry(int client)
{
    // 只对存活人类
    if (!IsHumanAlive(client))
        return;

    // 不是刀
    int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");
    if (weapon != GetPlayerWeaponSlot(client, 2))
        return;

    // 遍历投掷物数组
    for (int i = 0; i < sizeof(g_iDeimosProjectile); i++)
    {
        int proj = g_iDeimosProjectile[i];

        if (!IsValidEntity(proj))
            continue;

        // 距离检查
        float parryPos[3], projPos[3];
        GetEntityTopCenter(client, parryPos);
        GetEntPropVector(proj, Prop_Send, "m_vecOrigin", projPos);
        if (GetVectorDistance(parryPos, projPos) * GAMEUNITS_TO_METERS > DEIMOS_PARRY_RANGE)
            continue;

        // 朝向检查
        if (!IsTargetForward(client, proj))
            continue;

        // 弹反火花
        float sparkDir[3] = {0.0, 0.0, 1.0};
        TE_SetupSparks(projPos, sparkDir, 2, 2);
        TE_SendToAll();

        // 弹反音效
        EmitSoundToClient(client, SFX_DEIMOS2, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

        // 停风声
        StopDeimosWind(proj);

        AcceptEntityInput(proj, "Kill");
        g_iDeimosProjectile[i] = -1;          // 就是i索引没错
    }
}

//========================================================================================
// 恶魔僵尸技能：集中清理
//========================================================================================

// 注意：引擎不会在实体销毁时自动停此loop，各销毁路径必须在 Kill 之前显式调用本函数
void StopDeimosWind(int proj)
{
    if (IsValidEntity(proj))
        EmitSoundToAll(SFX_DEIMOS3, proj, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_STOPLOOPING);
}

void CleanupProjectile(int zombie)
{
    g_iDeimosAimTarget[zombie] = -1;

    // 停预瞄think
    if (g_hDeimosAim[zombie] != INVALID_HANDLE)
    {
        KillTimer(g_hDeimosAim[zombie]);
        g_hDeimosAim[zombie] = INVALID_HANDLE;
    }

    // 停飞行think
    if (g_hDeimosProj[zombie] != INVALID_HANDLE)
    {
        KillTimer(g_hDeimosProj[zombie]);
        g_hDeimosProj[zombie] = INVALID_HANDLE;
    }

    // 清理飞行物实体
    int proj = g_iDeimosProjectile[zombie];
    if (IsValidEntity(proj))
    {
        StopDeimosWind(proj);
        AcceptEntityInput(proj, "Kill");
    }
        
    g_iDeimosProjectile[zombie] = -1;
    if (proj != -1)
        g_iDeimosProjOwner[proj] = -1;

    // 恢复 NPC 移动
    Han_UnlockZombie(zombie);
}
