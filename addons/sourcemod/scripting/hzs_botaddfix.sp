//========================================================================================
// hzs_botaddfix.sp
//
// 补丁原理：
//   在 Reset() 的 hostage 循环回边中，把
//     cmp reg, [g_Hostages.m_Size]
//   改为
//     cmp reg, 0
//     nop; nop; nop
//   使循环直接退出，跳过所有 zombie hostage 初始化。
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>

#define GAMEDATA "hzs_botaddfix.gamedata"

public Plugin myinfo =
{
	name        = "HZS Bot Add Fix",
	author      = "Ducheese",
	description = "字节补丁：修复 InitializeHostageInfo buffer overflow",
	version     = "1.0",
	url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
	Handle gc = LoadGameConfigFile(GAMEDATA);
	if (gc == INVALID_HANDLE)
		SetFailState("[BotAddFix] 无法加载 gamedata");

	Address resetAddr = GameConfGetAddress(gc, "CSGameState_Reset");
	if (resetAddr == Address_Null)
		SetFailState("[BotAddFix] 无法定位 CSGameState_Reset");

	int backedgeOffset = GameConfGetOffset(gc, "LoopBackedge");
	if (backedgeOffset == -1)
		SetFailState("[BotAddFix] 无法读取 LoopBackedge offset");

	int cmpRegByte = GameConfGetOffset(gc, "CmpRegByte");
	if (cmpRegByte == -1)
		SetFailState("[BotAddFix] 无法读取 CmpRegByte");

	// 回边比较: cmp reg, 0; nop; nop; nop
	Address patchAddr = resetAddr + view_as<Address>(backedgeOffset);
	StoreToAddress(patchAddr,                       0x83,    NumberType_Int8);
	StoreToAddress(patchAddr + view_as<Address>(1), cmpRegByte, NumberType_Int8);
	StoreToAddress(patchAddr + view_as<Address>(2), 0x00,    NumberType_Int8);
	StoreToAddress(patchAddr + view_as<Address>(3), 0x90,    NumberType_Int8);
	StoreToAddress(patchAddr + view_as<Address>(4), 0x90,    NumberType_Int8);
	StoreToAddress(patchAddr + view_as<Address>(5), 0x90,    NumberType_Int8);

	LogMessage("[BotAddFix] patched back-edge at %X + 0x%X (reg=0x%02X)",
		resetAddr, backedgeOffset, cmpRegByte);

	delete gc;
}
