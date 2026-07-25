//========================================================================================
// hzs_killpenaltyfix.sp
//
// 字节补丁：绕过击杀/伤害人质的金钱惩罚和踢出惩罚
//
// 原理：
//   三个直接签名定位各补丁点，跨 v91/v92/Steam64 通用：
//
//   OnTakeDamage_Alive::AddAccount(4个参数) → 32: add esp,10h / 64: NOP
//   Event_Killed::AddAccount(4个参数)        → 同上
//   Event_Killed::CheckForHostageAbuse        → 32: add esp,4 / 64: test→xor
//
//   注意 32-bit 不能简单 NOP call，否则压栈参数无人清理导致栈损闪退。
//   64-bit 参数走寄存器（RCX/RDX/R8/R9），NOP call 安全。
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>

#define GAMEDATA "hzs_killpenaltyfix.gamedata"

bool g_bIsWin64;

public Plugin myinfo =
{
	name        = "HZS Kill Penalty Fix",
	author      = "Ducheese",
	description = "字节补丁：绕过击杀/伤害人质的金钱惩罚与踢出惩罚",
	version     = "1.0",
	url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
	Handle gc = LoadGameConfigFile(GAMEDATA);
	if (gc == INVALID_HANDLE)
		SetFailState("[KillPenaltyFix] 无法加载 gamedata");

	g_bIsWin64 = (GameConfGetOffset(gc, "IsWin64") == 1);

	// ===== 1. OnTakeDamage_Alive: 跳过 AddAccount =====
	{
		Address sig = GameConfGetAddress(gc, "OnTakeDamage_AddAccount");
		if (sig == Address_Null)
			SetFailState("[KillPenaltyFix] 无法定位 OnTakeDamage_AddAccount");

		int off = GameConfGetOffset(gc, "OnTakeDamage_CallOffset");
		Address callAddr = sig + view_as<Address>(off);

		if (g_bIsWin64)
		{
			// x64: NOP call (5 bytes). 参数在寄存器中，无栈问题
			WriteNops(callAddr, 5);
		}
		else
		{
			// x86: add esp, 10h; nop; nop (清理 4 参数 = 16 字节)
			WriteAddEsp(callAddr, 16);
		}
		LogMessage("[KillPenaltyFix] patched OnTakeDamage_AddAccount @ %X", callAddr);
	}

	// ===== 2. Event_Killed: 跳过 AddAccount =====
	{
		Address sig = GameConfGetAddress(gc, "EventKilled_AddAccount");
		if (sig == Address_Null)
			SetFailState("[KillPenaltyFix] 无法定位 EventKilled_AddAccount");

		int off = GameConfGetOffset(gc, "EventKilled_AddAccount_CallOffset");
		Address callAddr = sig + view_as<Address>(off);

		if (g_bIsWin64)
		{
			WriteNops(callAddr, 5);
		}
		else
		{
			WriteAddEsp(callAddr, 16);
		}
		LogMessage("[KillPenaltyFix] patched EventKilled_AddAccount @ %X", callAddr);
	}

	// ===== 3. Event_Killed: 跳过 CheckForHostageAbuse =====
	{
		Address sig = GameConfGetAddress(gc, "EventKilled_CheckForHostageAbuse");
		if (sig == Address_Null)
			SetFailState("[KillPenaltyFix] 无法定位 EventKilled_CheckForHostageAbuse");

		int off = GameConfGetOffset(gc, "EventKilled_Abuse_CallOffset");
		Address patchAddr = sig + view_as<Address>(off);

		if (g_bIsWin64)
		{
			// x64: abuse 内联。将 test ecx,ecx (85 C9) 改为 xor ecx,ecx (33 C9)
			// 使后续的 jle 永远成立，跳过整个惩罚块
			StoreToAddress(patchAddr,                      0x33, NumberType_Int8);
			StoreToAddress(patchAddr + view_as<Address>(1), 0xC9, NumberType_Int8);
		}
		else
		{
			// x86: add esp, 4; nop; nop (清理 1 参数 = 4 字节)
			WriteAddEsp(patchAddr, 4);
		}
		LogMessage("[KillPenaltyFix] patched EventKilled_CheckForHostageAbuse @ %X", patchAddr);
	}

	delete gc;
}

//========================================================================================
// 补丁工具函数
//========================================================================================

void WriteNops(Address addr, int count)
{
	for (int i = 0; i < count; i++)
		StoreToAddress(addr + view_as<Address>(i), 0x90, NumberType_Int8);
}

// 写入 add esp, argBytes; nop; nop（5 字节，正好覆盖一个 call E8 xx xx xx xx）
void WriteAddEsp(Address addr, int argBytes)
{
	StoreToAddress(addr,                      0x83, NumberType_Int8);
	StoreToAddress(addr + view_as<Address>(1), 0xC4, NumberType_Int8);
	StoreToAddress(addr + view_as<Address>(2), argBytes, NumberType_Int8);
	StoreToAddress(addr + view_as<Address>(3), 0x90, NumberType_Int8);
	StoreToAddress(addr + view_as<Address>(4), 0x90, NumberType_Int8);
}
