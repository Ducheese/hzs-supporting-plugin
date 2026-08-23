#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <clientprefs>

#include "HZSMenu/globals.inc"
#include "HZSMenu/mainmenu.inc"
#include "HZSMenu/sellmenu.inc"
#include "HZSMenu/gunshotmenu.inc"
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
    RegConsoleCmd("sm_guanyu", Cmd_Guanyu, "关羽之歌");
    RegConsoleCmd("sm_muteguns", Cmd_MuteGuns, "切换他人枪声消去 (0/1)");

    RegisterHitmarkerCookies();
    RegisterGunshotCookies();

    AddTempEntHook("Shotgun Shot", Hook_TEFireBullets);

    // 每 30s 提示打开快捷菜单
    CreateTimer(30.0, Timer_MenuReminder, _, TIMER_REPEAT);
}

public void OnPluginEnd()
{
    RemoveTempEntHook("Shotgun Shot", Hook_TEFireBullets);
}

public void OnMapStart()
{
    PrecacheSound(GUANYU_SOUND, true);

    for (int i = 1; i <= MaxClients; i++)
    {
        g_fGuanyuCooldown[i] = -9999.0;
    }
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
    ResetClientHitmarker(client);
    ResetClientGunshot(client);

    if (AreClientCookiesCached(client))
    {
        OnClientCookiesCached(client);
    }
}

public void OnClientDisconnect(int client)
{
    ResetClientHitmarker(client);
    ResetClientGunshot(client);
}

public void OnClientCookiesCached(int client)
{
    if (IsClientInGame(client))
    {
        LoadClientHitmarkerCookies(client);
        LoadClientGunshotCookies(client);
    }
}

public Action Cmd_Guanyu(int client, int args)
{
    // client 0 = 控制台：无冷却，直接播放
    if (client >= 1 && !IsClientInGame(client))
        return Plugin_Handled;

    if (client >= 1)
    {
        float now = GetGameTime();
        if (now - g_fGuanyuCooldown[client] < GUANYU_COOLDOWN)
        {
            PrintToChat(client, "\x04[关羽之歌] \x07FFFFFF你已经释怀过了，再等等吧");
            return Plugin_Handled;
        }
        g_fGuanyuCooldown[client] = now;
    }

    EmitSoundToAll(GUANYU_SOUND);

    char name[MAX_NAME_LENGTH];
    if (client >= 1)
        GetClientName(client, name, sizeof(name));
    else
        strcopy(name, sizeof(name), "管理员");
    PrintToChatAll("\x04[关羽之歌] \x03%s \x07FFFFFF释怀了", name);

    return Plugin_Handled;
}
