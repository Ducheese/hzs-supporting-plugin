//========================================================================================
// 嗜血女巫技能：致幻（两阶段：接近→灰度，长时间后→致盲致聋+方向键旋转）
//========================================================================================

// 每只 Witch 自己的 near 检查 timer
public Action Timer_WitchNearThink(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        g_hWitchNear[zombie] = INVALID_HANDLE;

        // 灾变插件可能直接 kill 所有女巫而不触发死亡事件
        // 如果没有任何存活的 Witch，清理所有 Phase 1 受害者
        bool anyAlive;
        for (int i = 1; i <= 2048; i++)
        {
            if (IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH))
            {
                anyAlive = true;
                break;
            }
        }

        /**
         * 理论上如果有多个女巫在场然后被kill了
         * 这里的清理会重复执行，不过只要不报错就无所谓
         */
        if (!anyAlive)
            CleanupOrphanedWitchPhase1();

        return Plugin_Stop;
    }

    /**
     * 因为是具体某个witch和玩家的距离计算
     * 所以不是“首个witch执行”的模式
     */
    for (int victim = 1; victim <= MaxClients; victim++)
    {
        if (!IsHumanAlive(victim))
            continue;

        float dist = GetMeterDistance(zombie, victim);

        if (dist <= WITCH_NEAR_RADIUS)
        {
            if (g_iWitchPhase[victim] == 0)
            {
                StartWitchPhase1(victim);
            }
            else if (g_iWitchPhase[victim] == 1)
            {
                /**
                 * 多个witch在旁边，计时叠加
                 */
                g_flWitchNearTime[victim] += 0.1;
                if (g_flWitchNearTime[victim] >= WITCH_NEAR_TIME)
                    StartWitchPhase2(victim);
            }
        }
        // out-of-range 不在此 timer 处理，Phase 1/2 清理见下方
    }

    // 清理失去覆盖的Phase 1玩家（仅首个witch执行，避免重复）
    /**
     * 1号witch负责全局phase1状态更新的think
     */
    int firstWitch = -1;
    for (int i = 1; i <= 2048; i++)
    {
        if (IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH))
        {
            firstWitch = i;
            break;
        }
    }
    if (zombie == firstWitch)
        CleanupOrphanedWitchPhase1();

    return Plugin_Continue;
}

// 清理失去所有女巫覆盖的 Phase 1 玩家（可选排除指定实体）
void CleanupOrphanedWitchPhase1(int excludeEntity = -1)
{
    for (int victim = 1; victim <= MaxClients; victim++)
    {
        if (g_iWitchPhase[victim] != 1)
            continue;

        bool anyNear;
        for (int i = 1; i <= 2048; i++)
        {
            if (i != excludeEntity && IsValidEntity(i)
            && IsSameName(i, ZOMBIE_WITCH)
            && GetMeterDistance(i, victim) <= WITCH_NEAR_RADIUS)
            {
                anyNear = true;
                break;
            }
        }

        if (!anyNear)
        {
            g_iWitchPhase[victim] = 0;
            g_flWitchNearTime[victim] = 0.0;
            KillWitchCC(victim);
        }
    }
}

// 第一阶段：普通灰度
void StartWitchPhase1(int victim)
{
    if (g_iWitchPhase[victim] >= 1)
        return;

    g_iWitchPhase[victim] = 1;
    int cc = CreateWitchCC(victim, LUT_WITCH_MILD);
    if (cc != -1)
        g_iWitchMildCC[victim] = cc;
}

// 第二阶段：极端灰度 + 隐藏模型 + 按键旋转 + 音频高切
void StartWitchPhase2(int victim)
{
    if (g_iWitchPhase[victim] == 2)
        return;

    g_iWitchPhase[victim] = 2;
    g_bIsBlind[victim] = true;
    g_iWitchKeyRot[victim] = GetRandomInt(0, 2);

    // 关闭第一阶段 CC
    KillWitchCC(victim);

    // 开启第二阶段 CC（极端灰度 + 色阶量化）
    int cc2 = CreateWitchCC(victim, LUT_WITCH_SEVERE);
    if (cc2 != -1)
        g_iWitchSevereCC[victim] = cc2;

    // 高频切除：交替 33↔34，每次在 1.6s 过期前（1.4s）切到另一个
    // 33 和 34 链路完全相同（DFR + FLT LP 1kHz），切换听不出区别
    g_bWitchDSPFlip[victim] = false;
    ClientCommand(victim, "dsp_player 33");
    if (g_hWitchDSPToggle[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchDSPToggle[victim]);
        g_hWitchDSPToggle[victim] = INVALID_HANDLE;
    }
    g_hWitchDSPToggle[victim] = CreateTimer(1.4, Timer_WitchDSPToggle, victim, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);

    // 致幻音乐（wav 自带循环标记才会 loop）
    EmitSoundToClient(victim, SFX_WITCH2, _, SNDCHAN_STATIC);

    // 提示（好蠢的写法）
    char ZombieName[32];
    for (int i = 1; i <= 2048; i++)
    {
        if (IsValidEntity(i) && IsSameName(i, ZOMBIE_WITCH))
        {
            Han_GetZombieName(i, ZombieName, sizeof(ZombieName));
            break;
        }
    }
    CPrintToChat(victim, "{green}[华仔] {red}你被%s的幻术吞噬了！", ZombieName);
    EmitSoundToClient(victim, SFX_WITCH1, _, SNDCHAN_STATIC, SNDLEVEL_NORMAL, SND_NOFLAGS, SNDVOL_NORMAL, SNDPITCH_NORMAL);

    // 10 秒自毁（先 kill 旧的防泄露）
    if (g_hWitchBlind[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchBlind[victim]);
        g_hWitchBlind[victim] = INVALID_HANDLE;
    }
    g_hWitchBlind[victim] = CreateTimer(WITCH_BLIND_TIME, Timer_DeBlind, victim);
}

