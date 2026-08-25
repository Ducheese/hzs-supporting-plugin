// ============================================================================
// HZSMusicBox/globals.inc — 全局常量与变量声明
// ============================================================================

#define VERSION                      "1.2"
#define MUSICBOX_CONFIG              "configs/hzs_musicbox.cfg"
#define MUSICBOX_MAX_TRACKS          128
#define MUSICBOX_TITLE_LENGTH        128
#define MUSICBOX_ARTIST_LENGTH       96
#define MUSICBOX_DEFAULT_VOLUME      100
#define MUSICBOX_MAX_HISTORY         64

// 引擎底层音频标志位（Source 引擎标准位域）
#define MUSICBOX_SOUND_CHANGE_VOLUME (1 << 0)
#define MUSICBOX_SOUND_STOP          (1 << 2)
#define MUSICBOX_SOUND_IGNORE_NAME   (1 << 9)

// ----------------------------------------------------------------------------
// 曲目元数据（全服共享）
// ----------------------------------------------------------------------------
char g_sTrackPath[MUSICBOX_MAX_TRACKS][PLATFORM_MAX_PATH];
char g_sTrackTitle[MUSICBOX_MAX_TRACKS][MUSICBOX_TITLE_LENGTH];
char g_sTrackArtist[MUSICBOX_MAX_TRACKS][MUSICBOX_ARTIST_LENGTH];
float g_fTrackDuration[MUSICBOX_MAX_TRACKS];
int g_iTrackCount = 0;

// ----------------------------------------------------------------------------
// 播放状态与控制（按 Client 槽位）
// ----------------------------------------------------------------------------
bool g_bClientMusicStarted[MAXPLAYERS + 1];
int g_iClientVolume[MAXPLAYERS + 1];
int g_iClientTrack[MAXPLAYERS + 1];
float g_fClientTrackStartTime[MAXPLAYERS + 1];
bool g_bClientMusicPlaying[MAXPLAYERS + 1];
Handle g_hClientNextTrackTimer[MAXPLAYERS + 1];

// ----------------------------------------------------------------------------
// 随机洗牌池与历史回退栈（常驻滚动，换图与退服不重置）
// ----------------------------------------------------------------------------
int g_iClientShuffle[MAXPLAYERS + 1][MUSICBOX_MAX_TRACKS];
int g_iClientShufflePos[MAXPLAYERS + 1];
int g_iClientShuffleSize[MAXPLAYERS + 1];

int g_iClientHistory[MAXPLAYERS + 1][MUSICBOX_MAX_HISTORY];
int g_iClientHistoryCount[MAXPLAYERS + 1];
int g_iClientHistoryIndex[MAXPLAYERS + 1];

// ----------------------------------------------------------------------------
// 发声实体与 ClientPrefs Cookie 句柄
// ----------------------------------------------------------------------------
int g_iMusicSource = -1;
Handle g_hCookieVolume = INVALID_HANDLE;
Handle g_hCookieEnabled = INVALID_HANDLE;
