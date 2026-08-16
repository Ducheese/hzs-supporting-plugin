//========================================================================================
// hzs_gamerulesfix.sp
//
// 字节补丁：修复 g_pGameRules 为 0 时的空指针崩溃（换图/游戏规则析构竞态）
//
// 背景（2026-08-16，gdb 实测，第二次崩溃）：
//   玩家掉线后服务器崩溃 @ server.dll+0x2BFC70，rcx=0 处 mov (%rcx),%rax。
//   根因：可见性检查函数 sub_1802BFB60 在 | | 判断链中调用
//   (*(g_pGameRules vtable + 232))(g_pGameRules, ...)（vtable 槽 29）。
//   CGameRules 析构时把全局指针 qword_180753DD8（= g_pGameRules）置 0
//   （sub_1801C1A60: qword_180753DD8 = 0），而换图/重置期间实体更新链
//   （范围效果实体 → sub_180181160 → sub_1802BEC00 → sub_1802BFB60）
//   仍引用它 → 空指针解引用。引擎在别处有 cmp qword_180753DD8, 0 防御
//   （sub_180118C70），唯独此调用点缺失。
//
// 修复：崩溃点（mov rax,[rcx]）前插入 test rcx,rcx; jz ——
//   g_pGameRules 为空时跳过槽 29 调用、直接 return 0（0x1802BFC98 的
//   mov al,0 = 函数返回 false = "不可见"），行为确定且安全。
//   正常路径（g_pGameRules 非空）完全不变。
//
// v1.1 修正（2026-08-16 实测）：v1.0 的 E9 rel2 目标写错——
//   跳 0x1802BFC98（crash+0x28）是无条件 return false，把正常路径的
//   call 后判断链（test al,al; jz 0x1B）整个跳过，函数恒返回 false，
//   导致 HAN zombiehurt 判定失效（飙血/hitmarker 不显示）。
//   正确目标 = call 后第一条指令 crash+9（84 C0 test al,al），
//   由原代码自己的 jz 决定何时 return false。
//
// 布局（server.dll 2026-07-12 构建，imagebase 0x180000000）：
//   crash(0x1802BFC70): E9 rel32 → shell（5B，覆盖 mov rax,[rcx] 3B + call 前 2B）
//   shell(0x180433791，函数间 int3 padding，19B）：
//     48 85 C9            test rcx,rcx
//     74 09               jz +9（g_pGameRules 为空 → 跳 E9 → return false）
//     48 8B 01            mov rax,[rcx]
//     FF 90 E8 00 00 00   call [rax+0xE8]（vtable 槽 29）
//     E9 rel32            jmp → crash+9（test al,al，恢复原流程）
//
// 只在 windows64 生效。
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>

#define GAMEDATA "hzs_gamerulesfix.gamedata"

