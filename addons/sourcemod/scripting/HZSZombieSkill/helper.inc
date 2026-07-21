//========================================================================================
// HZS储备函数
//========================================================================================

stock bool IsSameName(int zombie, char string[32])     // 这个僵尸的名字是否和string相同
{
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    return StrEqual(ZombieName, string, false);
}

stock int FindAnimEntity(int zombie)                   // 根据僵尸实体号，返回其动画实体（实体类型为prop_dynamic_ornament）
{
    char targetname[128];
    GetEntPropString(zombie, Prop_Data, "m_iName", targetname, sizeof(targetname));

    return EntRefToEntIndex(StringToInt(targetname));
}

//========================================================================================
// 人类有效性验证
//========================================================================================

stock bool IsValidClient(int client, bool bAlive = false)    // 从sika那挪过来的常用函数
{
    return (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsClientSourceTV(client) && (!bAlive || IsPlayerAlive(client)));
}

stock bool IsZeroPostion(int client)                   // 和hzs_botfakerespawn插件相关，如果人类出现在0 0 0点附近，判断为已死亡
{
    float pos[3];
    GetEntPropVector(client, Prop_Send, "m_vecOrigin", pos);

    return GetVectorDistance(pos, view_as<float>({0.0, 0.0, 0.0})) < 10.0;    // 足够小就行
}

stock bool IsHumanAlive(int client)                    // 判断人类是否活着（包括假死）
{
    if (!IsValidClient(client, true))
    {
        return false;
    }

    if (IsZeroPostion(client))
    {
        return false;
    }

    return true;
}

//========================================================================================
// 计算实体间米制距离
//========================================================================================

stock float GetMeterDistance(int entity1, int entity2)
{
    float pos1[3];
    float pos2[3];

    GetEntPropVector(entity1, Prop_Send, "m_vecOrigin", pos1);
    GetEntPropVector(entity2, Prop_Send, "m_vecOrigin", pos2);
    return GetVectorDistance(pos1, pos2) * GAMEUNITS_TO_METERS;
}

//========================================================================================
// 朝向判断（修改自botstayhere.sp）
//========================================================================================

bool IsTargetForward(int zombie, int human)
{
    float ZombiePos[3], HumanPos[3], ZombieAng[3];

    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", ZombiePos);
    GetEntPropVector(human, Prop_Send, "m_vecOrigin", HumanPos);
    GetEntPropVector(zombie, Prop_Send, "m_angRotation", ZombieAng);

    float fTempPoints[3], fTempAngles[3];

    MakeVectorFromPoints(ZombiePos, HumanPos, fTempPoints);
    GetVectorAngles(fTempPoints, fTempAngles);

    // Differenz&x
    float fDiffz = ZombieAng[1] - fTempAngles[1];     // z管水平方向转动，眼睛看的方向，与连线方向的夹角，欧拉角里叫做“偏航角”

    // Correct it
    if (fDiffz < -180)
        fDiffz = 360 + fDiffz;                        // 调整到在-180到+180之间

    if (fDiffz > 180)
        fDiffz = 360 - fDiffz;

    if (fDiffz >= -67.5 && fDiffz <= 67.5)
    {
        return true;
    }

    return false;
}

//========================================================================================
// 僵尸无敌时间（修改自sm_dmlite.sp 更适合NPC的合法性验证）
//========================================================================================

stock void ProtectZombie(int zombie, float duration)
{
    SetEntProp(zombie, Prop_Data, "m_takedamage", 0, 1);
    CreateTimer(duration, Timer_RemoveProtect, zombie);
}

public Action Timer_RemoveProtect(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        return Plugin_Stop;
    }

    SetEntProp(zombie, Prop_Data, "m_takedamage", 2, 1);

    return Plugin_Stop;
}

//========================================================================================
// 粒子生成和消灭（修改自CSS Blood.sp）
//========================================================================================

stock void WriteParticle(char[] ParticleName, float ImpactOrigin[3], float duration)
{
    int ent = CreateEntityByName("info_particle_system");

    if (ent != -1)
    {
        PushEntRef(g_iParticle, PARTICLE_POOL, ent);

        DispatchKeyValue(ent, "effect_name", ParticleName);

        DispatchSpawn(ent);
        ActivateEntity(ent);

        AcceptEntityInput(ent, "start");
        AcceptEntityInput(ent, "SetParent", ent, ent, 0);

        TeleportEntity(ent, ImpactOrigin, NULL_VECTOR, NULL_VECTOR);

        CreateTimer(duration, Timer_DeleteParticle, ent);
    }
}

