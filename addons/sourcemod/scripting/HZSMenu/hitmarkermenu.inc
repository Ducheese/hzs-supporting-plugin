// ============================================================================
// HZSMenu/hitmarkermenu.inc — 命中反馈开关子菜单 + Cookie 持久化
// ============================================================================

void RegisterCookies()
{
    // 注册名保持部署过的原名（改名会导致玩家历史设置丢失）
    g_hCookieHitmarkerOverlay = RegClientCookie("hzs_hitmarker_overlay", "HZS Hitmarker Overlay", CookieAccess_Protected);
    g_hCookieHitmarkerSound = RegClientCookie("hzs_hitmarker_sound", "HZS Hitmarker Sound", CookieAccess_Protected);
}

void LoadClientCookies(int client)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || IsFakeClient(client))
        return;

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
            g_bHitmarkerOverlay[client] = !g_bHitmarkerOverlay[client];
            SetClientCookie(client, g_hCookieHitmarkerOverlay, g_bHitmarkerOverlay[client] ? "1" : "0");
            FakeClientCommand(client, "sm_hitmarker_overlay %d", g_bHitmarkerOverlay[client] ? 1 : 0);
        }
        else if (StrEqual(info, "sound"))
        {
            g_bHitmarkerSound[client] = !g_bHitmarkerSound[client];
            SetClientCookie(client, g_hCookieHitmarkerSound, g_bHitmarkerSound[client] ? "1" : "0");
            FakeClientCommand(client, "sm_hitmarker_sound %d", g_bHitmarkerSound[client] ? 1 : 0);
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