public Plugin myinfo =
{
	name        = "HZS GameRules Fix",
	author      = "Ducheese",
	description = "字节补丁：修复 g_pGameRules 析构期间空指针崩溃（换图竞态）",
	version     = "1.1",
	url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
	Handle gc = LoadGameConfigFile(GAMEDATA);
	if (gc == INVALID_HANDLE)
		SetFailState("[GameRulesFix] 无法加载 gamedata");

	Address sig = GameConfGetAddress(gc, "GameRulesCheck");
	if (sig == Address_Null)
	{
		LogError("[GameRulesFix] 无法定位 GameRulesCheck 签名（非 windows64 平台？）");
		delete gc;
		return;
	}

	int crashOff = GameConfGetOffset(gc, "GameRules_CrashOffset");
	int shellOff = GameConfGetOffset(gc, "GameRules_ShellOffset");

	Address crash = sig + view_as<Address>(crashOff);   // 崩溃点 mov rax,[rcx]
	Address shell = sig + view_as<Address>(shellOff);   // 补丁壳（int3 padding）

	// ===== 写入前验证原字节，防签名漂移误伤 =====

	// 壳跳转 rel2（恢复点 = call 后第一条指令 crash+9），先算好供旧版补丁检测复用
	Address resume = crash + view_as<Address>(9);
	int rel2 = view_as<int>(resume) - (view_as<int>(shell) + 0x0E + 5);

	// crash 处已是 E9 = 有旧补丁在场：核对壳的 rel2。rel2 不匹配 = v1.0 坏补丁
	//（跳 crash+0x28 恒返回 false），还原原字节后重打；匹配则幂等跳过。
	bool bUpgrade = false;
	if (LoadFromAddress(crash, NumberType_Int8) == 0xE9)
	{
		int existingRel2 = LoadFromAddress(shell + view_as<Address>(0x0F), NumberType_Int32);
		if (existingRel2 == rel2)
		{
			LogMessage("[GameRulesFix] 补丁已存在且版本一致，跳过");
			delete gc;
			return;
		}
		LogMessage("[GameRulesFix] 检测到旧版补丁（rel2 %X != %X），还原重打", existingRel2, rel2);
		bUpgrade = true;

		// 还原 crash 点被 E9 rel32 覆盖的 5 字节（48 8B 01 FF 90）
		StoreToAddress(crash,                       0x48, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(1), 0x8B, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(2), 0x01, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(3), 0xFF, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(4), 0x90, NumberType_Int8);
	}

	// crash 处应为 48 8B 01 (mov rax,[rcx])
	if (LoadFromAddress(crash, NumberType_Int8) != 0x48 ||
	    LoadFromAddress(crash + view_as<Address>(1), NumberType_Int8) != 0x8B ||
	    LoadFromAddress(crash + view_as<Address>(2), NumberType_Int8) != 0x01)
	{
		SetFailState("[GameRulesFix] 崩溃点字节不匹配（期望 48 8B 01），拒绝打补丁 @ %X", crash);
	}

	// shell 处应为 int3 padding (CC)；升级路径下被旧壳占用，先清回 CC
	if (bUpgrade)
	{
		for (int i = 0; i < 19; i++)
			StoreToAddress(shell + view_as<Address>(i), 0xCC, NumberType_Int8);
	}
	else if (LoadFromAddress(shell, NumberType_Int8) != 0xCC)
	{
		SetFailState("[GameRulesFix] 壳位置字节不匹配（期望 CC padding），拒绝打补丁 @ %X", shell);
	}

	// ===== 1. crash 点: E9 rel32 → shell =====
	int rel1 = view_as<int>(shell) - view_as<int>(crash) - 5;
	StoreToAddress(crash, 0xE9, NumberType_Int8);
	StoreToAddress(crash + view_as<Address>(1), rel1, NumberType_Int32);
	LogMessage("[GameRulesFix] crash %X -> shell %X (rel1 %X)", crash, shell, rel1);

	// ===== 2. 壳（19 字节）=====
	// test rcx,rcx (48 85 C9)
	StoreToAddress(shell,                      0x48, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(1), 0x85, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(2), 0xC9, NumberType_Int8);
	// jz +9 (74 09) → g_pGameRules 为空，跳 return 0
	StoreToAddress(shell + view_as<Address>(3), 0x74, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(4), 0x09, NumberType_Int8);
	// mov rax,[rcx] (48 8B 01)
	StoreToAddress(shell + view_as<Address>(5), 0x48, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(6), 0x8B, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(7), 0x01, NumberType_Int8);
	// call [rax+0xE8] (FF 90 E8 00 00 00)
	StoreToAddress(shell + view_as<Address>(8),  0xFF, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(9),  0x90, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(10), 0xE8, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(11), 0x00, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(12), 0x00, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(13), 0x00, NumberType_Int8);
	// E9 rel32 → crash+9（call [rax+0xE8] 后的 test al,al，恢复原判断流程）
	//   不能跳 crash+0x28（jz 目标 = return false）——那会让非空路径恒返回 false
	StoreToAddress(shell + view_as<Address>(0x0E), 0xE9, NumberType_Int8);
	StoreToAddress(shell + view_as<Address>(0x0F), rel2, NumberType_Int32);
	LogMessage("[GameRulesFix] shell @ %X patched (rel2 %X)", shell, rel2);

	delete gc;
}
