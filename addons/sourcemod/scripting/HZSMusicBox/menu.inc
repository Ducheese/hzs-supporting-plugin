public Action Cmd_MusicBox(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    OpenMusicBoxMenu(client);
    return Plugin_Handled;
}

public Action Cmd_MusicBoxStart(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (g_iTrackCount <= 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 播放列表为空。");
        return Plugin_Handled;
    }

    if (g_bClientMusicStarted[client] && g_iClientTrack[client] >= 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 音乐已在播放。");
        return Plugin_Handled;
    }

    g_bClientMusicStarted[client] = true;
    StartTrackForClient(client, PickNextTrack(client), true);
    return Plugin_Handled;
}

public Action Cmd_MusicBoxStop(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (!g_bClientMusicStarted[client] || g_iClientTrack[client] < 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 当前没有正在播放的曲目。");
        return Plugin_Handled;
    }

    StopTrackForClient(client);
    g_bClientMusicStarted[client] = false;
    PrintToChat(client, "\x04[音乐盒]\x01 已停止播放。");
    return Plugin_Handled;
}

public Action Cmd_MusicBoxToggle(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (g_bClientMusicStarted[client] && g_iClientTrack[client] >= 0)
        return Cmd_MusicBoxStop(client, args);
    else
        return Cmd_MusicBoxStart(client, args);
}

public Action Cmd_MusicBoxNext(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (g_iTrackCount <= 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 播放列表为空。");
        return Plugin_Handled;
    }

    if (!g_bClientMusicStarted[client] || g_iClientTrack[client] < 0)
    {
        g_bClientMusicStarted[client] = true;
    }

    StartTrackForClient(client, PickNextTrack(client), true);
    return Plugin_Handled;
}

public Action Cmd_MusicBoxPrev(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (g_iTrackCount <= 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 播放列表为空。");
        return Plugin_Handled;
    }

    if (!g_bClientMusicStarted[client] || g_iClientTrack[client] < 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 当前没有正在播放的曲目。");
        return Plugin_Handled;
    }

    int prev = PickPrevTrack(client);
    if (prev < 0)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 已经是播放历史中的第一首。");
        return Plugin_Handled;
    }

    StartTrackForClient(client, prev, true);
    return Plugin_Handled;
}

public Action Cmd_MusicBoxVolume(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (args == 1)
    {
        char value[8];
        GetCmdArg(1, value, sizeof(value));
        SetClientMusicVolume(client, StringToInt(value));
    }
    else
    {
        OpenVolumeMenu(client);
    }

    return Plugin_Handled;
}

void OpenMusicBoxMenu(int client)
{
    Menu menu = new Menu(MenuHandler_MusicBox);

    char title[256];
    bool isPlaying = (g_bClientMusicStarted[client] && g_iClientTrack[client] >= 0);
    if (isPlaying)
    {
        int track = g_iClientTrack[client];
        float elapsed = GetGameTime() - g_fClientTrackStartTime[client];
        if (elapsed < 0.0)
            elapsed = 0.0;
        Format(title, sizeof(title), "音乐盒\n%s - %s\n进度 %.0f / %.0f 秒\n音量 %d%%",
            g_sTrackArtist[track], g_sTrackTitle[track], elapsed,
            g_fTrackDuration[track], g_iClientVolume[client]);
    }
    else
    {
        Format(title, sizeof(title), "音乐盒\n尚未开始播放\n音量 %d%%", g_iClientVolume[client]);
    }

    menu.SetTitle(title);
    menu.AddItem("prev", "上一首", isPlaying ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
    menu.AddItem("next", "下一首", isPlaying ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
    if (isPlaying)
        menu.AddItem("stop", "停止播放");
    else
        menu.AddItem("start", "开始播放");
    menu.AddItem("volume", "调整音量");
    menu.ExitBackButton = true;
    menu.Display(client, 20);
}

public int MenuHandler_MusicBox(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[16];
        menu.GetItem(param2, info, sizeof(info));

        if (StrEqual(info, "volume"))
        {
            OpenVolumeMenu(client);
        }
        else if (StrEqual(info, "next"))
        {
            Cmd_MusicBoxNext(client, 0);
            OpenMusicBoxMenu(client);
        }
        else if (StrEqual(info, "prev"))
        {
            Cmd_MusicBoxPrev(client, 0);
            OpenMusicBoxMenu(client);
        }
        else if (StrEqual(info, "stop"))
        {
            Cmd_MusicBoxStop(client, 0);
            OpenMusicBoxMenu(client);
        }
        else if (StrEqual(info, "start"))
        {
            Cmd_MusicBoxStart(client, 0);
            OpenMusicBoxMenu(client);
        }
    }
    else if (action == MenuAction_Cancel && param2 == MenuCancel_ExitBack)
    {
        if (IsValidMusicClient(param1))
            FakeClientCommand(param1, "sm_menu");
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }

    return 0;
}

void OpenVolumeMenu(int client)
{
    Menu menu = new Menu(MenuHandler_MusicVolume);
    menu.SetTitle("音乐盒音量");

    int volumes[] = {0, 25, 50, 75, 100};
    for (int i = 0; i < sizeof(volumes); i++)
    {
        char info[8];
        char display[32];
        IntToString(volumes[i], info, sizeof(info));
        Format(display, sizeof(display), "%d%%%s", volumes[i], volumes[i] == g_iClientVolume[client] ? " [当前]" : "");
        menu.AddItem(info, display);
    }

    menu.ExitBackButton = true;
    menu.Display(client, 20);
}

public int MenuHandler_MusicVolume(Menu menu, MenuAction action, int param1, int param2)
{
    if (action == MenuAction_Select)
    {
        int client = param1;
        char info[8];
        menu.GetItem(param2, info, sizeof(info));
        SetClientMusicVolume(client, StringToInt(info));
        OpenVolumeMenu(client);
    }
    else if (action == MenuAction_Cancel && param2 == MenuCancel_ExitBack)
    {
        OpenMusicBoxMenu(param1);
    }
    else if (action == MenuAction_End)
    {
        delete menu;
    }

    return 0;
}
