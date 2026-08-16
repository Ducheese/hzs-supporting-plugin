#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <clientprefs>

#include "HZSMenu/globals.inc"
#include "HZSMenu/mainmenu.inc"
#include "HZSMenu/sellmenu.inc"
#include "HZSMenu/hitmarkermenu.inc"
#include "HZSMenu/guidemenu.inc"
#include "HZSMenu/serverresmenu.inc"

#define VERSION "1.0"

public Plugin myinfo =
{
    name = "HZS Menu",
    author = "Ducheese",
    description = "服务器快捷菜单",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
};

// ============================================================================
// >> 插件生命周期
// ============================================================================
public void OnPluginStart()
{
    RegConsoleCmd("sm_menu", Cmd_Menu, "打开快捷菜单");

    RegisterCookies();

    // 每 30s 提示打开快捷菜单
    CreateTimer(30.0, Timer_MenuReminder, _, TIMER_REPEAT);
}

// 每 30s 全服提示
public Action Timer_MenuReminder(Handle timer)
{
    for (int client = 1; client <= MaxClients; client++)
    {
        if (IsClientInGame(client) && !IsFakeClient(client))
        {
            PrintToChat(client, "\x04[提示]\x01 输入 \x03!menu \x01可以打开快捷菜单");
        }
    }
    return Plugin_Continue;
}

public void OnClientPutInServer(int client)
{
    g_bHitmarkerOverlay[client] = true;
    g_bHitmarkerSound[client] = true;
}

public void OnClientDisconnect(int client)
{
    g_bHitmarkerOverlay[client] = true;
    g_bHitmarkerSound[client] = true;
}

public void OnClientCookiesCached(int client)
{
    LoadClientCookies(client);
}
