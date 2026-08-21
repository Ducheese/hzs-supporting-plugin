void CreateMusicSource()
{
    if (g_iMusicSource != -1 && IsValidEntity(g_iMusicSource))
        return;

    g_iMusicSource = CreateEntityByName("info_target");
    if (g_iMusicSource == -1)
    {
        LogError("Failed to create the HZS music source entity; falling back to each client as source");
        return;
    }

    DispatchSpawn(g_iMusicSource);
}

void DestroyMusicSource()
{
    if (g_iMusicSource != -1 && IsValidEntity(g_iMusicSource))
        AcceptEntityInput(g_iMusicSource, "Kill");

    g_iMusicSource = -1;
}

int GetMusicSource(int client)
{
    if (g_iMusicSource != -1 && IsValidEntity(g_iMusicSource))
        return g_iMusicSource;

    return client;
}

void StartTrackForClient(int client, int track, bool announce)
{
    if (!IsValidMusicClient(client) || track < 0 || track >= g_iTrackCount)
        return;

    CreateMusicSource();
    StopTrackForClient(client);

    g_iClientTrack[client] = track;
    g_fClientTrackStartTime[client] = GetGameTime();
    g_hClientNextTrackTimer[client] = CreateTimer(g_fTrackDuration[track], Timer_NextTrack,
        GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);

    // 音量 0 用 0.02 保活（0.0 会被客户端丢弃，后续 CHANGE_VOL 找不到而重建重播）
    float vol = float(g_iClientVolume[client]) / 100.0;
    if (vol == 0.0) vol = 0.02;
    EmitSoundToClient(client, g_sTrackPath[track], GetMusicSource(client), SNDCHAN_STATIC, SNDLEVEL_NONE,
        SND_NOFLAGS, vol, SNDPITCH_NORMAL);
    g_bClientMusicPlaying[client] = true;

    if (announce)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 正在播放：\x03%s - %s", g_sTrackArtist[track], g_sTrackTitle[track]);
    }
}

public Action Timer_NextTrack(Handle timer, int userid)
{
    int client = GetClientOfUserId(userid);
    if (client <= 0 || !IsValidMusicClient(client))
        return Plugin_Stop;

    g_hClientNextTrackTimer[client] = INVALID_HANDLE;

    if (!g_bClientMusicStarted[client] || g_iClientTrack[client] < 0)
        return Plugin_Stop;

    StartTrackForClient(client, PickNextTrack(client), true);
    return Plugin_Stop;
}

void StopAllMusic()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        StopTrackForClient(client);
        g_bClientMusicStarted[client] = false;
    }
}

void ResetMapTimerHandles()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        g_hClientNextTrackTimer[client] = INVALID_HANDLE;
    }
}

void StopClientNextTrackTimer(int client)
{
    if (client < 1 || client > MaxClients)
        return;

    if (g_hClientNextTrackTimer[client] != INVALID_HANDLE)
    {
        KillTimer(g_hClientNextTrackTimer[client]);
        g_hClientNextTrackTimer[client] = INVALID_HANDLE;
    }
}

void StopTrackForClient(int client)
{
    if (client < 1 || client > MaxClients)
        return;

    StopClientNextTrackTimer(client);
    StopClientSound(client);

    g_iClientTrack[client] = -1;
    g_fClientTrackStartTime[client] = 0.0;
    g_bClientMusicPlaying[client] = false;
}

void StopClientSound(int client)
{
    if (client < 1 || client > MaxClients)
        return;

    int track = g_iClientTrack[client];
    if (track >= 0 && track < g_iTrackCount && IsValidMusicClient(client) && g_bClientMusicPlaying[client])
    {
        EmitSoundToClient(client, g_sTrackPath[track], GetMusicSource(client), SNDCHAN_STATIC, SNDLEVEL_NONE,
            MUSICBOX_SOUND_STOP | MUSICBOX_SOUND_IGNORE_NAME, 0.0, SNDPITCH_NORMAL);
    }

    g_bClientMusicPlaying[client] = false;
}

void SetClientMusicVolume(int client, int volume)
{
    if (!IsValidMusicClient(client))
        return;

    if (!IsAllowedVolume(volume))
    {
        PrintToChat(client, "\x04[音乐盒]\x01 音量只能设置为 0、25、50、75 或 100。");
        return;
    }

    int oldVolume = g_iClientVolume[client];
    g_iClientVolume[client] = volume;
    g_bClientVolumeManual[client] = true;

    if (g_bClientCookiesReady[client])
    {
        char value[8];
        IntToString(volume, value, sizeof(value));
        SetClientCookie(client, g_hCookieVolume, value);
    }

    if (g_bClientMusicStarted[client] && g_iClientTrack[client] >= 0 && oldVolume != volume)
    {
        float newVol = float(volume) / 100.0;
        if (newVol == 0.0) newVol = 0.02;
        EmitSoundToClient(client, g_sTrackPath[g_iClientTrack[client]], GetMusicSource(client), SNDCHAN_STATIC,
            SNDLEVEL_NONE, MUSICBOX_SOUND_CHANGE_VOLUME, newVol, SNDPITCH_NORMAL);
        g_bClientMusicPlaying[client] = true;
    }

    PrintToChat(client, "\x04[音乐盒]\x01 音量已设置为 \x03%d%%\x01。", volume);
}

bool IsAllowedVolume(int volume)
{
    return volume == 0 || volume == 25 || volume == 50 || volume == 75 || volume == 100;
}

bool IsValidMusicClient(int client)
{
    return client >= 1 && client <= MaxClients && IsClientInGame(client) && !IsFakeClient(client);
}

public Action Hook_NormalSound(int clients[MAXPLAYERS], int &numClients, char sample[PLATFORM_MAX_PATH],
    int &entity, int &channel, float &volume, int &level, int &pitch, int &flags,
    char soundEntry[PLATFORM_MAX_PATH], int &seed)
{
    // 如果是音乐盒自己的发声实体，直接放行
    if (entity == g_iMusicSource && g_iMusicSource != -1)
        return Plugin_Continue;

    // 拦截 sound/custom/stagevox 目录下的所有音频（对开启音乐盒且音量>0的玩家屏蔽）
    if (StrContains(sample, "stagevox", false) != -1)
    {
        int newTotal = 0;
        for (int i = 0; i < numClients; i++)
        {
            int client = clients[i];
            // 如果玩家有效且未开启音乐盒（或音量为0），依然保留接收原版 stagevox
            if (IsValidMusicClient(client))
            {
                if (!g_bClientMusicStarted[client] || g_iClientVolume[client] == 0)
                {
                    clients[newTotal++] = client;
                }
            }
            else
            {
                clients[newTotal++] = client;
            }
        }

        numClients = newTotal;
        return (numClients > 0) ? Plugin_Changed : Plugin_Stop;
    }

    return Plugin_Continue;
}
