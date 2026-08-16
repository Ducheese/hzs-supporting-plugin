// ============================================================================
// HZSMenu/sellmenu.inc — Sell半价回收子菜单
// ============================================================================

void OpenSellMenu(int client)
{
    Menu menu = new Menu(MenuHandler_SellMenu);
    menu.SetTitle("Sell半价回收");

    menu.AddItem("all", "卖出全部");
    menu.AddItem("pri", "卖出主武器");
    menu.AddItem("sec", "卖出副武器");
    menu.AddItem("gre", "卖出投掷物");

    menu.ExitBackButton = true;
    menu.Display(client, MENU_TIMEOUT);
}

public int MenuHandler_SellMenu(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[16];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "all"))
        {
            FakeClientCommand(client, "sm_sell all");
        }
        else if (StrEqual(info, "pri"))
        {
            FakeClientCommand(client, "sm_sell pri");
        }
        else if (StrEqual(info, "sec"))
        {
            FakeClientCommand(client, "sm_sell sec");
        }
        else if (StrEqual(info, "gre"))
        {
            FakeClientCommand(client, "sm_sell gre");
        }
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
