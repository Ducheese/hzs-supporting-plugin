// ============================================================================
// HZSMenu/extragunsmenu.inc — 武器多持子菜单（Cookie 持久化 + 四槽位武器信息显示与实时同步）
// ============================================================================

void RegisterExtraGunsCookies()
{
    g_hCookieExtraGuns = RegClientCookie("hzs_extraguns_enable", "HZS Extra Guns Enable Cookie", CookieAccess_Protected);
}

void ResetClientExtraGuns(int client)
{
    if (client >= 1 && client <= MaxClients)
    {
        g_bExtraGuns[client] = false;    // 默认关
        g_bExtraGunsManual[client] = false;
        g_bInExtraGunsMenu[client] = false;
    }
}

void LoadClientExtraGunsCookies(int client)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client) || IsFakeClient(client))
        return;

    if (g_bExtraGunsManual[client])
    {
        SetClientCookie(client, g_hCookieExtraGuns, g_bExtraGuns[client] ? "1" : "0");
        return;
    }

    char sVal[8];
    GetClientCookie(client, g_hCookieExtraGuns, sVal, sizeof(sVal));
    if (sVal[0] != '\0')
    {
        g_bExtraGuns[client] = (StringToInt(sVal) != 0);
    }
    else
    {
        g_bExtraGuns[client] = false;    // 默认关
    }

    if (g_bExtraGuns[client])
    {
        FakeClientCommand(client, "sm_extraguns 1");
    }
}

static void GetCleanWeaponName(int weapon, char[] buffer, int maxlen)
{
    if (weapon <= 0 || !IsValidEntity(weapon))
    {
        strcopy(buffer, maxlen, "无");
        return;
    }

    char cls[64];
    GetEdictClassname(weapon, cls, sizeof(cls));

    if (strncmp(cls, "weapon_", 7) == 0)
    {
        strcopy(buffer, maxlen, cls[7]);
    }
    else
    {
        strcopy(buffer, maxlen, cls);
    }
}

void OpenExtraGunsMenu(int client)
{
    if (client < 1 || client > MaxClients || !IsClientInGame(client))
        return;

    g_bInExtraGunsMenu[client] = true;

    Menu menu = new Menu(MenuHandler_ExtraGunsMenu);
    menu.SetTitle("武器多持");

    // 获取四槽武器实体
    char namePri[32], nameExtraPri[32], nameSec[32], nameExtraSec[32];
    GetCleanWeaponName(ExtraGuns_GetWeapon(client, EG_Slot_Primary), namePri, sizeof(namePri));
    GetCleanWeaponName(ExtraGuns_GetWeapon(client, EG_Slot_ExtraPrimary), nameExtraPri, sizeof(nameExtraPri));
    GetCleanWeaponName(ExtraGuns_GetWeapon(client, EG_Slot_Secondary), nameSec, sizeof(nameSec));
    GetCleanWeaponName(ExtraGuns_GetWeapon(client, EG_Slot_ExtraSecondary), nameExtraSec, sizeof(nameExtraSec));

    // 槽位严格按序：主武器(1) -> 扩展主(2) -> 副武器(3) -> 扩展副(4)
    char itemPri[64], itemExtraPri[64], itemSec[64], itemExtraSec[64];
    Format(itemPri, sizeof(itemPri), "主武器: %s", namePri);
    Format(itemExtraPri, sizeof(itemExtraPri), "扩展主: %s", nameExtraPri);
    Format(itemSec, sizeof(itemSec), "副武器: %s", nameSec);
    Format(itemExtraSec, sizeof(itemExtraSec), "扩展副: %s", nameExtraSec);

    menu.AddItem("pri", itemPri);           // 对应按键 1
    menu.AddItem("extrapri", itemExtraPri); // 对应按键 2
    menu.AddItem("sec", itemSec);           // 对应按键 3
    menu.AddItem("extrasec", itemExtraSec); // 对应按键 4
    menu.AddItem("", "", ITEMDRAW_SPACER);  // 对应按键 5
    menu.AddItem("", "", ITEMDRAW_SPACER);  // 对应按键 6

    // 开关移至第 7 项，彻底远离数字 1/2/3/4
    char itemToggle[64];
    Format(itemToggle, sizeof(itemToggle), "多持开关 [%s]", g_bExtraGuns[client] ? "开启" : "关闭");
    menu.AddItem("toggle", itemToggle);     // 对应按键 7

    menu.ExitBackButton = true;
    menu.Display(client, MENU_TIMEOUT);
}

public int MenuHandler_ExtraGunsMenu(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[16];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "pri"))
        {
            // 按数字 1：切到当前主手主武器
            int pri = ExtraGuns_GetWeapon(client, EG_Slot_Primary);
            if (pri > 0 && IsValidEntity(pri))
            {
                char cls[64];
                GetEdictClassname(pri, cls, sizeof(cls));
                FakeClientCommand(client, "use %s", cls);
            }
            OpenExtraGunsMenu(client);
        }
        else if (StrEqual(info, "extrapri"))
        {
            // 按数字 2：把扩展主变成主武器，并切到该武器
            int extraPri = ExtraGuns_GetWeapon(client, EG_Slot_ExtraPrimary);
            if (extraPri > 0)
            {
                ExtraGuns_SwapPrimary(client);
            }
            OpenExtraGunsMenu(client);
        }
        else if (StrEqual(info, "sec"))
        {
            // 按数字 3：切到当前主手副武器
            int sec = ExtraGuns_GetWeapon(client, EG_Slot_Secondary);
            if (sec > 0 && IsValidEntity(sec))
            {
                char cls[64];
                GetEdictClassname(sec, cls, sizeof(cls));
                FakeClientCommand(client, "use %s", cls);
            }
            OpenExtraGunsMenu(client);
        }
        else if (StrEqual(info, "extrasec"))
        {
            // 按数字 4：把扩展副变成副武器，并切到该武器
            int extraSec = ExtraGuns_GetWeapon(client, EG_Slot_ExtraSecondary);
            if (extraSec > 0)
            {
                ExtraGuns_SwapSecondary(client);
            }
            OpenExtraGunsMenu(client);
        }
        else if (StrEqual(info, "toggle"))
        {
            // 按数字 7：切换多持总开关
            g_bExtraGuns[client] = !g_bExtraGuns[client];

            if (AreClientCookiesCached(client))
            {
                SetClientCookie(client, g_hCookieExtraGuns, g_bExtraGuns[client] ? "1" : "0");
            }
            else
            {
                g_bExtraGunsManual[client] = true;
            }

            FakeClientCommand(client, "sm_extraguns %d", g_bExtraGuns[client] ? 1 : 0);
            OpenExtraGunsMenu(client);
        }
    }
    else if (action == MenuAction_Cancel)
    {
        int client = param1;
        g_bInExtraGunsMenu[client] = false;

        if (param2 == MenuCancel_ExitBack)
        {
            OpenMainMenu(client);
        }
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }
    return 0;
}

// ============================================================================
// >> 实时同步：玩家在游戏中切枪或丢枪时，若停留在本菜单则秒级刷新
// ============================================================================
public void ExtraGuns_OnSlotsChanged(int client)
{
    if (client >= 1 && client <= MaxClients && IsClientInGame(client) && g_bInExtraGunsMenu[client])
    {
        OpenExtraGunsMenu(client);
    }
}
