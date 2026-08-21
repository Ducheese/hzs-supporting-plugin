// ============================================================================
// HZSMenu/hitmarkermenu.inc — 命中反馈开关子菜单 + Cookie 持久化
// ============================================================================

void RegisterHitmarkerCookies()
{
    // 注册名保持部署过的原名（改名会导致玩家历史设置丢失）
    g_hCookieHitmarkerOverlay = RegClientCookie("hzs_hitmarker_overlay", "HZS Hitmarker Overlay", CookieAccess_Protected);
    g_hCookieHitmarkerSound = RegClientCookie("hzs_hitmarker_sound", "HZS Hitmarker Sound", CookieAccess_Protected);
}

void ResetClientHitmarker(int client)
{
    if (client >= 1 && client <= MaxClients)
    {
        g_bHitmarkerOverlay[client] = true;
        g_bHitmarkerSound[client] = true;
        g_bHitmarkerManual[client] = false;
    }
}

void SetClientHitmarkerOverlay(int client, bool enable)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
        return;

    g_bHitmarkerOverlay[client] = enable;

    if (AreClientCookiesCached(client))
    {
        SetClientCookie(client, g_hCookieHitmarkerOverlay, enable ? "1" : "0");
    }
    else
    {
        g_bHitmarkerManual[client] = true;
    }

    SyncHitmarkerSettings(client);
}

void SetClientHitmarkerSound(int client, bool enable)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
        return;

    g_bHitmarkerSound[client] = enable;

    if (AreClientCookiesCached(client))
    {
        SetClientCookie(client, g_hCookieHitmarkerSound, enable ? "1" : "0");
    }
    else
    {
        g_bHitmarkerManual[client] = true;
    }

    SyncHitmarkerSettings(client);
}

void LoadClientHitmarkerCookies(int client)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || IsFakeClient(client))
        return;

    // 若玩家在 Cookie 载入完成前已手动修改，将新设置写回 Cookie 保存
    if (g_bHitmarkerManual[client])
    {
        SetClientCookie(client, g_hCookieHitmarkerOverlay, g_bHitmarkerOverlay[client] ? "1" : "0");
        SetClientCookie(client, g_hCookieHitmarkerSound, g_bHitmarkerSound[client] ? "1" : "0");
        return;
    }

    // 命中贴图开关
    char sOverlay[8];
    GetClientCookie(client, g_hCookieHitmarkerOverlay, sOverlay, sizeof(sOverlay));
    if (sOverlay[0] != '\0')
    {
        g_bHitmarkerOverlay[client] = (StringToInt(sOverlay) != 0);
    }
    else
    {
        g_bHitmarkerOverlay[client] = true;
    }

    // 命中音效开关
    char sSound[8];
    GetClientCookie(client, g_hCookieHitmarkerSound, sSound, sizeof(sSound));
    if (sSound[0] != '\0')
    {
        g_bHitmarkerSound[client] = (StringToInt(sSound) != 0);
    }
    else
    {
        g_bHitmarkerSound[client] = true;
    }

    // 同步设置给命中反馈插件
    SyncHitmarkerSettings(client);
}

void SyncHitmarkerSettings(int client)
{
    if (client > 0 && IsClientInGame(client) && !IsFakeClient(client))
    {
        FakeClientCommand(client, "sm_hitmarker_overlay %d", g_bHitmarkerOverlay[client] ? 1 : 0);
        FakeClientCommand(client, "sm_hitmarker_sound %d", g_bHitmarkerSound[client] ? 1 : 0);
    }
}

void OpenHitmarkerMenu(int client)
{
    Menu menu = new Menu(MenuHandler_HitmarkerMenu);
    menu.SetTitle("命中反馈开关");

    char itemOverlay[64];
    char itemSound[64];
    Format(itemOverlay, sizeof(itemOverlay), "命中贴图 [%s]", g_bHitmarkerOverlay[client] ? "开" : "关");
    Format(itemSound, sizeof(itemSound), "命中音效 [%s]", g_bHitmarkerSound[client] ? "开" : "关");

    menu.AddItem("overlay", itemOverlay);
    menu.AddItem("sound", itemSound);

    menu.ExitBackButton = true;
    menu.Display(client, MENU_TIMEOUT);
}

public int MenuHandler_HitmarkerMenu(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[16];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "overlay"))
        {
            SetClientHitmarkerOverlay(client, !g_bHitmarkerOverlay[client]);
        }
        else if (StrEqual(info, "sound"))
        {
            SetClientHitmarkerSound(client, !g_bHitmarkerSound[client]);
        }

        OpenHitmarkerMenu(client);
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
