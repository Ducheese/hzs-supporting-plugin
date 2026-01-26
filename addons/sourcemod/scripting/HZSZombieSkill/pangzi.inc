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
    // 冲锋句柄，方向实时调节，震地声同步
    DataPack pack = new DataPack();
    g_hChargePack[zombie] = pack;    // 全局管理，方便清理

    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    g_hCharge[zombie] = CreateTimer(0.5, Timer_ChargeThink, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    g_hMovetype[zombie] = CreateTimer(0.0, Timer_MovetypeThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 时间到了且未擒抱到人，自动结束冲锋
    CreateTimer(PANGZI_CHARGE_TIME, Timer_EndCharge, zombie);
}

public Action Timer_MovetypeThink(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        g_hMovetype[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    float dist, hitPos[3];
    GetDistanceToGround(zombie, dist, hitPos);

    // NPC僵尸能冲锋的关键，交替变换movetype防止升天，最好用单独的高频timer来检测
    if (dist < 1.0)
    {
        // PrintToChatAll("飞行模式, %f", dist);
        SetEntityMoveType(zombie, MOVETYPE_FLYGRAVITY);
    }
    else if (dist > 10.0)
    {
        // PrintToChatAll("地面模式, %f", dist);
        SetEntityMoveType(zombie, MOVETYPE_STEP);
        TeleportEntity(zombie, NULL_VECTOR, NULL_VECTOR, view_as<float>({0.0, 0.0, -100.0}));   // 迅速拉回地面
    }

    return Plugin_Continue;
}

public Action Timer_ChargeThink(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();

    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        g_hCharge[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    // 人没了，关闭冲锋句柄即可（僵尸死亡的处理移到event）
    if (IsZeroPostion(victim) || !IsPlayerAlive(victim))
    {
        ClearChargeHandles(zombie);

        SetEntityMoveType(zombie, MOVETYPE_STEP);
        return Plugin_Stop;
    }

    // 人和僵尸的位置都是时刻在变的

    // 距离够近，则关闭冲锋，开始擒抱逻辑
    if (GetMeterDistance(zombie, victim) <= PANGZI_GRAPPLE_RANGE)
    {
        ClearChargeHandles(zombie);
        
        SetEntityMoveType(zombie, MOVETYPE_STEP);
        StartGrapple(zombie, victim);
        return Plugin_Stop;
    }

    // 如果不是以上情况，则持续施加向目标的推力
    float fTargetOrigin[3];
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", fTargetOrigin);
    CreateKnockback(fTargetOrigin, zombie, view_as<float>({-PANGZI_CHARGE_FORCE, -PANGZI_CHARGE_FORCE, 0.0}));

    // // 辅助线（记得删）
    // TE_SetupBeamPoints(pos, fTargetOrigin, g_iBeamSprite, 0, 0, 0, 5.0, 2.0, 2.0, 1, 0.0, {255, 0, 0, 255}, 15);
    // TE_SendToAll();

    // 震地音效
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    EmitAmbientSound(SFX_CHARGE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    
    return Plugin_Continue;
}

public Action Timer_EndCharge(Handle timer, int zombie)
{
    // 没有抱到人，关闭冲锋句柄，进入CD冷却
    if (g_hCharge[zombie] != INVALID_HANDLE) 
    {
        ClearChargeHandles(zombie);    // 清理计时器和 DataPack

        SetEntityMoveType(zombie, MOVETYPE_STEP);
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

        KillTimer(g_hMovetype[zombie]);
        g_hMovetype[zombie] = INVALID_HANDLE; 
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

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", g_flEscapePos[victim]);

    // 更新数组
    g_iGrappling[zombie] = victim;
    g_bIsGrappled[victim] = true;

    // 更换第一视角
    GrappleFirstView(zombie, victim);

    // 第三人称看到的效果
    GrappleThirdView(zombie, victim);

    // 擒抱伤害和挣脱共用句柄（目前Bot无法自己挣脱）
    g_iUsePressCount[victim] = 0;    // 初始化按键计数

    DataPack pack = new DataPack();
    g_hGrapplePack[victim] = pack;    // 全局管理，方便清理

    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    
    g_hGrapple[victim] = CreateTimer(1.0, Timer_GrappleThink, pack, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 文字提示
    if (!IsFakeClient(victim))
    {
        CPrintToChat(victim, "{green}[华仔] {red}你被擒抱住了！快连按E键挣脱！");
    }
    else
    {
        CPrintToChatAll("{green}[华仔] {red}%s擒抱住了%s，快去帮忙掩护！", ZombieName, g_ClientName[victim]);   
    }

    // 吼叫音效
    EmitAmbientSound(SFX_GRAPPLE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

void GrappleFirstView(int zombie, int victim)
{
    float viewOffset[3] = {150.0, 0.0, 250.0};    // 这个位置决定了“眼睛”在哪里，可以放在僵尸的肩膀或头顶
    float viewAngles[3] = {15.0, 180.0, 0.0};     // 这个角度决定了“眼睛”朝哪里看，{0, 180, 0} 大致是朝向后方，即僵尸的脸
    
    int viewControl = CreateEntityByName("point_viewcontrol");

    if (viewControl != -1)
    {
        DispatchSpawn(viewControl);

        SetVariantString("!activator");
        AcceptEntityInput(viewControl, "SetParent", zombie);

        TeleportEntity(viewControl, viewOffset, viewAngles, NULL_VECTOR);

        AcceptEntityInput(viewControl, "Enable");
        SetClientViewEntity(victim, viewControl);
    }

    g_iViewControl[victim] = viewControl;
}

void GrappleThirdView(int zombie, int victim)
{
    float viewOffset[3] = {100.0, 0.0, 150.0};    // 这个位置决定了“眼睛”在哪里，可以放在僵尸的肩膀或头顶
    float viewAngles[3] = {15.0, 180.0, 0.0};     // 这个角度决定了“眼睛”朝哪里看，{0, 180, 0} 大致是朝向后方，即僵尸的脸

    SetVariantString("!activator");
    AcceptEntityInput(victim, "SetParent", zombie);

    TeleportEntity(victim, viewOffset, viewAngles, NULL_VECTOR);

    SetEntityMoveType(victim, MOVETYPE_NONE);   // 必须要有，不然就闪退
}

void ClearViewControl(int victim)
{
    int entity = g_iViewControl[victim];

    if (IsValidEntity(entity))
    {
        char classname[64];
        GetEdictClassname(entity, classname, sizeof(classname));

        if (StrEqual(classname, "point_viewcontrol", false))
        {
            RemoveEdict(entity);
        }
    }

    g_iViewControl[victim] = -1;
}

public Action Timer_GrappleThink(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();
    
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        g_hGrapple[victim] = INVALID_HANDLE;
        return Plugin_Stop;
    }

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
    Shake(victim, 10.0, 10.0, 1.0);
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

    // 恢复原样
    SetClientViewEntity(victim, victim);
    SetEntityMoveType(victim, MOVETYPE_WALK);
    AcceptEntityInput(victim, "ClearParent");

    // 清理实体
    ClearViewControl(victim);

    // 传送新位置
    TeleportEntity(victim, g_flEscapePos[victim], NULL_VECTOR, NULL_VECTOR);
}

//========================================================================================
// 巨型狂暴形态僵尸技能：投掷僵尸
//========================================================================================

void StartPickupAndThrow(int pangzi, int victim)
{
    // 变量初始化
    int thrownCount = 0;
    int thrownMax = GetRandomInt(PANGZI_THROW_MIN, PANGZI_THROW_MAX);

    // 搜索附近的“弹药”
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

            if (zombie == pangzi)   // 不能投掷自己（还得过滤掉Boss级别的大型僵尸）
            {
                continue;
            }

            if (IsSameName(zombie, ZOMBIE_GHOST)
            || IsSameName(zombie, BOSS_ANGELA)
            || IsSameName(zombie, BOSS_PANGZI))
            {
                continue;
            }

            if (g_bIsPickup[zombie])
            {
                continue;
            }

            if (GetMeterDistance(zombie, pangzi) > PANGZI_THROW_RADIUS)
            {
                continue;
            }

            thrownCount++;

            PickupAndThrowZombie(pangzi, zombie, victim);
            g_bIsPickup[zombie] = true;
            
            if (thrownCount >= thrownMax)
            {
                break;
            }
        }
    }

    // 延时开放技能占位
    if (thrownCount > 0)
    {
        // 基础信息
        char ZombieName[32];
        Han_GetZombieName(pangzi, ZombieName, sizeof(ZombieName));

        // 文字提示
        CPrintToChat(victim, "{green}[华仔] {red}%s想抓些什么丢过来！", ZombieName);
        CreateTimer(PANGZI_THROW_HOLD+PANGZI_THROW_FLYTIME+PANGZI_THROW_CD, Timer_SkillEnd, pangzi);
    }
    else
        g_bIsSkillUsed[pangzi] = false;    // 没有投掷任何僵尸，取消技能占位
}

void PickupAndThrowZombie(int pangzi, int zombie, int victim)
{
    // 设置父子关系，把子弹僵尸举到空中（尚未解除父子关系）
    PickupZombie(pangzi, zombie);

    // 延时投掷僵尸
    DataPack pack = new DataPack();
    pack.WriteCell(pangzi);
    pack.WriteCell(zombie);
    pack.WriteCell(victim);
    CreateTimer(PANGZI_THROW_HOLD, Timer_ThrowZombie, pack);
}

void PickupZombie(int pangzi, int zombie)
{
    float viewOffset[3] = {100.0, 0.0, 150.0};
    
    SetVariantString("!activator");
    AcceptEntityInput(zombie, "SetParent", pangzi);

    TeleportEntity(zombie, viewOffset, NULL_VECTOR, NULL_VECTOR);
    SetEntityMoveType(zombie, MOVETYPE_NONE);
    
    Han_SetZombieTarget(zombie, zombie, 0.0);
}

public Action Timer_ThrowZombie(Handle timer, DataPack pack)
{
    pack.Reset();
    int pangzi = pack.ReadCell();
    int zombie = pack.ReadCell();
    int victim = pack.ReadCell();
    delete pack;

    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        // PrintToChatAll("僵尸无效，直接返回");
        return Plugin_Stop;
    }

    if (!IsValidEntity(pangzi) || GetEntProp(pangzi, Prop_Data, "m_iHealth") <= 0)
    {
        // PrintToChatAll("胖子无效，没有投掷");
        AcceptEntityInput(zombie, "ClearParent");
        SetEntityMoveType(zombie, MOVETYPE_STEP);
        Han_UnlockZombie(zombie);
        g_bIsPickup[zombie] = false;
        return Plugin_Stop;
    }

    // 发射僵尸
    ThrowZombie(zombie, victim);

    return Plugin_Stop;
}

void ThrowZombie(int zombie, int victim)
{
    // 子弹僵尸解除父子关系，等待dummy实体带飞
    AcceptEntityInput(zombie, "ClearParent");

    // 获取子弹僵尸起始位置和目标位置
    float startPos[3], targetPos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", startPos);      // 已经解除父子关系了，理论上是绝对坐标
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", targetPos);

    // 随即摆动
    targetPos[0] += GetRandomFloat(-PANGZI_THROW_SPREAD, PANGZI_THROW_SPREAD);
    targetPos[1] += GetRandomFloat(-PANGZI_THROW_SPREAD, PANGZI_THROW_SPREAD);

    // 生成辅助实体
    int dummy = CreateEntityByName("prop_dynamic");
    if (dummy != -1)
    {
        /**
         * m_CollisionGroup : https://developer.valvesoftware.com/wiki/Collision_groups
         *     SetEntProp(dummy, Prop_Send, "m_CollisionGroup", 2);
         * m_usSolidFlags : https://wiki.facepunch.com/gmod/Enums/FSOLID
         *     SetEntProp(dummy, Prop_Send, "m_usSolidFlags", xxxx);
         * m_nSolidType : https://developer.valvesoftware.com/wiki/SetSolid()
         *     SetEntProp(dummy, Prop_Send, "m_nSolidType", 2);    // SOLID_BBOX
         *     float mins[] = {-16.0, -16.0, 0.0};
         *     float maxs[] = {16.0, 16.0, 72.0};
         *     SetEntPropVector(dummy, Prop_Send, "m_vecMins", mins);
         *     SetEntPropVector(dummy, Prop_Send, "m_vecMaxs", maxs);
         */
        // 这里用error模型占位，反正看不见
        SetEntityModel(dummy, "models/error.mdl");
        SetEntityRenderMode(dummy, RENDER_NONE);

        // 生成dummy实体并传送初始位置
        DispatchSpawn(dummy);
        TeleportEntity(dummy, startPos, NULL_VECTOR, NULL_VECTOR);

        // 绑定僵尸到dummy实体上
        SetVariantString("!activator");
        AcceptEntityInput(zombie, "SetParent", dummy);

        // 设置dummy实体的碰撞体积
        SetEntProp(dummy, Prop_Send, "m_CollisionGroup", 9);    // 只和世界有碰撞
        SetEntProp(dummy, Prop_Send, "m_nSolidType", 2);    // SOLID_BBOX
        float mins[] = {-32.0, -32.0, 0.0};
        float maxs[] = {32.0, 32.0, 72.0};
        SetEntPropVector(dummy, Prop_Send, "m_vecMins", mins);
        SetEntPropVector(dummy, Prop_Send, "m_vecMaxs", maxs);

        // 计算初始速度（抛物线核心），假设我们希望 1.0 秒飞到
        SetEntityMoveType(dummy, MOVETYPE_FLYGRAVITY);

        float time = PANGZI_THROW_FLYTIME; 
        float gravity = GetConVarFloat(FindConVar("sv_gravity"));
        float vel[3];
        vel[0] = (targetPos[0] - startPos[0]) / time;
        vel[1] = (targetPos[1] - startPos[1]) / time;
        vel[2] = (targetPos[2] - startPos[2]) / time + (0.5 * gravity * time);

        // 发射dummy实体
        TeleportEntity(dummy, NULL_VECTOR, NULL_VECTOR, vel);

        // 延时清理
        DataPack pack = new DataPack();
        pack.WriteCell(zombie);
        pack.WriteCell(dummy);
        CreateTimer(PANGZI_THROW_FLYTIME+0.1, Timer_LandZombie, pack);
    }
}

public Action Timer_LandZombie(Handle timer, DataPack pack)
{
    pack.Reset();
    int zombie = pack.ReadCell();
    int dummy = pack.ReadCell();
    delete pack;

    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        // PrintToChatAll("僵尸无效，直接销毁dummy实体");
    }
    else
    {
        float landPos[3];
        GetEntPropVector(dummy, Prop_Send, "m_vecOrigin", landPos);

        // 子弹僵尸解除父子关系，恢复movetype，重置目标
        AcceptEntityInput(zombie, "ClearParent");

        TeleportEntity(zombie, landPos, NULL_VECTOR, NULL_VECTOR);

        SetEntityMoveType(zombie, MOVETYPE_STEP);
        Han_UnlockZombie(zombie);
        g_bIsPickup[zombie] = false;
    }

    if (IsValidEntity(dummy))
    {
        AcceptEntityInput(dummy, "Kill");
    }

    return Plugin_Stop;
}