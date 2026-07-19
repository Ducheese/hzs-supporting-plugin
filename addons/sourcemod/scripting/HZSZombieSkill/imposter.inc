//========================================================================================
// 伪人僵尸技能：随机选取一个玩家，模仿其模型和动作
//========================================================================================

public Action Timer_ImposterSyncThink(Handle timer, int zombie)
{
    if (!IsValidEntity(zombie) || GetEntProp(zombie, Prop_Data, "m_iHealth") <= 0)
    {
        g_hImposterSync[zombie] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    int target = g_iImposterTarget[zombie];

    // target 死了 → 初始化数组记录
    if (target > 0 && !IsHumanAlive(target))
    {
        g_iImposterTarget[zombie] = -1;
        target = -1;
    }

    // 没 target → 选人或显真身
    if (target == -1)
    {
        // 杀旧假身（不论有没有人模仿都要清）
        KillFakeBody(zombie);

        // 随机选目标
        int human = PickRandomHuman();
        if (human != -1)
        {
            g_iImposterTarget[zombie] = human;

            // 藏真身
            /**
             * 会重复执行，不过没有影响
             */
            int animEnt = FindAnimEntity(zombie);
            if (animEnt != -1)
            {
                SetEntityRenderMode(animEnt, RENDER_TRANSCOLOR);
                SetEntityRenderColor(animEnt, 255, 255, 255, 0);
            }

            // 建假身
            int newFake = CreateFakeBody(zombie, human);
            if (newFake != -1)
            {
                g_iFakeBody[zombie] = newFake;
                SetEntityRenderColor(newFake, 255, 255, 255, 255);
            }
        }
        else
        {
            // 没人 → 显真身
            int animEnt = FindAnimEntity(zombie);
            if (animEnt != -1)
            {
                SetEntityRenderMode(animEnt, RENDER_NORMAL);
                SetEntityRenderColor(animEnt, 255, 255, 255, 255);
            }
        }
        return Plugin_Continue;
    }

    // 有 target → 同步动作
    int fake = g_iFakeBody[zombie];
    if (fake > 0 && IsValidEntity(fake))
    {
        SyncFakeBodyAnimation(zombie, fake, target);
    }

    return Plugin_Continue;
}

// 杀假身（ClearParent + Kill）
void KillFakeBody(int zombie)
{
    int fake = g_iFakeBody[zombie];
    if (fake > 0 && IsValidEntity(fake))
    {
        AcceptEntityInput(fake, "ClearParent");
        AcceptEntityInput(fake, "Kill");
        g_iFakeBody[zombie] = -1;
    }
}

// 同步假身动画和朝向
void SyncFakeBodyAnimation(int zombie, int fake, int target)
{
    // 人停→伪人停，人动→恢复
    if (IsHumanMoving(target))
    {
        if (g_bImposterPaused[zombie])
        {
            g_bImposterPaused[zombie] = false;
            Han_UnlockZombie(zombie);
        }
    }
    else
    {
        if (!g_bImposterPaused[zombie])
        {
            g_bImposterPaused[zombie] = true;
            Han_SetZombieTarget(zombie, zombie, 0.0);
        }
    }

    // 朝向
    float ang[3];
    GetEntPropVector(target, Prop_Send, "m_angRotation", ang);
    TeleportEntity(fake, NULL_VECTOR, ang, NULL_VECTOR);

    // 动画
    SetEntProp(fake, Prop_Send, "m_nSequence", GetEntProp(target, Prop_Send, "m_nSequence"));
    SetEntPropFloat(fake, Prop_Send, "m_flCycle", GetEntPropFloat(target, Prop_Send, "m_flCycle"));
    SetEntProp(fake, Prop_Send, "m_flAnimTime", GetEntProp(target, Prop_Send, "m_flAnimTime"));

    for (int i = 0; i < 24; i++)
    {
        SetEntPropFloat(fake, Prop_Send, "m_flPoseParameter", GetEntPropFloat(target, Prop_Send, "m_flPoseParameter", i), i);
    }
}

int CreateFakeBody(int zombie, int human)
{
    char model[PLATFORM_MAX_PATH];
    GetEntPropString(human, Prop_Data, "m_ModelName", model, sizeof(model));

    int fake = CreateEntityByName("prop_dynamic");
    if (fake == -1)
        return -1;

    DispatchKeyValue(fake, "model", model);
    DispatchSpawn(fake);

    // 禁用客户端自主动画——改为服务端全量同步驱动
    SetEntProp(fake, Prop_Data, "m_bClientSideAnimation", 0);

    SetVariantString("!activator");
    AcceptEntityInput(fake, "SetParent", zombie);
    TeleportEntity(fake, view_as<float>({0.0, 0.0, 0.0}), view_as<float>({0.0, 0.0, 0.0}), NULL_VECTOR);

    SetEntityRenderMode(fake, RENDER_TRANSCOLOR);
    SetEntityRenderColor(fake, 255, 255, 255, 0);

    /**
     * 女巫致盲后，伪人僵尸的模型也会隐藏
     */
    SDKHook(fake, SDKHook_SetTransmit, OnWitchBlindSetTransmit);

    return fake;
}

void EndImposter(int zombie)
{
    if (g_hImposterSync[zombie] != INVALID_HANDLE)
    {
        KillTimer(g_hImposterSync[zombie]);
        g_hImposterSync[zombie] = INVALID_HANDLE;
    }

    int animEnt = FindAnimEntity(zombie);
    if (animEnt != -1)
    {
        SetEntityRenderMode(animEnt, RENDER_NORMAL);
        SetEntityRenderColor(animEnt, 255, 255, 255, 255);
    }

    KillFakeBody(zombie);
}
