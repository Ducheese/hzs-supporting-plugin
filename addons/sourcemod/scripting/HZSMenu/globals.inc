// ============================================================================
// HZSMenu/globals.inc — 常量与全局变量
// ============================================================================

#define MENU_TIMEOUT  20      // 菜单显示时长（秒）

// ============================================================================
// >> 命中反馈（菜单文案：命中贴图 / 命中音效）
// ============================================================================

// Cookie 句柄（注册名保持部署过的原名，改名会导致玩家历史设置丢失）
Handle g_hCookieHitmarkerOverlay = null;
Handle g_hCookieHitmarkerSound = null;

// 开关状态（按玩家）
bool g_bHitmarkerOverlay[MAXPLAYERS + 1] = {true, ...};
bool g_bHitmarkerSound[MAXPLAYERS + 1] = {true, ...};
bool g_bHitmarkerManual[MAXPLAYERS + 1];

// ============================================================================
// >> 关羽之歌（菜单技能：全服播放音效，冷却 60s）
// ============================================================================

#define GUANYU_SOUND      "player/guanyuzhige.wav"
#define GUANYU_COOLDOWN   60.0

float g_fGuanyuCooldown[MAXPLAYERS + 1];

// ============================================================================
// >> 他人枪声消去（开关控制）
// ============================================================================

Handle g_hCookieMuteGunshots = null;
bool g_bMuteGunshots[MAXPLAYERS + 1] = {false, ...};
bool g_bGunshotManual[MAXPLAYERS + 1];

// ============================================================================
// >> 武器多持（开关控制 + 槽位显示）
// ============================================================================

Handle g_hCookieExtraGuns = null;
bool g_bExtraGuns[MAXPLAYERS + 1] = {false, ...};    // 默认关
bool g_bExtraGunsManual[MAXPLAYERS + 1];
bool g_bInExtraGunsMenu[MAXPLAYERS + 1] = {false, ...}; // 是否正处于武器多持子菜单中




