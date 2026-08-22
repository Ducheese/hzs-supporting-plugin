#if defined _hzs_killfeed_included
 #endinput
#endif
#define _hzs_killfeed_included

#define HZS_FEED_NORMAL   0   // 普通击杀 / 白色
#define HZS_FEED_SPECIAL  1   // 特殊 / 浅蓝
#define HZS_FEED_BOSS     2   // BOSS / 金色
#define HZS_FEED_STREAK   3   // 连杀 / 红色
#define HZS_FEED_RESCUE   4   // 救援 / 协助 / 绿色
#define HZS_FEED_TACTICAL 5   // 战术 / 橙黄

/**
 * 向 HZS KillFeed 推送自定义事件、积分奖励与 HUD 提示
 *
 * @param client       获得积分和提示的玩家客户端索引 (1-MaxClients)
 * @param eventName    显示在准星下方 Feed 队列中的文本 (例如: "解救受困队友 +200")
 * @param rewardScore  增加的积分值 (添加到 g_iScore 并刷新准星左侧积分榜)
 * @param type         事件类型 (HZS_FEED_*)
 */
native void HZS_KillFeed_AddEvent(int client, const char[] eventName, int rewardScore, int type = HZS_FEED_SPECIAL);

public SharedPlugin __pl_hzs_killfeed = 
{
    name = "hzs_killfeed",
    file = "hzs_killfeed.smx",
#if defined REQUIRE_PLUGIN
    required = 1,
#else
    required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_hzs_killfeed_SetNTVOptional()
{
    MarkNativeAsOptional("HZS_KillFeed_AddEvent");
}
#endif