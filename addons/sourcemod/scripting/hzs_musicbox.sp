#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <clientprefs>
#include <HanZombieScenarioAPI>

#define MUSICBOX_VERSION             "1.0"
#define MUSICBOX_CONFIG              "configs/hzs_musicbox.cfg"
#define MUSICBOX_MAX_TRACKS          128
#define MUSICBOX_TITLE_LENGTH        128
#define MUSICBOX_ARTIST_LENGTH       96
#define MUSICBOX_DEFAULT_VOLUME      100
#define MUSICBOX_MAX_JOIN_ATTEMPTS   120

// These are the engine sound flags, not the legacy SourceMod enum values.
#define MUSICBOX_SOUND_CHANGE_VOLUME (1 << 0)
#define MUSICBOX_SOUND_STOP          (1 << 2)
#define MUSICBOX_SOUND_IGNORE_NAME   (1 << 9)

char g_sTrackPath[MUSICBOX_MAX_TRACKS][PLATFORM_MAX_PATH];
char g_sTrackTitle[MUSICBOX_MAX_TRACKS][MUSICBOX_TITLE_LENGTH];
char g_sTrackArtist[MUSICBOX_MAX_TRACKS][MUSICBOX_ARTIST_LENGTH];
float g_fTrackDuration[MUSICBOX_MAX_TRACKS];
int g_iTrackCount;

bool g_bRoundMusicStarted;
int g_iClientTrack[MAXPLAYERS + 1] = {-1, ...};
float g_fClientTrackStartTime[MAXPLAYERS + 1];
Handle g_hClientNextTrackTimer[MAXPLAYERS + 1] = {INVALID_HANDLE, ...};

int g_iMusicSource = -1;

int g_iClientVolume[MAXPLAYERS + 1];
bool g_bClientMusicPlaying[MAXPLAYERS + 1];
bool g_bClientCookiesReady[MAXPLAYERS + 1];
Handle g_hClientJoinTimer[MAXPLAYERS + 1] = {INVALID_HANDLE, ...};
int g_iClientJoinAttempts[MAXPLAYERS + 1];
Handle g_hCookieVolume = INVALID_HANDLE;

public Plugin myinfo =
{
    name = "HZS Music Box",
    author = "OpenCode",
    description = "HAN 灾变音乐盒",
    version = MUSICBOX_VERSION,
    url = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    g_hCookieVolume = RegClientCookie("hzs_musicbox_volume", "HZS Music Box Volume", CookieAccess_Protected);

    RegConsoleCmd("sm_musicbox", Cmd_MusicBox, "打开 HZS 音乐盒菜单");
    RegConsoleCmd("sm_musicbox_next", Cmd_MusicBoxNext, "切换自己的 HZS 音乐盒曲目");
    RegConsoleCmd("sm_musicbox_volume", Cmd_MusicBoxVolume, "设置 HZS 音乐盒音量");
    RegAdminCmd("sm_musicbox_start", Cmd_MusicBoxStart, ADMFLAG_GENERIC, "手动启动音乐盒（无 HAN 环境测试用）");
    RegAdminCmd("sm_musicbox_stop", Cmd_MusicBoxStop, ADMFLAG_GENERIC, "手动停止音乐盒（无 HAN 环境测试用）");

    HookEvent("player_team", Event_ClientReady, EventHookMode_Post);
    HookEvent("player_spawn", Event_ClientReady, EventHookMode_Post);

    for (int client = 1; client <= MaxClients; client++)
    {
        g_iClientVolume[client] = MUSICBOX_DEFAULT_VOLUME;
        g_iClientTrack[client] = -1;
        g_bClientMusicPlaying[client] = false;
        g_bClientCookiesReady[client] = false;
    }

    LoadPlaylist();
}

public void OnMapStart()
{
    // NO_MAPCHANGE timers are already destroyed by the engine at this point.
    ResetMapTimerHandles();
    StopMusic();
    DestroyMusicSource();

    LoadPlaylist();
    PrecachePlaylist();
    CreateMusicSource();
}

public void OnMapEnd()
{
    StopMusic();
    DestroyMusicSource();
}

public void OnPluginEnd()
{
    StopMusic();
    DestroyMusicSource();
}

public void OnClientPutInServer(int client)
{
    g_iClientVolume[client] = MUSICBOX_DEFAULT_VOLUME;
    g_bClientMusicPlaying[client] = false;
    g_bClientCookiesReady[client] = false;
    g_iClientJoinAttempts[client] = 0;
    KillClientJoinTimer(client);
    StopClientNextTrackTimer(client);

    g_iClientTrack[client] = -1;
    g_fClientTrackStartTime[client] = 0.0;

    if (!IsFakeClient(client) && g_bRoundMusicStarted)
        QueueClientMusic(client);
}