public Action Timer_DeleteParticle(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        char classname[64];
        GetEdictClassname(entity, classname, sizeof(classname));

        if (StrEqual(classname, "info_particle_system", false))
        {
            PopEntRef(g_iParticle, PARTICLE_POOL, entity);
            AcceptEntityInput(entity, "Kill");
        }
    }

    return Plugin_Stop;
}

//========================================================================================
// 计算到地面距离（飞行和冲锋逻辑都会用到）
//========================================================================================

stock float GetDistanceToGround(int entity)
{
    float pos[3];
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", pos);

    Handle trace = TR_TraceRayFilterEx(pos, {90.0, 0.0, 0.0}, MASK_PLAYERSOLID, RayType_Infinite, TraceEntityFilter_NotClient, entity);

    if (!TR_DidHit(trace))
    {
        delete trace;
        return 9999.0;  // 脚下虚空（天空盒/地图外），视为无限高
    }

    float endPos[3];
    TR_GetEndPosition(endPos, trace);
    delete trace;

    return pos[2] - endPos[2];
}

//========================================================================================
// 屏幕震动 & 渐变色（UserMessage: Shake / Fade）
//========================================================================================

/**
 * @param client      目标玩家
 * @param flAmplitude 振幅
 * @param flFrequency 频率
 * @param flDuration  持续时间（秒）
 */
stock void Shake(int client, float flAmplitude, float flFrequency, float flDuration)    // 只对活着的玩家有效
{
    Handle hBf = StartMessageOne("Shake", client);   // hBf (Handle to Buffer)

    if (hBf != INVALID_HANDLE)
    {
        BfWriteByte(hBf, 0);
        BfWriteFloat(hBf, flAmplitude);
        BfWriteFloat(hBf, flFrequency);
        BfWriteFloat(hBf, flDuration);
        EndMessage();
    }
}

/**
 * @param client   目标玩家
 * @param duration 渐入时长（ms）
 * @param holdtime 保持时长（ms）
 * @param flags    渐变标志（FFADE_IN / FFADE_OUT 等）
 * @param color    RGBA 颜色
 */
stock void Fade(int client, int duration, int holdtime, int flags, int color[4])    // 对已经死了的玩家也有效
{
    Handle hBf = StartMessageOne("Fade", client, 0);

    if (hBf != INVALID_HANDLE)
    {
        BfWriteShort(hBf, duration);
        BfWriteShort(hBf, holdtime);
        BfWriteShort(hBf, flags);
        BfWriteByte(hBf, color[0]);
        BfWriteByte(hBf, color[1]);
        BfWriteByte(hBf, color[2]);
        BfWriteByte(hBf, color[3]);
        EndMessage();
    }
}

//========================================================================================
// 瞬时动态光（TE: Dynamic Light）
//========================================================================================

stock void DynamicLight(float pos[3], int r, int g, int b, int exponent, float radius, float time, float decay)
{
    /**
     * @param pos          光源位置
     * @param r, g, b      颜色 (0-255)
     * @param exponent     衰减指数（通常设为 8-10，决定光的硬度）
     * @param radius       光照半径
     * @param time         持续时间（秒）
     * @param decay        衰减速度
     */
    TE_Start("Dynamic Light");
    TE_WriteVector("m_vecOrigin", pos);
    TE_WriteNum("r", r);
    TE_WriteNum("g", g);
    TE_WriteNum("b", b);
    TE_WriteNum("exponent", exponent);
    TE_WriteFloat("m_fRadius", radius);
    TE_WriteFloat("m_fTime", time);
    TE_WriteFloat("m_fDecay", decay);
    TE_SendToAll();
}

//========================================================================================
// 碰撞箱可视化（Debug: 光束画12条棱）
//========================================================================================

stock void DebugDrawBBox(int entity, float duration, int color[4])
{
    float origin[3], mins[3], maxs[3];
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", origin);
    GetEntPropVector(entity, Prop_Send, "m_vecMins", mins);
    GetEntPropVector(entity, Prop_Send, "m_vecMaxs", maxs);

    // 8个顶点
    float v[8][3];
    for (int i = 0; i < 8; i++)
    {
        v[i][0] = origin[0] + ((i & 1) ? maxs[0] : mins[0]);
        v[i][1] = origin[1] + ((i & 2) ? maxs[1] : mins[1]);
        v[i][2] = origin[2] + ((i & 4) ? maxs[2] : mins[2]);
    }

    // 12条棱的顶点对
    int edges[12][2] = {
        {0,1},{0,2},{0,4},{1,3},{1,5},
        {2,3},{2,6},{3,7},{4,5},{4,6},
        {5,7},{6,7}
    };

    int sprite = PrecacheModel(TOOL_BEAMSPRITE, true);
    for (int i = 0; i < 12; i++)
    {
        TE_SetupBeamPoints(v[edges[i][0]], v[edges[i][1]], sprite, 0, 0, 0, duration, 1.0, 1.0, 1, 0.0, color, 0);
        TE_SendToAll();
    }
}

