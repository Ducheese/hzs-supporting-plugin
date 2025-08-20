//========================================================================================
// HZS STOCK
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

stock bool IsZeroPostion(int client)                   // 和hzs_botfakerespawn插件相关，如果人类出现在0 0 0点附近，判断为已死亡
{
    float fClientOrigin[3];
    GetEntPropVector(client, Prop_Send, "m_vecOrigin", fClientOrigin);

    return GetVectorDistance(fClientOrigin, view_as<float>({0.0, 0.0, 0.0})) < 10.0;    // 足够小就行
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
        return Plugin_Continue;
    }

    SetEntProp(zombie, Prop_Data, "m_takedamage", 2, 1);

    return Plugin_Continue;
}

//========================================================================================
// 朝向判断 修改自botstayhere.sp 只输入浮点数组 都用脚底坐标
//========================================================================================

bool IsTargetForward(float ZombiePos[3], float HumanPos[3], float ZombieAng[3])
{
    float fTempPoints[3];
    float fTempAngles[3];

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

    return Plugin_Continue;
}

//========================================================================================
// STOCK
//========================================================================================

stock bool IsValidClient(int client, bool bAlive = false)    // 从sika那挪过来的常用函数
{
    return (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsClientSourceTV(client) && (!bAlive || IsPlayerAlive(client)));
}

stock void Shake(int client, float flAmplitude, float flFrequency, float flDuration)    // 只对活着的玩家有效
{
    Handle hBf = StartMessageOne("Shake", client);

    if (hBf != INVALID_HANDLE)
    {
        BfWriteByte(hBf, 0);
        BfWriteFloat(hBf, flAmplitude);
        BfWriteFloat(hBf, flFrequency);
        BfWriteFloat(hBf, flDuration);
        EndMessage();
    }
}