// 创建 per-player CC 实体（通用）
int CreateWitchCC(int victim, const char[] lutFile)
{
    int cc = CreateEntityByName("color_correction");
    if (cc == -1)
        return -1;

    DispatchKeyValue(cc, "filename", lutFile);
    DispatchKeyValue(cc, "maxweight", "1.0");
    DispatchKeyValue(cc, "fadeInDuration", "0.3");
    DispatchKeyValue(cc, "fadeOutDuration", "0.3");
    DispatchKeyValue(cc, "minfalloff", "-1");
    DispatchKeyValue(cc, "maxfalloff", "-1");
    DispatchKeyValue(cc, "StartDisabled", "1");

    /**
     * 必须在 Spawn 前（Spawn → UpdateTransmitState → FL_EDICT_ALWAYS）
     * per-player显示的关键，光靠SDKHook_SetTransmit是不起作用的
     */
    DHookEntity(g_hShouldTransmitDetour, false, cc, _, OnCCUpdateTransmitState_Pre);

    DispatchSpawn(cc);
    ActivateEntity(cc);

    /**
     * 理论上位置是无所谓的
     */
    float victimPos[3];
    GetEntPropVector(victim, Prop_Send, "m_vecOrigin", victimPos);
    TeleportEntity(cc, victimPos, NULL_VECTOR, NULL_VECTOR);

    g_iWitchCCOwner[cc] = victim;
    SDKHook(cc, SDKHook_SetTransmit, OnWitchCCSetTransmit);
    AcceptEntityInput(cc, "Enable");
    return cc;
}

// 销毁玩家所有 CC 实体
void KillWitchCC(int client)
{
    int cc1 = g_iWitchMildCC[client];
    if (cc1 > 0 && IsValidEntity(cc1))
    {
        SDKUnhook(cc1, SDKHook_SetTransmit, OnWitchCCSetTransmit);
        g_iWitchCCOwner[cc1] = -1;
        AcceptEntityInput(cc1, "Kill");
    }
    g_iWitchMildCC[client] = -1;

    int cc2 = g_iWitchSevereCC[client];
    if (cc2 > 0 && IsValidEntity(cc2))
    {
        SDKUnhook(cc2, SDKHook_SetTransmit, OnWitchCCSetTransmit);
        g_iWitchCCOwner[cc2] = -1;
        AcceptEntityInput(cc2, "Kill");
    }
    g_iWitchSevereCC[client] = -1;
}

// 解除致盲（正常到期或被震荡波打断）
void EndWitchBlind(int victim)
{
    g_bIsBlind[victim] = false;
    g_iWitchPhase[victim] = 0;
    g_flWitchNearTime[victim] = 0.0;

    // 停止 DSP 和音乐
    ClientCommand(victim, "dsp_player 0");
    StopSound(victim, SNDCHAN_STATIC, SFX_WITCH2);

    // 清理 timer
    if (g_hWitchBlind[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchBlind[victim]);
        g_hWitchBlind[victim] = INVALID_HANDLE;
    }
    if (g_hWitchDSPToggle[victim] != INVALID_HANDLE)
    {
        KillTimer(g_hWitchDSPToggle[victim]);
        g_hWitchDSPToggle[victim] = INVALID_HANDLE;
    }

    // 重置按键映射
    g_iWitchKeyRot[victim] = -1;

    // 杀所有 CC
    KillWitchCC(victim);
}

public Action Timer_DeBlind(Handle timer, int client)
{
    g_hWitchBlind[client] = INVALID_HANDLE;
    if (g_bIsBlind[client])
        EndWitchBlind(client);
    return Plugin_Stop;
}

// 交替 33↔34 防 DSP 过期（两者 DFR + FLT LP 1kHz 链路完全相同）
public Action Timer_WitchDSPToggle(Handle timer, int victim)
{
    if (!IsHumanAlive(victim) || !g_bIsBlind[victim])
    {
        g_hWitchDSPToggle[victim] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    g_bWitchDSPFlip[victim] = !g_bWitchDSPFlip[victim];
    ClientCommand(victim, "dsp_player %d", g_bWitchDSPFlip[victim] ? 34 : 33);
    return Plugin_Continue;
}

// 致盲状态下的模型隐藏：SetTransmit 回调（不隐藏受害者自己）
public Action OnWitchBlindSetTransmit(int entity, int client)
{
    if (g_bIsBlind[client] && entity != client)
        return Plugin_Handled;
    return Plugin_Continue;
}

public Action OnWitchCCSetTransmit(int cc, int client)
{
    // 只传给受害者
    if (client == g_iWitchCCOwner[cc])
        return Plugin_Continue;
    return Plugin_Handled;
}