//========================================================================================
// 半径可视化（Debug: 24段光束画圆环）
//========================================================================================

stock void DebugDrawRadius(const float center[3], float radius, int color[4], float duration)
{
    int sprite = PrecacheModel(TOOL_BEAMSPRITE, true);
    float ground[3];
    ground = center;

    Handle tr = TR_TraceRayFilterEx(center, view_as<float>({90.0, 0.0, 0.0}), MASK_SOLID, RayType_Infinite, TraceEntityFilter_Ground);
    if (TR_DidHit(tr))
    {
        float end[3];
        TR_GetEndPosition(end, tr);
        ground[2] = end[2] + 2.0;
    }
    delete tr;

    int segments = 24;
    float prev[3];
    for (int i = 0; i <= segments; i++)
    {
        float ang = 2.0 * FLOAT_PI * float(i) / float(segments);
        float cur[3];
        cur[0] = ground[0] + radius * Cosine(ang);
        cur[1] = ground[1] + radius * Sine(ang);
        cur[2] = ground[2];

        if (i > 0)
        {
            TE_SetupBeamPoints(prev, cur, sprite, 0, 0, 0, duration, 2.0, 2.0, 1, 0.0, color, 0);
            TE_SendToAll();
        }
        prev = cur;
    }
}

//========================================================================================
// Trace 过滤器
//========================================================================================

/**
 * 只命中世界几何（entity == 0），忽略所有实体。
 */
public bool TraceEntityFilter_Ground(int entity, int mask, any data)
{
    return (entity == 0);
}

/**
 * 忽略自身（data）以及所有玩家实体。
 */
public bool TraceEntityFilter_NotClient(int entity, int mask, any data)
{
    if (entity == data || (entity >= 1 && entity <= MaxClients))
        return false;

    return true;
}

/**
 * 只排除自身，不过滤其他任何实体
 */
public bool TraceEntityFilter_Self(int entity, int mask, any data)
{
    return entity != data;
}

//========================================================================================
// 传送碰撞保护（防卡人）
//========================================================================================

/**
 * 检测实体是否与其他物体重叠
 * 写法和fakerespawn一样
 */
stock int IsEntityStuck(int entity)
{
    float vecMin[3], vecMax[3], vecOrigin[3];
    GetEntPropVector(entity, Prop_Send, "m_vecMins", vecMin);
    GetEntPropVector(entity, Prop_Send, "m_vecMaxs", vecMax);
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", vecOrigin);
    TR_TraceHullFilter(vecOrigin, vecOrigin, vecMin, vecMax, MASK_PLAYERSOLID, TraceEntityFilter_Self, entity);
    return TR_GetEntityIndex();
}

/**
 * 消除碰撞，每秒检测一次，不卡了自动恢复
 */
