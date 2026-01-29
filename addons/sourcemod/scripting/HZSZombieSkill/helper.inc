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
// 朝向判断 修改自botstayhere.sp
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
// 僵尸无敌时间 修改自sm_dmlite.sp 更适合NPC的合法性验证
//========================================================================================

void ProtectZombie(int zombie, float duration)
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
// 粒子生成和消灭 来自CSS Blood.sp
//========================================================================================

void WriteParticle(char[] ParticleName, float ImpactOrigin[3], float duration)
{
    int ent = CreateEntityByName("info_particle_system");
    
    if (ent != -1)
    {
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
            RemoveEdict(entity);
        }
    }

    return Plugin_Stop;
}

//========================================================================================
// 计算到地面距离 飞行和冲锋逻辑都会用到
//========================================================================================

void GetDistanceToGround(int entity, float &dist, float fRayEndPoint[3])
{
    float fRayStartPoint[3];
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", fRayStartPoint);

    Handle trace = TR_TraceRayFilterEx(fRayStartPoint, {90.0, 0.0, 0.0}, MASK_PLAYERSOLID, RayType_Infinite, TraceEntityFilter_NotClient, entity);      // 方向向下
    TR_GetEndPosition(fRayEndPoint, trace);

    dist = fRayStartPoint[2] - fRayEndPoint[2];
}

public bool TraceEntityFilter_NotClient(int entity, int mask, any data)
{
    if (entity == data || (entity >= 1 && entity <= MaxClients))     // 忽略自身，以及人类client实体
    {
        return false;
    }

    return true;
}

//========================================================================================
// 特殊效果：屏幕晃动、屏幕渐变、闪光
//========================================================================================

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

stock void DynamicLight(float pos[3], int r, int g, int b, int exponent, float radius, float time, float decay)
{
    /**
     * 在位置生成一个瞬时动态光
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