public void OnClientCookiesCached(int client)
{
    if (!IsValidMusicClient(client))
        return;

    g_bClientCookiesReady[client] = true;

    char value[8];
    GetClientCookie(client, g_hCookieVolume, value, sizeof(value));

    int volume = MUSICBOX_DEFAULT_VOLUME;
    if (value[0] != '\0')
        volume = StringToInt(value);

    if (!IsAllowedVolume(volume))
        volume = MUSICBOX_DEFAULT_VOLUME;

    int oldVolume = g_iClientVolume[client];
    g_iClientVolume[client] = volume;

    if (g_bRoundMusicStarted && g_iClientTrack[client] >= 0 && oldVolume != volume)
    {
        if (volume == 0)
        {
            StopClientSound(client);
        }
        else if (!g_bClientMusicPlaying[client])
        {
            PlayClientTrack(client, false);
        }
        else
        {
            EmitSoundToClient(client, g_sTrackPath[g_iClientTrack[client]], GetMusicSource(client), SNDCHAN_STATIC,
                SNDLEVEL_NONE, MUSICBOX_SOUND_CHANGE_VOLUME, float(volume) / 100.0, SNDPITCH_NORMAL);
        }
    }
}

public void OnClientDisconnect(int client)
{
    KillClientJoinTimer(client);
    StopClientNextTrackTimer(client);
    g_bClientMusicPlaying[client] = false;
    g_bClientCookiesReady[client] = false;
    g_iClientTrack[client] = -1;
}

public void Event_ClientReady(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (client > 0 && g_bRoundMusicStarted)
        QueueClientMusic(client);
}

// The first zombie opens an independent music stream for each ready player.
public void Han_OnZombieCreated(int zombie)
{
    if (!g_bRoundMusicStarted && g_iTrackCount > 0)
        StartRoundMusic();
}

public void Han_OnGameEnd()
{
    StopMusic();
}

public void Han_OnHumanWin()
{
    StopMusic();
}

public void Han_OnZombieWin()
{
    StopMusic();
}

public Action Cmd_MusicBox(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    OpenMusicBoxMenu(client);
    return Plugin_Handled;
}

public Action Cmd_MusicBoxNext(int client, int args)
{
    if (!IsValidMusicClient(client))
        return Plugin_Handled;

    if (g_iTrackCount <= 0 || !g_bRoundMusicStarted)
    {
        PrintToChat(client, "\x04[音乐盒]\x01 当前没有正在播放的曲目。");
        return Plugin_Handled;
    }

    StartTrackForClient(client, PickNextTrack(g_iClientTrack[client]), true);
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

public Action Cmd_MusicBoxStart(int client, int args)
{
    if (g_iTrackCount <= 0)
    {
        ReplyToCommand(client, "[音乐盒] 播放列表为空，无法启动。");
        return Plugin_Handled;
    }

    if (g_bRoundMusicStarted)
    {
        ReplyToCommand(client, "[音乐盒] 音乐已在播放。");
        return Plugin_Handled;
    }

    StartRoundMusic();
    ReplyToCommand(client, "[音乐盒] 已手动启动音乐（测试模式）。");
    return Plugin_Handled;
}

public Action Cmd_MusicBoxStop(int client, int args)
{
    if (!g_bRoundMusicStarted)
    {
        ReplyToCommand(client, "[音乐盒] 音乐未在播放。");
        return Plugin_Handled;
    }

    StopMusic();
    ReplyToCommand(client, "[音乐盒] 已手动停止音乐（测试模式）。");
    return Plugin_Handled;
}

void LoadPlaylist()
{
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

int PickNextTrack(int previousTrack = -1)
{
    if (g_iTrackCount <= 1)
        return 0;

    int track = GetRandomInt(0, g_iTrackCount - 1);
    if (previousTrack < 0 || previousTrack >= g_iTrackCount)
        return track;

    do
    {
        track = GetRandomInt(0, g_iTrackCount - 1);
    }
    while (track == previousTrack);

    return track;
}

void StartRoundMusic()
{
    g_bRoundMusicStarted = true;

    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsValidMusicClient(client))
            continue;

        if (GetClientTeam(client) >= 2)
            StartTrackForClient(client, PickNextTrack(), true);
        else
            QueueClientMusic(client);
    }
}

void StartTrackForClient(int client, int track, bool announce)
{
    if (!IsValidMusicClient(client) || track < 0 || track >= g_iTrackCount)
        return;

    StopTrackForClient(client);

    g_iClientTrack[client] = track;
    g_fClientTrackStartTime[client] = GetGameTime();
    g_hClientNextTrackTimer[client] = CreateTimer(g_fTrackDuration[track], Timer_NextTrack,
        GetClientUserId(client), TIMER_FLAG_NO_MAPCHANGE);

    if (g_iClientVolume[client] > 0)
    {
        EmitSoundToClient(client, g_sTrackPath[track], GetMusicSource(client), SNDCHAN_STATIC, SNDLEVEL_NONE,
            SND_NOFLAGS, float(g_iClientVolume[client]) / 100.0, SNDPITCH_NORMAL);
        g_bClientMusicPlaying[client] = true;
    }

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

    if (!g_bRoundMusicStarted || g_iClientTrack[client] < 0)
        return Plugin_Stop;

    StartTrackForClient(client, PickNextTrack(g_iClientTrack[client]), true);
    return Plugin_Stop;
}

