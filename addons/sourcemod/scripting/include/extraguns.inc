#if defined _extraguns_included
 #endinput
#endif
#define _extraguns_included

enum EG_GunSlot
{
    EG_Slot_Primary = 0,        // 主手主武器
    EG_Slot_Secondary = 1,      // 主手副武器
    EG_Slot_ExtraPrimary = 2,   // 备用主武器
    EG_Slot_ExtraSecondary = 3  // 备用副武器
};

/**
 * 获取玩家指定槽位的武器实体索引
 *
 * @param client        玩家 Client Index
 * @param slot          槽位枚举 (EG_Slot_Primary, EG_Slot_Secondary, EG_Slot_ExtraPrimary, EG_Slot_ExtraSecondary)
 * @return              武器实体索引 (若无则返回 0)
 */
native int ExtraGuns_GetWeapon(int client, EG_GunSlot slot);

/**
 * 手动互换玩家的主手主武器与备用主武器并拔枪
 *
 * @param client        玩家 Client Index
 */
native void ExtraGuns_SwapPrimary(int client);

/**
 * 手动互换玩家的主手副武器与备用副武器并拔枪
 *
 * @param client        玩家 Client Index
 */
native void ExtraGuns_SwapSecondary(int client);

/**
 * 玩家四槽位武器发生变化时的全局广播（装备 / 丢弃 / 双击切换 / 重建）
 *
 * @param client        玩家 Client Index
 */
forward void ExtraGuns_OnSlotsChanged(int client);

public SharedPlugin __pl_extraguns = 
{
    name = "extraguns",
    file = "extraguns.smx",
#if defined REQUIRE_PLUGIN
    required = 1,
#else
    required = 0,
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_extraguns_SetNTVOptional()
{
    MarkNativeAsOptional("ExtraGuns_GetWeapon");
    MarkNativeAsOptional("ExtraGuns_SwapPrimary");
    MarkNativeAsOptional("ExtraGuns_SwapSecondary");
}
#endif
