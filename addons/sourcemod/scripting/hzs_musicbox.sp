// ============================================================================
// hzs_musicbox.sp — HAN 灾变音乐盒主插件
// ============================================================================

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

// ============================================================================
// >> 插件生命周期
// ============================================================================

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
    AddNormalSoundHook(Hook_NormalSound);

    // 仅在服务器启动/插件载入时初始化槽位默认值
    for (int client = 1; client <= MaxClients; client++)
    {
        g_iClientVolume[client] = MUSICBOX_DEFAULT_VOLUME;
        g_iClientTrack[client] = -1;
        g_fClientTrackStartTime[client] = 0.0;
        g_bClientMusicPlaying[client] = false;
        g_bClientMusicStarted[client] = false;
        g_hClientNextTrackTimer[client] = INVALID_HANDLE;
        ResetClientShuffle(client);
        ResetClientHistory(client);
    }

    LoadPlaylist();
}

public void OnMapStart()
{
    // 换图时清空计时器句柄、停止上一张图的声音并销毁旧发声实体
    ResetMapTimerHandles();
    StopAllMusicSound();
    DestroyMusicSource();

    LoadPlaylist();
    PrecachePlaylist();
    CreateMusicSource();
}

public void OnMapEnd()
{
    StopAllMusicSound();
    DestroyMusicSource();
}

public void OnPluginEnd()
{
    RemoveNormalSoundHook(Hook_NormalSound);
    StopAllMusicSound();
    DestroyMusicSource();
}

// ============================================================================
// >> 玩家连接与 Cookie 管理
// ============================================================================

public void OnClientPutInServer(int client)
{
    // 仅清理单局/单图的发声运行状态，绝不碰历史栈和洗牌池
    g_iClientTrack[client] = -1;
    g_fClientTrackStartTime[client] = 0.0;
    g_bClientMusicPlaying[client] = false;
    StopClientNextTrackTimer(client);

    if (AreClientCookiesCached(client))
    {
        LoadClientCookies(client);
    }
}

public void OnClientDisconnect(int client)
{
    // 仅停止当前正在播放的声音与定时器，绝不碰历史栈和洗牌池
    StopTrackForClient(client);
}

public void OnClientCookiesCached(int client)
{
    if (!IsValidMusicClient(client))
        return;

    LoadClientCookies(client);
}

void LoadClientCookies(int client)
{
    char sEnabled[8];
    GetClientCookie(client, g_hCookieEnabled, sEnabled, sizeof(sEnabled));
    if (sEnabled[0] != '\0')
    {
        g_bClientMusicStarted[client] = (StringToInt(sEnabled) != 0);
    }

    char sVolume[8];
    GetClientCookie(client, g_hCookieVolume, sVolume, sizeof(sVolume));

    int volume = MUSICBOX_DEFAULT_VOLUME;
    if (sVolume[0] != '\0')
        volume = StringToInt(sVolume);

    if (!IsAllowedVolume(volume))
        volume = MUSICBOX_DEFAULT_VOLUME;

    int oldVolume = g_iClientVolume[client];
    g_iClientVolume[client] = volume;

    // 1. 若当前已在播放且音量有变动，热更新音量
    if (g_bClientMusicStarted[client] && g_iClientTrack[client] >= 0 && oldVolume != volume)
    {
        float newVol = float(volume) / 100.0;
        if (newVol == 0.0) newVol = 0.02;
        EmitSoundToClient(client, g_sTrackPath[g_iClientTrack[client]], GetMusicSource(client), SNDCHAN_STATIC,
            SNDLEVEL_NONE, MUSICBOX_SOUND_CHANGE_VOLUME, newVol, SNDPITCH_NORMAL);
        g_bClientMusicPlaying[client] = true;
    }
    // 2. 中途进服补播：玩家已开启音乐盒且当前耳边无音乐在播放
    else if (g_bClientMusicStarted[client] && g_iClientTrack[client] < 0)
    {
        StartTrackForClient(client, PickNextTrack(client), false);
    }
}

// ============================================================================
// >> 游戏事件与灾变接口
// ============================================================================

public void Event_RoundStart(Event event, const char[] name, bool dontBroadcast)
{
    CreateMusicSource();

    for (int client = 1; client <= MaxClients; client++)
    {
        if (!IsValidMusicClient(client))
            continue;
        if (!g_bClientMusicStarted[client])
            continue;
        if (g_iClientTrack[client] >= 0)
            continue;

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
