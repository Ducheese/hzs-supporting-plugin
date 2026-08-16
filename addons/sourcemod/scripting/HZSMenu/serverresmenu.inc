// ============================================================================
// HZSMenu/serverresmenu.inc — 资源兼容子菜单（选择后 PrintToChat 说明）
// ============================================================================

void OpenServerResMenu(int client)
{
    Menu menu = new Menu(MenuHandler_ServerResMenu);
    menu.SetTitle("资源兼容");

    menu.AddItem("particle", "粒子特效的共存性问题");
    menu.AddItem("gloves", "两阵营不同手套插件");

    menu.ExitBackButton = true;
    menu.Display(client, MENU_TIMEOUT);
}

public int MenuHandler_ServerResMenu(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[32];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "particle"))
        {
            // 文本超 255 字节会被引擎拒绝发送，拆成两条（每条 ≤255 字节）
            PrintToChat(client, "\x04[快捷菜单]\x01 飙血和僵尸技能都使用了新加的粒子库，如果服务器下载下来的particles_manifest，和你自己使用的冲突了(比如MW枪火粒子)");
            PrintToChat(client, "\x04[快捷菜单]\x01 就把服务器particles_manifest新加的两行粒子复制到自己的里面，且只保留自己的那份particles_manifest，这样才能共存");
        }
        else if (StrEqual(info, "gloves"))
        {
            PrintToChat(client, "\x04[快捷菜单]\x01 本服务器支持 \x03 岭南都指挥使 \x01的两阵营不同手套插件，你可以在武器qc文件内配置两套skinfamilies，T阵营会自动使用第二套");
        }

        OpenServerResMenu(client);
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
