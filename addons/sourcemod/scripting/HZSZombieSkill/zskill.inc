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
            SetEntPropEnt(ent, Prop_Send, "m_hOwnerEntity", zombie);      // 希望爆炸杀伤能返回有效的killer
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

            if (fTargetDistance*GAMEUNITS_TO_METERS > HEAL_RANGE)
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

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 生成击退
    Shake(victim, 30.0, 10.0, 3.0);
    CreateKnockback(pos, victim, view_as<float>({DEIMOS_KNOCKPOWER, DEIMOS_KNOCKPOWER, DEIMOS_KNOCKPOWER}));

    // 击退音效
    EmitSoundToClient(victim, SFX_KNOCKBACK, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}你被%s击飞了！", ZombieName);
}

void CreateKnockback(float pos[3], int target, float strength[3])
{
    float fTargetOrigin[3], fTempAngles[3], fTempPoints[3];
    float vReturn[3];
    
    GetEntPropVector(target, Prop_Send, "m_vecOrigin", fTargetOrigin);     // target指的是人类

    MakeVectorFromPoints(pos, fTargetOrigin, fTempPoints);
    GetVectorAngles(fTempPoints, fTempAngles);

    vReturn[0] = Cosine(DegToRad(fTempAngles[1])) * strength[0];
    vReturn[1] = Sine(DegToRad(fTempAngles[1])) * strength[1];
    vReturn[2] = strength[2];

    TeleportEntity(target, NULL_VECTOR, NULL_VECTOR, vReturn);
}

//========================================================================================
// 憎恶屠夫技能：生成鬼手模型，控制住人类移动，一次性技能
//========================================================================================

void StartTrap(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float fTargetOrigin[3];
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", fTargetOrigin);   // 是人的位置

    // 生成陷阱
    CreateTrap(fTargetOrigin);
    CreateTimer(BUTCHER_STUCKTIME, Timer_DeStuck, victim);

    // 音效
    EmitSoundToClient(victim, SFX_BUTCHER, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}你被%s的鬼手控制住了！", ZombieName);
}

void CreateTrap(float pos[3])
{
    // 生成鬼手实体
    int ent = CreateEntityByName("prop_dynamic"); 

    if (ent != -1)
    {
        SetEntProp(ent, Prop_Send, "m_nModelIndex", g_iZombieTrap);
        
        DispatchKeyValue(ent, "DefaultAnim", "trap");
        DispatchKeyValue(ent, "HoldAnimation", "true");       // 可有可无，因为trap这个动作很长
        DispatchSpawn(ent);

        TeleportEntity(ent, pos, NULL_VECTOR, NULL_VECTOR);

        CreateTimer(BUTCHER_STUCKTIME, Timer_KillTrap, ent);
    }
}

public Action Timer_DeStuck(Handle timer, int client)
{
    g_bIsStuck[client] = false;

    return Plugin_Stop;
}

public Action Timer_KillTrap(Handle timer, int entity)
{
    if (IsValidEntity(entity))
    {
        AcceptEntityInput(entity, "Kill");
    }

    return Plugin_Stop;
}

//========================================================================================
// 幽灵僵尸技能：身形几乎不可见
//========================================================================================

void StartInvert(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    // 延时结束影响
    CreateTimer(WITCH_INVERTTIME, Timer_DeInvert, victim);

    // 音效
    EmitSoundToClient(victim, SFX_WITCH, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}你中了%s的幻术！", ZombieName);
}

public Action Timer_DeInvert(Handle timer, int client)
{
    g_bIsInvert[client] = false;

    return Plugin_Stop;
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