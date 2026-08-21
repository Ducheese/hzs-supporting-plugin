void LoadPlaylist()
{
    bool hadTracks = g_iTrackCount > 0;
    g_iTrackCount = 0;

    char configPath[PLATFORM_MAX_PATH];
    BuildPath(Path_SM, configPath, sizeof(configPath), MUSICBOX_CONFIG);

    KeyValues kv = new KeyValues("MusicBox");
    if (!kv.ImportFromFile(configPath))
    {
        LogError("Music box config not found or invalid: %s", configPath);
        delete kv;
        return;
    }

    if (!kv.JumpToKey("Tracks") || !kv.GotoFirstSubKey())
    {
        LogError("Music box config has no valid Tracks section: %s", configPath);
        delete kv;
        return;
    }

    do
    {
        if (g_iTrackCount >= MUSICBOX_MAX_TRACKS)
        {
            LogError("Music box playlist exceeds the %d track limit", MUSICBOX_MAX_TRACKS);
            break;
        }

        char path[PLATFORM_MAX_PATH];
        char title[MUSICBOX_TITLE_LENGTH];
        char artist[MUSICBOX_ARTIST_LENGTH];
        kv.GetString("path", path, sizeof(path));
        kv.GetString("title", title, sizeof(title), "未命名曲目");
        kv.GetString("artist", artist, sizeof(artist), "未知艺术家");

        float duration = kv.GetFloat("duration", 0.0);
        if (path[0] == '\0' || duration <= 0.0)
        {
            LogError("Music box track %d is missing path or positive duration", g_iTrackCount + 1);
            continue;
        }

        strcopy(g_sTrackPath[g_iTrackCount], sizeof(g_sTrackPath[]), path);
        strcopy(g_sTrackTitle[g_iTrackCount], sizeof(g_sTrackTitle[]), title);
        strcopy(g_sTrackArtist[g_iTrackCount], sizeof(g_sTrackArtist[]), artist);
        g_fTrackDuration[g_iTrackCount] = duration;
        g_iTrackCount++;
    }
    while (kv.GotoNextKey());

    delete kv;
    PrintToServer("[HZS Music Box] Loaded %d track(s)", g_iTrackCount);

    if (hadTracks && g_iTrackCount > 0)
        ResetAllClientPlaylists();
}

void PrecachePlaylist()
{
    for (int i = 0; i < g_iTrackCount; i++)
    {
        PrecacheSound(g_sTrackPath[i], true);

        char downloadPath[PLATFORM_MAX_PATH + 8];
        Format(downloadPath, sizeof(downloadPath), "sound/%s", g_sTrackPath[i]);
        AddFileToDownloadsTable(downloadPath);
    }
}

void ResetClientShuffle(int client)
{
    if (client < 1 || client > MaxClients)
        return;
    g_iClientShufflePos[client] = -1;
    g_iClientShuffleSize[client] = 0;
}

void ResetClientHistory(int client)
{
    if (client < 1 || client > MaxClients)
        return;
    g_iClientHistoryCount[client] = 0;
    g_iClientHistoryIndex[client] = -1;
}

void ResetAllClientPlaylists()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        ResetClientShuffle(client);
        ResetClientHistory(client);
    }
}

void BuildClientShuffle(int client, int excludeTrack = -1)
{
    if (g_iTrackCount <= 0)
        return;

    for (int i = 0; i < g_iTrackCount; i++)
        g_iClientShuffle[client][i] = i;

    for (int i = g_iTrackCount - 1; i > 0; i--)
    {
        int j = GetRandomInt(0, i);
        int tmp = g_iClientShuffle[client][i];
        g_iClientShuffle[client][i] = g_iClientShuffle[client][j];
        g_iClientShuffle[client][j] = tmp;
    }

    if (excludeTrack >= 0 && g_iTrackCount > 1 && g_iClientShuffle[client][0] == excludeTrack)
    {
        int swap = GetRandomInt(1, g_iTrackCount - 1);
        int tmp = g_iClientShuffle[client][0];
        g_iClientShuffle[client][0] = g_iClientShuffle[client][swap];
        g_iClientShuffle[client][swap] = tmp;
    }

    g_iClientShuffleSize[client] = g_iTrackCount;
    g_iClientShufflePos[client] = -1;
}

int PickNextTrack(int client)
{
    if (g_iTrackCount <= 0)
        return -1;
    if (g_iTrackCount == 1)
        return 0;

    // 1. 若玩家之前按过“上一首”回退到历史中，顺着历史向前走
    if (g_iClientHistoryIndex[client] >= 0 && g_iClientHistoryIndex[client] + 1 < g_iClientHistoryCount[client])
    {
        g_iClientHistoryIndex[client]++;
        return g_iClientHistory[client][g_iClientHistoryIndex[client]];
    }

    // 2. 否则从洗牌池中取出一首新歌
    if (g_iClientShufflePos[client] == -1 || g_iClientShuffleSize[client] != g_iTrackCount)
    {
        int prev = g_iClientTrack[client];
        BuildClientShuffle(client, prev);
        g_iClientShufflePos[client] = 0;
    }
    else
    {
        g_iClientShufflePos[client]++;
        if (g_iClientShufflePos[client] >= g_iClientShuffleSize[client])
        {
            // 当前洗牌轮次耗尽，重洗一份（首曲避开刚播完的曲目）
            int prev = g_iClientTrack[client];
            BuildClientShuffle(client, prev);
            g_iClientShufflePos[client] = 0;
        }
    }

    int nextTrack = g_iClientShuffle[client][g_iClientShufflePos[client]];

    // 3. 将新选中的曲目压入历史栈末尾
    if (g_iClientHistoryCount[client] < MUSICBOX_MAX_HISTORY)
    {
        g_iClientHistory[client][g_iClientHistoryCount[client]] = nextTrack;
        g_iClientHistoryCount[client]++;
        g_iClientHistoryIndex[client] = g_iClientHistoryCount[client] - 1;
    }
    else
    {
        // 历史栈已满，左移淘汰最旧的一首
        for (int i = 0; i < MUSICBOX_MAX_HISTORY - 1; i++)
        {
            g_iClientHistory[client][i] = g_iClientHistory[client][i + 1];
        }
        g_iClientHistory[client][MUSICBOX_MAX_HISTORY - 1] = nextTrack;
        g_iClientHistoryIndex[client] = MUSICBOX_MAX_HISTORY - 1;
    }

    return nextTrack;
}

int PickPrevTrack(int client)
{
    if (g_iTrackCount <= 0)
        return -1;
    if (g_iTrackCount == 1)
        return 0;

    // 检查历史栈中是否有更早的曲目
    if (g_iClientHistoryIndex[client] > 0)
    {
        g_iClientHistoryIndex[client]--;
        return g_iClientHistory[client][g_iClientHistoryIndex[client]];
    }

    // 历史栈已经回退到最开头，无更早曲目
    return -1;
}
