// ========================================================================================
// 异形斗兽技能：冲撞
// ========================================================================================

void StartDash(int zombie, int victim)
{
    // 基础信息
    char ZombieName[32];
    Han_GetZombieName(zombie, ZombieName, sizeof(ZombieName));

    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);

    // 锁定目标，但冲撞方向是依照僵尸自己的朝向，而不是目标所在的位置
    Han_SetZombieTarget(zombie, victim, 2.5);

    // 冲撞动作
    Han_SetCustomAnimation(zombie, "run_shoot_knife", 2.5);  // 动作的总持续时间（秒）

    // 延时1秒后才开始移动
    CreateTimer(1.0, Timer_DashMovement, zombie);  // 播放动作1秒起才开始产生冲撞位移

    // 延时开放技能占位
    CreateTimer(YIXING_DASH_CD+2.5, Timer_SkillEnd, zombie);

    // 音效
    EmitAmbientSound(SFX_DASH1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 文字提示
    CPrintToChat(victim, "{green}[华仔] {red}小心%s的冲撞！", ZombieName);
}

public Action Timer_DashMovement(Handle timer, int zombie)
{
    // 合法性检测
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        // PrintToChatAll("僵尸无效，直接返回");
        return Plugin_Stop;
    }

    // 音效
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    EmitAmbientSound(SFX_DASH2, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
    EmitAmbientSound(SFX_DASH3, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 冲撞句柄
    g_hCharge[zombie] = CreateTimer(0.1, Timer_DashThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
    g_hMovetype[zombie] = CreateTimer(0.0, Timer_MovetypeThink, zombie, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 持续时间 = 总时间 - 起手时间 - 起身时间，和胖子不一样，都是固定值
    CreateTimer(1.0, Timer_EndDash, zombie);

    return Plugin_Stop;
}

public Action Timer_DashThink(Handle timer, int zombie)
{
    // 合法性检测
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        KillTimer(timer);
        g_hCharge[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    // 施加一个巨大的向前冲力（不再按照目标的位置，而是保持一个固定方向）
    float ang[3];
    GetEntPropVector(zombie, Prop_Send, "m_angRotation", ang);
    float vel[3];
    GetAngleVectors(ang, vel, NULL_VECTOR, NULL_VECTOR);

    // 沿途火花效果
    float pos[3], dir[3];
    dir[0] = -1.0 * vel[0];
    dir[1] = -1.0 * vel[1];
    dir[2] = 0.1;
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    TE_SetupSparks(pos, dir, 600, 16);
    TE_SendToAll();

    // 生成推力
    vel[0] *= YIXING_DASH_FORCE;
    vel[1] *= YIXING_DASH_FORCE;
    vel[2] = 0.0;
    TeleportEntity(zombie, NULL_VECTOR, NULL_VECTOR, vel);

    // 生成撞飞效果
    DashKnockback(zombie);
    
    return Plugin_Continue;
}

public Action Timer_EndDash(Handle timer, int zombie)
{
    // 冲撞结束，清理句柄
    if (g_hCharge[zombie] != INVALID_HANDLE)
    {
        ClearChargeHandles(zombie);

        SetEntityMoveType(zombie, MOVETYPE_STEP);
        // 进入冷却的时间是固定的，所以放到前面去
    }

    return Plugin_Stop;
}

void DashKnockback(int zombie)
{
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsHumanAlive(i))
        {
            if (GetMeterDistance(zombie, i) < YIXING_DASH_RADIUS)
            {
                // 状态管理
                g_bIsStuck[i] = false;
                g_bIsInvert[i] = false;
                
                // 造成伤害
                SDKHooks_TakeDamage(i, zombie, zombie, YIXING_DASH_DAMAGE, DMG_SLASH, _, _, _, false);

                // 生成击退
                CreateKnockback(zombie, i, view_as<float>({800.0, 800.0, 400.0}));

                // 屏幕震动+渐隐
                Shake(i, 30.0, 10.0, 3.0);
                Fade(i, 128, 0, 16, {255, 0, 0, 200});

                // 音效
                EmitSoundToClient(i, SFX_DASH4, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

                // 耳鸣
                CreateTimer(0.2, Timer_Deafness, i);
            }
        }
    }
}

public Action Timer_Deafness(Handle timer, int client)
{
    ClientCommand(client, "dsp_player 37");
    Fade(client, 512, 128, 17, {0, 0, 0, 255});

    return Plugin_Stop;
}

// ========================================================================================
// 异形斗兽技能：震荡波
// ========================================================================================

void StartShockWave(int zombie)
{   
    // 基础信息
    float pos[3];
    GetEntPropVector(zombie, Prop_Send, "m_vecOrigin", pos);
    pos[2] += 20.0;

    // 瞬间闪光
    DynamicLight(pos, 75, 75, 255, 10, 500.0, 0.25, 0.0);

    // 光圈
    TE_SetupBeamRingPoint(pos, 10.0, YIXING_SHOCK_RADIUS / GAMEUNITS_TO_METERS, 
        PrecacheModel("materials/sprites/lgtning.vmt"), 
        PrecacheModel("materials/sprites/halo01.vmt"), 
        1, 1, 0.25, 100.0, 1.0, 
        {75, 75, 255, 255}, 
        0, 0);
    TE_SendToAll();

    // 延时开放技能占位
    CreateTimer(YIXING_SHOCK_CD, Timer_SkillEnd, zombie);

    // 实际影响
    ShockWaveEffect(zombie);

    // 音效
    EmitAmbientSound(SFX_SHOCK1, pos, _, SNDLEVEL_GUNFIRE, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);
}

void ShockWaveEffect(int zombie)
{
    for (int client = 1; client <= MaxClients; client++)
    {
        if (IsHumanAlive(client))
        {
            if (GetMeterDistance(zombie, client) < YIXING_SHOCK_RADIUS)
            {
                // 护甲扣减固定值
                SetEntProp(client, Prop_Send, "m_ArmorValue", 
                    GetEntProp(client, Prop_Send, "m_ArmorValue") - YIXING_SHOCK_ARMOR > 0 ? 
                    GetEntProp(client, Prop_Send, "m_ArmorValue") - YIXING_SHOCK_ARMOR : 
                    0
                );

                // 剥夺当前弹匣
                int weapon = GetEntPropEnt(client, Prop_Send, "m_hActiveWeapon");       
                if (weapon != -1)
                {
                    if (HasEntProp(weapon, Prop_Send, "m_iClip1"))
                    {
                        SetEntProp(weapon, Prop_Send, "m_iClip1", 0);
                    }
                }

                // 记住当前方向
                GetClientEyeAngles(client, g_flAng[client]);

                // 延时结束影响
                g_bIsStock[client] = true;
                CreateTimer(YIXING_SHOCK_TIME, Timer_DeShock, client);

                // 音效（其实想做tesla电弧效果）
                EmitSoundToClient(client, SFX_SHOCK2, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

                // // 剥夺所有clip（个人感觉堪比护甲减半）
                // for (int j = 0; j < 48; j++)
                // {
                //     int weapon = GetEntPropEnt(i, Prop_Data, "m_hMyWeapons", j);
                //     if (weapon != -1)
                //     {
                //         if (HasEntProp(weapon, Prop_Send, "m_iClip1"))
                //         {
                //             SetEntProp(weapon, Prop_Send, "m_iClip1", 0);
                //         }
                //     }
                // }

                // BOT很蠢，切武器约等于剥夺武器
            }
        }
    }
}

public Action Timer_DeShock(Handle timer, int client)
{
    g_bIsStock[client] = false;

    return Plugin_Stop;
}