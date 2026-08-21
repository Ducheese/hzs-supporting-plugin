#define VERSION                      "1.1"
#define MUSICBOX_CONFIG              "configs/hzs_musicbox.cfg"
#define MUSICBOX_MAX_TRACKS          128
#define MUSICBOX_TITLE_LENGTH        128
#define MUSICBOX_ARTIST_LENGTH       96
#define MUSICBOX_DEFAULT_VOLUME      100
#define MUSICBOX_MAX_HISTORY         64

// These are the engine sound flags, not the legacy SourceMod enum values.
#define MUSICBOX_SOUND_CHANGE_VOLUME (1 << 0)
#define MUSICBOX_SOUND_STOP          (1 << 2)
#define MUSICBOX_SOUND_IGNORE_NAME   (1 << 9)

char g_sTrackPath[MUSICBOX_MAX_TRACKS][PLATFORM_MAX_PATH];
char g_sTrackTitle[MUSICBOX_MAX_TRACKS][MUSICBOX_TITLE_LENGTH];
char g_sTrackArtist[MUSICBOX_MAX_TRACKS][MUSICBOX_ARTIST_LENGTH];
float g_fTrackDuration[MUSICBOX_MAX_TRACKS];
int g_iTrackCount;

bool g_bClientMusicStarted[MAXPLAYERS + 1];
int g_iClientTrack[MAXPLAYERS + 1] = {-1, ...};
float g_fClientTrackStartTime[MAXPLAYERS + 1];
Handle g_hClientNextTrackTimer[MAXPLAYERS + 1] = {INVALID_HANDLE, ...};

// 洗牌播放池：当前随机轮次可用曲目
int g_iClientShuffle[MAXPLAYERS + 1][MUSICBOX_MAX_TRACKS];
int g_iClientShufflePos[MAXPLAYERS + 1];
int g_iClientShuffleSize[MAXPLAYERS + 1];

// 历史栈：记录玩家听过的历史曲目序列（最多 64 首）
int g_iClientHistory[MAXPLAYERS + 1][MUSICBOX_MAX_HISTORY];
int g_iClientHistoryCount[MAXPLAYERS + 1];
int g_iClientHistoryIndex[MAXPLAYERS + 1];

int g_iMusicSource = -1;

int g_iClientVolume[MAXPLAYERS + 1];
bool g_bClientMusicPlaying[MAXPLAYERS + 1];
bool g_bClientCookiesReady[MAXPLAYERS + 1];
bool g_bClientVolumeManual[MAXPLAYERS + 1];
Handle g_hCookieVolume = INVALID_HANDLE;

