// ============================================================================
// HZSMenu/gunshotmenu.inc — 他人枪声消去（Shotgun Shot TempEntity 过滤）+ Cookie 持久化
// ============================================================================

void RegisterGunshotCookies()
{
    g_hCookieMuteGunshots = RegClientCookie("hzs_mute_gunshots", "HZS Mute Others Gunshots", CookieAccess_Protected);
}

void ResetClientGunshot(int client)
{
    if (client >= 1 && client <= MaxClients)
    {
        g_bMuteGunshots[client] = false;
        g_bGunshotManual[client] = false;
    }
}

void LoadClientGunshotCookies(int client)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || IsFakeClient(client))
        return;

    // 若玩家在 Cookie 载入完成前已手动修改，将新设置写回 Cookie 保存
    if (g_bGunshotManual[client])
    {
        SetClientCookie(client, g_hCookieMuteGunshots, g_bMuteGunshots[client] ? "1" : "0");
        return;
    }

    char sVal[8];
    GetClientCookie(client, g_hCookieMuteGunshots, sVal, sizeof(sVal));
    if (sVal[0] != '\0')
    {
        g_bMuteGunshots[client] = (StringToInt(sVal) != 0);
    }
    else
    {
        g_bMuteGunshots[client] = false;
    }
}

void SetClientGunshotMute(int client, bool mute)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
        return;

    g_bMuteGunshots[client] = mute;

    if (AreClientCookiesCached(client))
    {
        SetClientCookie(client, g_hCookieMuteGunshots, mute ? "1" : "0");
    }
    else
    {
        g_bGunshotManual[client] = true;
    }

    PrintToChat(client, "\x04[快捷菜单]\x01 他人枪声已\x03%s\x01。", mute ? "静音" : "开启");
}

public Action Cmd_MuteGuns(int client, int args)
{
    if (client < 1 || !IsClientInGame(client))
        return Plugin_Handled;

    if (args == 1)
    {
        char arg[8];
        GetCmdArg(1, arg, sizeof(arg));
        SetClientGunshotMute(client, StringToInt(arg) != 0);
    }
    else
    {
        OpenGunshotMenu(client);
    }

    return Plugin_Handled;
}

void OpenGunshotMenu(int client)
{
    Menu menu = new Menu(MenuHandler_GunshotMenu);
    menu.SetTitle("他人枪声消去");

    char itemStatus[64];
    Format(itemStatus, sizeof(itemStatus), "他人枪声 [%s]", g_bMuteGunshots[client] ? "已静音" : "正常");
    menu.AddItem("toggle", itemStatus);

    menu.ExitBackButton = true;
    menu.Display(client, MENU_TIMEOUT);
}

public int MenuHandler_GunshotMenu(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[16];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "toggle"))
        {
            SetClientGunshotMute(client, !g_bMuteGunshots[client]);
        }

        OpenGunshotMenu(client);
    }
    else if (action == MenuAction_Cancel && param2 == MenuCancel_ExitBack)
    {
        OpenMainMenu(param1);
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }
    return 0;
}

// ============================================================================
// >> TempEntity 拦截：过滤 "Shotgun Shot"（CS:S 所有武器开火网络广播）
// ============================================================================

public Action Hook_TEFireBullets(const char[] te_name, const int[] Players, int numClients, float delay)
{
    int clients[MAXPLAYERS];
    int count = 0;

    for (int i = 0; i < numClients; i++)
    {
        int client = Players[i];
        if (client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsFakeClient(client))
        {
            if (!g_bMuteGunshots[client])
            {
                clients[count++] = client;
            }
        }
        else
        {
            clients[count++] = client;
        }
    }

    // 所有玩家都保持正常接收
    if (count == numClients)
        return Plugin_Continue;

    // 所有接收者都开启了静音，直接阻断
    if (count == 0)
        return Plugin_Stop;

    // 重新打包转发给未静音的玩家
    float vOrigin[3];
    TE_ReadVector("m_vecOrigin", vOrigin);
    float fAngle0 = TE_ReadFloat("m_vecAngles[0]");
    float fAngle1 = TE_ReadFloat("m_vecAngles[1]");
    int iWeaponID = TE_ReadNum("m_iWeaponID");
    int iMode = TE_ReadNum("m_iMode");
    int iSeed = TE_ReadNum("m_iSeed");
    int iPlayer = TE_ReadNum("m_iPlayer");
    float fInaccuracy = TE_ReadFloat("m_fInaccuracy");
    float fSpread = TE_ReadFloat("m_fSpread");

    TE_Start("Shotgun Shot");
    TE_WriteVector("m_vecOrigin", vOrigin);
    TE_WriteFloat("m_vecAngles[0]", fAngle0);
    TE_WriteFloat("m_vecAngles[1]", fAngle1);
    TE_WriteNum("m_iWeaponID", iWeaponID);
    TE_WriteNum("m_iMode", iMode);
    TE_WriteNum("m_iSeed", iSeed);
    TE_WriteNum("m_iPlayer", iPlayer);
    TE_WriteFloat("m_fInaccuracy", fInaccuracy);
    TE_WriteFloat("m_fSpread", fSpread);
    TE_Send(clients, count, delay);

    return Plugin_Stop;
}