void StopMusic()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        KillClientJoinTimer(client);
        StopTrackForClient(client);
    }

    g_bRoundMusicStarted = false;
}

void ResetMapTimerHandles()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        g_hClientNextTrackTimer[client] = INVALID_HANDLE;
        g_hClientJoinTimer[client] = INVALID_HANDLE;
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
    if (track >= 0 && track < g_iTrackCount && IsValidMusicClient(client))
    {
        EmitSoundToClient(client, g_sTrackPath[track], GetMusicSource(client), SNDCHAN_STATIC, SNDLEVEL_NONE,
            MUSICBOX_SOUND_STOP | MUSICBOX_SOUND_IGNORE_NAME, 0.0, SNDPITCH_NORMAL);
    }

    g_bClientMusicPlaying[client] = false;
}

void PlayClientTrack(int client, bool announceJoin)
{
    if (!g_bRoundMusicStarted || g_iClientTrack[client] < 0 || !IsValidMusicClient(client))
        return;

    if (!g_bClientMusicPlaying[client] && g_iClientVolume[client] > 0)
    {
        int track = g_iClientTrack[client];
        EmitSoundToClient(client, g_sTrackPath[track], GetMusicSource(client), SNDCHAN_STATIC, SNDLEVEL_NONE,
            SND_NOFLAGS, float(g_iClientVolume[client]) / 100.0, SNDPITCH_NORMAL);
        g_bClientMusicPlaying[client] = true;
    }

    if (announceJoin)
    {
        int track = g_iClientTrack[client];
        PrintToChat(client, "\x04[音乐盒]\x01 已补播放：\x03%s - %s", g_sTrackArtist[track], g_sTrackTitle[track]);
    }
}

public Action Timer_JoinMusic(Handle timer, int userid)
{
    int client = GetClientOfUserId(userid);
    if (!IsValidMusicClient(client))
        return Plugin_Stop;

    g_iClientJoinAttempts[client]++;

    if (g_bRoundMusicStarted && GetClientTeam(client) >= 2)
    {
        if (g_iClientTrack[client] < 0)
            StartTrackForClient(client, PickNextTrack(), true);
        else
            PlayClientTrack(client, true);

        g_hClientJoinTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    if (g_iClientJoinAttempts[client] >= MUSICBOX_MAX_JOIN_ATTEMPTS)
    {
        g_hClientJoinTimer[client] = INVALID_HANDLE;
        return Plugin_Stop;
    }

    return Plugin_Continue;
}

void QueueClientMusic(int client)
{
    if (!IsValidMusicClient(client) || !g_bRoundMusicStarted || g_iClientTrack[client] >= 0)
        return;

    KillClientJoinTimer(client);
    g_iClientJoinAttempts[client] = 0;
    g_hClientJoinTimer[client] = CreateTimer(0.25, Timer_JoinMusic, GetClientUserId(client),
        TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

void KillClientJoinTimer(int client)
{
    if (client < 1 || client > MaxClients)
        return;

    if (g_hClientJoinTimer[client] != INVALID_HANDLE)
    {
        KillTimer(g_hClientJoinTimer[client]);
        g_hClientJoinTimer[client] = INVALID_HANDLE;
    }
}

void OpenMusicBoxMenu(int client)
{
    Menu menu = new Menu(MenuHandler_MusicBox);

    char title[256];
    if (g_bRoundMusicStarted && g_iClientTrack[client] >= 0)
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
    menu.AddItem("volume", "调整音量");
    menu.AddItem("next", "切换我的曲目");
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

    if (g_bClientCookiesReady[client])
    {
        char value[8];
        IntToString(volume, value, sizeof(value));
        SetClientCookie(client, g_hCookieVolume, value);
    }

    if (g_bRoundMusicStarted && g_iClientTrack[client] >= 0 && oldVolume != volume)
    {
        if (volume == 0)
        {
            StopClientSound(client);
        }
        else if (oldVolume == 0 || !g_bClientMusicPlaying[client])
        {
            PlayClientTrack(client, false);
        }
        else
        {
            EmitSoundToClient(client, g_sTrackPath[g_iClientTrack[client]], GetMusicSource(client), SNDCHAN_STATIC,
                SNDLEVEL_NONE, MUSICBOX_SOUND_CHANGE_VOLUME, float(volume) / 100.0, SNDPITCH_NORMAL);
        }
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