stock void StartUnstuck(int entity)
{
    SetEntProp(entity, Prop_Data, "m_CollisionGroup", 2);    // COLLISION_GROUP_DEBRIS
    CreateTimer(1.0, Timer_UnstuckThink, EntIndexToEntRef(entity), TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

public Action Timer_UnstuckThink(Handle timer, int ref)
{
    int entity = EntRefToEntIndex(ref);
    if (entity == INVALID_ENT_REFERENCE || !IsValidEntity(entity) || IsEntityStuck(entity) == -1)
    {
        if (entity != INVALID_ENT_REFERENCE && IsValidEntity(entity))
            SetEntProp(entity, Prop_Data, "m_CollisionGroup", 5);    // COLLISION_GROUP_PLAYER
        return Plugin_Stop;
    }
    return Plugin_Continue;
}

//========================================================================================
// 检测脚下天空盒并传送至随机 T 复活点
//========================================================================================

stock bool TeleportOffSkybox(int entity)
{
    // 朝脚下打射线，检测是否站在天空盒上（头顶 90 度俯角穿透一切）
    float pos[3];
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", pos);

    Handle tr = TR_TraceRayFilterEx(pos, view_as<float>({90.0, 0.0, 0.0}), MASK_SOLID, RayType_Infinite, TraceEntityFilter_NotClient, entity);

    bool bHit = TR_DidHit(tr);
    bool bSkybox = !bHit;
    if (bHit)
    {
        // 表面属性 76 = sky（CS:S 的天空盒材质）
        bSkybox = TR_GetSurfaceProps(tr) == 76;
    }
    delete tr;

    if (!bSkybox)
        return false;

    int n = GetArraySize(g_hSpawnPoint);
    if (n <= 0)
        return false;

    int ref = GetArrayCell(g_hSpawnPoint, GetRandomInt(0, n - 1));
    if (ref == INVALID_ENT_REFERENCE)
        return false;

    float sp[3], sa[3];
    GetEntPropVector(ref, Prop_Send, "m_vecOrigin", sp);
    GetEntPropVector(ref, Prop_Send, "m_angRotation", sa);
    TeleportEntity(entity, sp, sa, NULL_VECTOR);
    return true;
}

//========================================================================================
// 扫描最近存活人类（适用于任意实体，如僵尸、传送门等）
//========================================================================================

stock int FindNearestHuman(int entity)
{
    int target = -1;
    float origin[3], bestDist = 999999.0;
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", origin);

    for (int i = 1; i <= MaxClients; i++)
    {
        if (!IsHumanAlive(i))
            continue;
        float tp[3];
        GetClientEyePosition(i, tp);
        float d = GetVectorDistance(origin, tp);
        if (d < bestDist) { bestDist = d; target = i; }
    }

    return target;
}

//========================================================================================
// 随机选取一个存活人类
//========================================================================================

stock int PickRandomHuman()
{
    int humans[MAXPLAYERS+1];
    int count = 0;

    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsHumanAlive(i))
            humans[count++] = i;
    }

    if (count == 0)
        return -1;

    return humans[GetRandomInt(0, count - 1)];
}

//========================================================================================
// 判断人类是否在移动（速度 > 50 units/s）
//========================================================================================

stock bool IsHumanMoving(int client)
{
    float vel[3];
    GetEntPropVector(client, Prop_Data, "m_vecVelocity", vel);
    return GetVectorLength(vel) > 50.0;
}

//========================================================================================
// T 复活点收集
//========================================================================================

stock void CollectTSpawnPoints()
{
    ClearArray(g_hSpawnPoint);
    int entity = -1;
    while ((entity = FindEntityByClassname(entity, "info_player_terrorist")) != -1)
        PushArrayCell(g_hSpawnPoint, EntIndexToEntRef(entity));
}

//========================================================================================
// Bot 自动捡最近武器
//========================================================================================

stock void BotFindWeapon(int client)
{
    if (!g_bBotFindWeapon || g_hHideCall == INVALID_HANDLE)
        return;

    // 优先捡主武器
    if (GetPlayerWeaponSlot(client, 0) == -1)
    {
        FindNearestWeapon(client, 0);
        return;
    }

    // 有主武器但缺副武器
    if (GetPlayerWeaponSlot(client, 1) == -1)
        FindNearestWeapon(client, 1);
}

stock void FindNearestWeapon(int client, int slot)
{
    float now = GetEngineTime();
    if (now - g_flBotFindThrottle[client] <= 3.0)
        return;
    g_flBotFindThrottle[client] = now;

    float botPos[3];
    GetClientAbsOrigin(client, botPos);

    int bestEnt = -1;
    float bestDist = 999999.0;
    int e = -1;
    while ((e = FindEntityByClassname(e, "weapon_*")) != -1)
    {
        if (GetEntPropEnt(e, Prop_Data, "m_hOwnerEntity") != -1)
            continue;
        if (SDKCall(g_hGetWeaponSlot, e) != slot)
            continue;

        float pos[3];
        GetEntPropVector(e, Prop_Send, "m_vecOrigin", pos);
        float dist = GetVectorDistance(botPos, pos, false);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestEnt = e;
        }
    }

    if (bestEnt == -1)
        return;

    float targetPos[3];
    GetEntPropVector(bestEnt, Prop_Send, "m_vecOrigin", targetPos);
    SDKCall(g_hHideCall, client, targetPos, 0, false);

    // debug 输出
    float zEye[3], wPos[3];
    GetClientEyePosition(client, zEye);
    wPos = targetPos;
    TE_SetupBeamPoints(zEye, wPos, PrecacheModel(TOOL_BEAMSPRITE), 0, 0, 0, 0.15, 1.5, 1.5, 0, 0.0, {0, 255, 255, 255}, 0);
    TE_SendToAll();
}

//========================================================================================
// 强制缴械（不缴刀）
//========================================================================================

stock void DropActiveWeapon(int client)
{
    int weapon = GetEntPropEnt(client, Prop_Data, "m_hActiveWeapon");
    if (weapon != -1 && weapon != GetPlayerWeaponSlot(client, 2))
        CS_DropWeapon(client, weapon, true, true);
}
