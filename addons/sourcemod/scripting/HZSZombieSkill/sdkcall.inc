
//========================================================================================
// 运行时天空盒切换（SDKCall: R_LoadNamedSkys）
//========================================================================================

void PrepSkySDKCall()
{
    Handle gc = LoadGameConfigFile(GAMEDATA_CUSTOM);
    if (gc == INVALID_HANDLE)
    {
        LogError("[hzs_zombieskill] Failed to load gamedata");
        return;
    }

    StartPrepSDKCall(SDKCall_Static);
    if (!PrepSDKCall_SetFromConf(gc, SDKConf_Signature, "R_LoadNamedSkys"))
    {
        LogError("[hzs_zombieskill] Failed to find signature for R_LoadNamedSkys");
        delete gc;
        return;
    }
    PrepSDKCall_AddParameter(SDKType_String, SDKPass_Pointer);
    PrepSDKCall_SetReturnInfo(SDKType_Bool, SDKPass_Plain);
    g_hRLoadNamedSkys = EndPrepSDKCall();

    if (g_hRLoadNamedSkys == INVALID_HANDLE)
        LogError("[hzs_zombieskill] Failed to prepare SDKCall for R_LoadNamedSkys");

    delete gc;
}

//========================================================================================
// 获取武器弹匣容量（SDKCall: GetMaxClip1）
//========================================================================================

void PrepMaxClipSDKCall()
{
    Handle gc = LoadGameConfigFile(GAMEDATA_CUSTOM);
    if (gc == INVALID_HANDLE)
    {
        LogError("[hzs_zombieskill] Failed to load gamedata");
        return;
    }

    StartPrepSDKCall(SDKCall_Entity);
    if (!PrepSDKCall_SetFromConf(gc, SDKConf_Signature, "GetMaxClip1"))
    {
        LogError("[hzs_zombieskill] Failed to find signature for GetMaxClip1");
        delete gc;
        return;
    }
    PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_ByValue);
    g_hGetMaxClip1 = EndPrepSDKCall();

    if (g_hGetMaxClip1 == INVALID_HANDLE)
        LogError("[hzs_zombieskill] Failed to prepare SDKCall for GetMaxClip1");

    delete gc;
}

//========================================================================================
// 获取武器槽位（SDKCall: GetWeaponSlot）
//========================================================================================

void PrepGetWeaponSlot()
{
    Handle gc = LoadGameConfigFile(GAMEDATA_CUSTOM);
    if (gc == INVALID_HANDLE)
    {
        LogError("[hzs_zombieskill] Failed to load gamedata");
        return;
    }

    StartPrepSDKCall(SDKCall_Entity);
    if (!PrepSDKCall_SetFromConf(gc, SDKConf_Signature, "GetWeaponSlot"))
    {
        LogError("[hzs_zombieskill] Failed to find signature for GetWeaponSlot");
        delete gc;
        return;
    }
    PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_ByValue);
    g_hGetWeaponSlot = EndPrepSDKCall();

    if (g_hGetWeaponSlot == INVALID_HANDLE)
        LogError("[hzs_zombieskill] Failed to prepare SDKCall for GetWeaponSlot");

    delete gc;
}

//========================================================================================
// 让 Bot 移动到指定坐标（SDKCall: CCSBot_Hide）
//========================================================================================

void PrepHideSDKCall()
{
    Handle gc = LoadGameConfigFile(GAMEDATA_CUSTOM);
    if (gc == INVALID_HANDLE)
    {
        LogError("[hzs_zombieskill] Failed to load gamedata");
        return;
    }

    StartPrepSDKCall(SDKCall_Entity);
    if (!PrepSDKCall_SetFromConf(gc, SDKConf_Signature, "CCSBot_Hide"))
    {
        LogError("[hzs_zombieskill] Failed to find signature for CCSBot_Hide");
        delete gc;
        return;
    }
    PrepSDKCall_AddParameter(SDKType_Vector, SDKPass_ByRef);
    PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_ByValue);
    PrepSDKCall_AddParameter(SDKType_Bool, SDKPass_ByValue);
    g_hHideCall = EndPrepSDKCall();

    if (g_hHideCall == INVALID_HANDLE)
        LogError("[hzs_zombieskill] Failed to prepare SDKCall for CCSBot_Hide");

    delete gc;
}
