//========================================================================================
// 僵尸无视距离全图索敌（Patch: GiveUpJA + DHook: CHostage_Idle）
//========================================================================================

void PatchGiveUp()
{
    Handle gc = LoadGameConfigFile(GAMEDATA_CUSTOM);
    if (gc == INVALID_HANDLE)
        SetFailState("[hzs_zombieskill] Failed to load gamedata");

    Address jaAddr = GameConfGetAddress(gc, "GiveUpAddr");
    if (jaAddr == Address_Null)
        SetFailState("[hzs_zombieskill] Failed to resolve GiveUp address");

    // 0F 87 xx xx xx xx → 6x NOP（跳过 distance>2000 的 giveUp return）
    for (int i = 0; i < 6; i++)
        StoreToAddress(jaAddr + view_as<Address>(i), 0x90, NumberType_Int8);

    LogMessage("[hzs_zombieskill] Patched GiveUpJA at %X", jaAddr);

    delete gc;
}

void PrepIdleDetour()
{
    Handle gc = LoadGameConfigFile(GAMEDATA_CUSTOM);
    if (gc == INVALID_HANDLE)
        SetFailState("[hzs_zombieskill] Failed to load gamedata");

    g_hIdleDetour = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_CBaseEntity);
    DHookSetFromConf(g_hIdleDetour, gc, SDKConf_Signature, "CHostage_Idle");

    if (!DHookEnableDetour(g_hIdleDetour, false, OnIdle_Pre))
        SetFailState("[hzs_zombieskill] Failed to enable CHostage_Idle detour");

    delete gc;
}

public MRESReturn OnIdle_Pre(int zombie)
{
    return MRES_Supercede;
}

//========================================================================================
// 避免颜色校正实体影响所有人（DHook: CColorCorrection::UpdateTransmitState）
//========================================================================================

void PrepCCDetour()
{
    // CColorCorrection vtable[19] = UpdateTransmitState（IDA 确认）
    g_hShouldTransmitDetour = DHookCreate(19, HookType_Entity, ReturnType_Int, ThisPointer_CBaseEntity);
    if (g_hShouldTransmitDetour == INVALID_HANDLE)
        LogError("[hzs_zombieskill] Failed to create UpdateTransmitState vtable hook");
}

public MRESReturn OnCCUpdateTransmitState_Pre(int entity, Handle hReturn)
{
    DHookSetReturn(hReturn, 0);
    return MRES_Supercede;
}
