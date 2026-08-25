// ============================================================================
// HZSMusicBox/playlist.inc — 播放列表读取、预缓存、洗牌与历史栈管理
// ============================================================================

void LoadPlaylist()
{
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

    char newPaths[MUSICBOX_MAX_TRACKS][PLATFORM_MAX_PATH];
    char newTitles[MUSICBOX_MAX_TRACKS][MUSICBOX_TITLE_LENGTH];
    char newArtists[MUSICBOX_MAX_TRACKS][MUSICBOX_ARTIST_LENGTH];
    float newDurations[MUSICBOX_MAX_TRACKS];
    int newCount = 0;

    do
    {
        if (newCount >= MUSICBOX_MAX_TRACKS)
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
            LogError("Music box track %d is missing path or positive duration", newCount + 1);
            continue;
        }

        strcopy(newPaths[newCount], sizeof(newPaths[]), path);
        strcopy(newTitles[newCount], sizeof(newTitles[]), title);
        strcopy(newArtists[newCount], sizeof(newArtists[]), artist);
        newDurations[newCount] = duration;
        newCount++;
    }
    while (kv.GotoNextKey());

    delete kv;

    // 比对曲目配置是否真正发生变动
    bool playlistChanged = (newCount != g_iTrackCount);
    if (!playlistChanged)
    {
        for (int i = 0; i < newCount; i++)
        {
            if (!StrEqual(g_sTrackPath[i], newPaths[i]))
            {
                playlistChanged = true;
                break;
            }
        }
    }

    g_iTrackCount = newCount;
    for (int i = 0; i < newCount; i++)
    {
        strcopy(g_sTrackPath[i], sizeof(g_sTrackPath[]), newPaths[i]);
        strcopy(g_sTrackTitle[i], sizeof(g_sTrackTitle[]), newTitles[i]);
        strcopy(g_sTrackArtist[i], sizeof(g_sTrackArtist[]), newArtists[i]);
        g_fTrackDuration[i] = newDurations[i];
    }

    PrintToServer("[HZS Music Box] Loaded %d track(s)", g_iTrackCount);

    // 仅在曲目配置真正发生变更时（如服主增删歌曲）才重置洗牌池与历史栈
    if (playlistChanged)
    {
        ResetAllClientPlaylists();
    }
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

    // Fisher-Yates 洗牌算法
    for (int i = g_iTrackCount - 1; i > 0; i--)
    {
        int j = GetRandomInt(0, i);
        int tmp = g_iClientShuffle[client][i];
        g_iClientShuffle[client][i] = g_iClientShuffle[client][j];
        g_iClientShuffle[client][j] = tmp;
    }

    // 避免新轮次第一首与刚播完的上一首完全相同
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

    // 1. 若玩家之前按过“上一首”回退在历史中，顺着历史向前进
    if (g_iClientHistoryIndex[client] >= 0 && g_iClientHistoryIndex[client] + 1 < g_iClientHistoryCount[client])
    {
        g_iClientHistoryIndex[client]++;
        return g_iClientHistory[client][g_iClientHistoryIndex[client]];
    }

    // 2. 否则从洗牌池中取出一首新曲
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
            // 当前洗牌轮次耗尽，重洗一份
            int prev = g_iClientTrack[client];
            BuildClientShuffle(client, prev);
            g_iClientShufflePos[client] = 0;
        }
    }

    int nextTrack = g_iClientShuffle[client][g_iClientShufflePos[client]];

    // 3. 将新选中的曲目压入历史栈末尾（滑动窗口）
    if (g_iClientHistoryCount[client] < MUSICBOX_MAX_HISTORY)
    {
        g_iClientHistory[client][g_iClientHistoryCount[client]] = nextTrack;
        g_iClientHistoryCount[client]++;
        g_iClientHistoryIndex[client] = g_iClientHistoryCount[client] - 1;
    }
    else
    {
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

    // 已回退到历史栈最开头
    return -1;
}
