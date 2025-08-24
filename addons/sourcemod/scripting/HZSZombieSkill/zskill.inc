//========================================================================================
// 迷雾僵尸技能：受击后立刻产生大量黑烟，阻碍真人玩家视线，一次性技能
//========================================================================================

void CreateSmoke(float pos[3])
{
    pos[2] += 55.0;                                        // 普通体型僵尸参考身高（来自[H-AN-CSS]ZombieScenario.sp）

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
        // ActivateEntity(ent);

        AcceptEntityInput(ent, "TurnOn");

        // TeleportEntity(ent, pos, NULL_VECTOR, NULL_VECTOR);

        CreateTimer(SmokeFadeTime, Timer_StopSmoke, ent);
        CreateTimer(SmokeKillTime, Timer_KillSmoke, ent);
    }

    EmitAmbientSound(SFX_SMOKE1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    EmitAmbientSound(SFX_SMOKE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

public Action Timer_StopSmoke(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        AcceptEntityInput(entity, "TurnOff");        // 烟开始消散
    }

    return Plugin_Continue;
}

public Action Timer_KillSmoke(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        AcceptEntityInput(entity, "Kill");           // 烟彻底消散
    }

    return Plugin_Continue;
}

//========================================================================================
// 自爆僵尸技能：死亡后延时生成爆炸实体，伤害可观，一次性技能
//========================================================================================

public Action Timer_CreateExplosion(Handle timer, DataPack pack)
{
    float pos[3];

    pack.Reset();
    int zombie = pack.ReadCell();
    pos[0] = pack.ReadCell();
    pos[1] = pack.ReadCell();
    pos[2] = pack.ReadCell();
    CloseHandle(pack);

    int ent = CreateEntityByName("env_explosion");

    if (ent != -1)
    {
        char Origin[32];
        Format(Origin, sizeof(Origin), "%f %f %f", pos[0], pos[1], pos[2]);

        DispatchKeyValue(ent, "Origin", Origin);
        DispatchKeyValue(ent, "iMagnitude", ExplodeDamage);
        DispatchKeyValue(ent, "iRadiusOverride", ExplodeRange);

        DispatchSpawn(ent);
        // ActivateEntity(ent);

        AcceptEntityInput(ent, "Explode");                                // 实体会自己删除，不需要特别去kill

        // TeleportEntity(ent, pos, NULL_VECTOR, NULL_VECTOR);            // 傻了，怎么能先爆炸再传送呢

        TE_SetupSparks(pos, view_as<float>({0.0, 0.0, 2.0}), 600, 8);     // 更多的火花效果（参考Rocket HE_ZDT.sp）
        TE_SendToAll();

        if (IsValidEntity(zombie))
            SetEntPropEnt(ent, Prop_Send, "m_hOwnerEntity", zombie);      // 希望爆炸杀伤能返回有效的killer
    }

    EmitAmbientSound(SFX_EXPLODE2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    return Plugin_Continue;
}

//========================================================================================
// 治疗僵尸技能：自出生起，每隔1s为周围僵尸恢复固定血量，无限次数技能
//========================================================================================

public Action Timer_ZombieHealing(Handle timer, int entity)
{
    if (!IsValidEntity(entity) || GetEntProp(entity, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        RepeatTask[entity] = INVALID_HANDLE;
        return Plugin_Continue;
    }

    float pos[3];
    GetEntPropVector(entity, Prop_Send, "m_vecOrigin", pos);

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

            float fTargetOrigin[3];
            GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", fTargetOrigin);

            float fTargetDistance = GetVectorDistance(pos, fTargetOrigin);

            if (fTargetDistance*GAMEUNITS_TO_METERS > HealRange)
            {
                continue;
            }

            SetEntProp(zombie, Prop_Data, "m_iHealth", GetEntProp(zombie, Prop_Data, "m_iHealth") + HealDamage < g_iMaxHealth[zombie] ? GetEntProp(zombie, Prop_Data, "m_iHealth") + HealDamage : g_iMaxHealth[zombie]);
        }
    }

    pos[2] += 50.0;
    WriteParticle("z_group_healer_zombie_buff", pos, 0.1);     // 即便缩短时间，依然有点太密集了

    return Plugin_Continue;
}

//========================================================================================
// 恶魔猎手技能：击飞玩家，无限次数技能
//========================================================================================

void CreateKnockback(float pos[3], int target, float strength[3])
{
    float fTargetPosition[3], fTempAngles[3], fTempPoints[3];
    float vReturn[3];
    
    GetEntPropVector(target, Prop_Send, "m_vecOrigin", fTargetPosition);     // target指的是人类

    MakeVectorFromPoints(pos, fTargetPosition, fTempPoints);
    GetVectorAngles(fTempPoints, fTempAngles);

    vReturn[0] = Cosine(DegToRad(fTempAngles[1])) * strength[0];
    vReturn[1] = Sine(DegToRad(fTempAngles[1])) * strength[1];
    vReturn[2] = strength[2];

    TeleportEntity(target, NULL_VECTOR, NULL_VECTOR, vReturn);
}

//========================================================================================
// 憎恶屠夫技能：生成鬼手模型，控制住人类移动，一次性技能
//========================================================================================

void CreateTrap(int client)
{
    float pos[3];
    GetEntPropVector(client, Prop_Send, "m_vecOrigin", pos);

    int ent = CreateEntityByName("prop_dynamic"); 

    if (ent != -1)
    {
        DispatchKeyValue(ent, "model", "models/heavyzombietrap/zombitrap.mdl");
        DispatchKeyValue(ent, "DefaultAnim", "trap");
        DispatchKeyValue(ent, "HoldAnimation", "true");       // 可有可无，因为trap这个动作很长

        DispatchSpawn(ent);
        // ActivateEntity(ent);
        ActivateEntity(ent);

        TeleportEntity(ent, pos, NULL_VECTOR, NULL_VECTOR);

        CreateTimer(StuckTime, Timer_KillTrap, ent);
    }
}

public Action Timer_KillTrap(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        AcceptEntityInput(entity, "Kill");
    }

    return Plugin_Continue;
}