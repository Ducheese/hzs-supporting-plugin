#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>
#include <clientprefs>
#include <HanZombieScenarioAPI>

#include "HZSMusicBox/globals.inc"
#include "HZSMusicBox/playlist.inc"
#include "HZSMusicBox/playback.inc"
#include "HZSMusicBox/menu.inc"

public Plugin myinfo =
{
    name = "HZS Music Box",
    author = "Ducheese",
    description = "HAN 灾变音乐盒",
    version = VERSION,
    url = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    g_hCookieVolume = RegClientCookie("hzs_musicbox_volume", "HZS Music Box Volume", CookieAccess_Protected);
    g_hCookieEnabled = RegClientCookie("hzs_musicbox_enabled", "HZS Music Box Enabled", CookieAccess_Protected);

    RegConsoleCmd("sm_musicbox", Cmd_MusicBox, "打开 HZS 音乐盒菜单");
    RegConsoleCmd("sm_musicbox_start", Cmd_MusicBoxStart, "启动自己的 HZS 音乐盒");
    RegConsoleCmd("sm_musicbox_stop", Cmd_MusicBoxStop, "停止自己的 HZS 音乐盒");
    RegConsoleCmd("sm_musicbox_toggle", Cmd_MusicBoxToggle, "切换自己的 HZS 音乐盒播放/停止状态");
    RegConsoleCmd("sm_musicbox_next", Cmd_MusicBoxNext, "切换自己的 HZS 音乐盒曲目（下一首）");
    RegConsoleCmd("sm_musicbox_prev", Cmd_MusicBoxPrev, "切换自己的 HZS 音乐盒曲目（上一首）");
    RegConsoleCmd("sm_musicbox_volume", Cmd_MusicBoxVolume, "设置 HZS 音乐盒音量");

    HookEvent("round_start", Event_RoundStart, EventHookMode_Post);
    HookEvent("player_spawn", Event_PlayerSpawn, EventHookMode_Post);
    AddNormalSoundHook(Hook_NormalSound);

    for (int client = 1; client <= MaxClients; client++)
    {
        g_iClientVolume[client] = MUSICBOX_DEFAULT_VOLUME;
        g_iClientTrack[client] = -1;
        g_bClientMusicPlaying[client] = false;
        g_bClientMusicStarted[client] = false;
        ResetClientShuffle(client);
        ResetClientHistory(client);
    }

    LoadPlaylist();
}

public void OnMapStart()
{
    // NO_MAPCHANGE timers are already destroyed by the engine at this point.
    ResetMapTimerHandles();
    StopAllMusic();
    DestroyMusicSource();
    ResetAllClientPlaylists();

    LoadPlaylist();
    PrecachePlaylist();
    CreateMusicSource();
}

public void OnMapEnd()
{
    StopAllMusic();
    DestroyMusicSource();
}

public void OnPluginEnd()
{
    RemoveNormalSoundHook(Hook_NormalSound);
    StopAllMusic();
    DestroyMusicSource();
}

public void OnClientPutInServer(int client)
{
    g_iClientVolume[client] = MUSICBOX_DEFAULT_VOLUME;
    g_bClientMusicPlaying[client] = false;
    g_bClientMusicStarted[client] = false;

    g_iClientTrack[client] = -1;
    g_fClientTrackStartTime[client] = 0.0;
    StopClientNextTrackTimer(client);
    ResetClientShuffle(client);
    ResetClientHistory(client);

    if (AreClientCookiesCached(client))
    {
        OnClientCookiesCached(client);
    }
}

public void OnClientCookiesCached(int client)
{
    if (!IsValidMusicClient(client))
        return;

    char sEnabled[8];
    GetClientCookie(client, g_hCookieEnabled, sEnabled, sizeof(sEnabled));
    g_bClientMusicStarted[client] = (sEnabled[0] != '\0' && StringToInt(sEnabled) != 0);

    char value[8];
    GetClientCookie(client, g_hCookieVolume, value, sizeof(value));

    int volume = MUSICBOX_DEFAULT_VOLUME;
    if (value[0] != '\0')
        volume = StringToInt(value);

    if (!IsAllowedVolume(volume))
        volume = MUSICBOX_DEFAULT_VOLUME;

    g_iClientVolume[client] = volume;
    if (g_bClientMusicStarted[client] && g_iClientTrack[client] < 0)
    {
        StartTrackForClient(client, PickNextTrack(client), false);
    }
}

public void OnClientDisconnect(int client)
{
    StopClientNextTrackTimer(client);
    StopTrackForClient(client);
    g_bClientMusicPlaying[client] = false;
    g_bClientMusicStarted[client] = false;
    g_iClientTrack[client] = -1;
    ResetClientShuffle(client);
    ResetClientHistory(client);
}

public void Event_RoundStart(Event event, const char[] name, bool dontBroadcast)
{
    CreateMusicSource();

    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsValidMusicClient(client))
            continue;
        if (!g_bClientMusicStarted[client])
            continue;

        StartTrackForClient(client, PickNextTrack(client), false);
    }
}

public void Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
    int client = GetClientOfUserId(event.GetInt("userid"));
    if (IsValidMusicClient(client) && g_bClientMusicStarted[client] && g_iClientTrack[client] < 0)
    {
        StartTrackForClient(client, PickNextTrack(client), false);
    }
}

public void Han_OnZombieCreated(int zombie)
{
}

public void Han_OnGameEnd()
{
}

public void Han_OnHumanWin()
{
}

public void Han_OnZombieWin()
{
    for (int client = 1; client <= MaxClients; client++)
    {
        StopTrackForClient(client);
    }
}
