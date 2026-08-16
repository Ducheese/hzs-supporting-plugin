// ============================================================================
// HZSMenu/mainmenu.inc — 主菜单
// ============================================================================

public Action Cmd_Menu(int client, int args)
{
    if (client > 0 && IsClientInGame(client))
    {
        OpenMainMenu(client);
    }
    return Plugin_Handled;
}

void OpenMainMenu(int client)
{
    Menu menu = new Menu(MenuHandler_MainMenu);
    menu.SetTitle("menu");

    menu.AddItem("turret", "HZS机枪炮台");
    menu.AddItem("zipline", "HZS滑索");
    menu.AddItem("cheer", "欢呼");
    menu.AddItem("sell", "Sell半价回收");
    menu.AddItem("hitmarker", "命中反馈开关");
    menu.AddItem("guide_play", "玩法提示");
    menu.AddItem("guide_res", "资源兼容");

    menu.ExitButton = true;
    menu.Display(client, MENU_TIMEOUT);
}

public int MenuHandler_MainMenu(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[32];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "turret"))
        {
            if (!IsPlayerAlive(client))
            {
                PrintToChat(client, "\x04[快捷菜单]\x01 死亡状态无法使用炮台菜单");
                OpenMainMenu(client);   // 拒绝后保持菜单打开
                return 0;
            }
            FakeClientCommand(client, "sm_turret");
        }
        else if (StrEqual(info, "zipline"))
        {
            if (!IsPlayerAlive(client))
            {
                PrintToChat(client, "\x04[快捷菜单]\x01 死亡状态无法使用滑索菜单");
                OpenMainMenu(client);   // 拒绝后保持菜单打开
                return 0;
            }
            FakeClientCommand(client, "sm_zipline");
        }
        else if (StrEqual(info, "cheer"))
        {
            FakeClientCommand(client, "sm_cheer");
            OpenMainMenu(client);
        }
        else if (StrEqual(info, "sell"))
        {
            if (!IsPlayerAlive(client))
            {
                PrintToChat(client, "\x04[快捷菜单]\x01 死亡状态无法使用Sell菜单");
                OpenMainMenu(client);   // 拒绝后保持菜单打开
                return 0;
            }
            OpenSellMenu(client);
        }
        else if (StrEqual(info, "hitmarker"))
        {
            OpenHitmarkerMenu(client);
        }
        else if (StrEqual(info, "guide_play"))
        {
            ShowGuideHUD(client);
            OpenMainMenu(client);
        }
        else if (StrEqual(info, "guide_res"))
        {
            OpenServerResMenu(client);
        }
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }
    return 0;
}
