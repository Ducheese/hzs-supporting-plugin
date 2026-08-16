//========================================================================================
// hzs_bonefollowerfix.sp
//
// 字节补丁：修复 CDynamicProp::BoneFollowerManager 更新循环的空指针崩溃
//
// 背景（2026-08-16，gdb 三次实测同一崩溃点）：
//   Steam 64 服务器换图时偶发 SIGSEGV @ server.dll+0x208A1F，rcx=0 处 mov (%rcx),%rax。
//   根因：引擎更新函数（64: sub_180208900 / 32: sub_1019F340）遍历
//   m_BoneFollowerManager 的 follower 句柄列表，serial 校验通过后直接解引用
//   follower 实体的 m_pPhysicsObject（64: +0x2E0 / 32: +0x1E4）。CleanUpMap 清理
//   实体时物理对象先被释放置 0、实体句柄 serial 尚未失效 → 空指针解引用。
//   与任何插件无关（有/无 cheer 三次崩溃同地址同寄存器值）。
//
// 修复：崩溃点（mov eax,[ecx] / mov rax,[rcx]）前插入 test; jz 跳过——
//   物理对象为空时跳过物理更新调用，正是引擎缺失的空指针检查。
//   正常路径（有物理对象）行为完全不变。
//
// 布局：
//   64 位（windows64，Steam 2026 构建，imagebase 0x180000000）：
//     crash(0x180208A1F): E9 rel32 → shell（5B，覆盖 mov rax,[rcx] 3B + call 前 2B）
//     shell(0x1804238A1，函数间 int3 padding，19B）：
//       48 85 C9            test rcx,rcx
//       74 09               jz +9（物理对象为空 → 跳回循环继续）
//       48 8B 01            mov rax,[rcx]
//       FF 90 20 02 00 00   call [rax+0x220]（vtable 槽 68）
//       E9 rel32            jmp → 循环增量 inc ebx
//   32 位（windows，v91/v92 non-Steam）：4 字节就位补丁，无需壳——
//     crash(0x1019F407 / 0x101A0645): mov eax,[ecx]（2B）后正好是 push 0（2B）
//       85 C9               test ecx,ecx
//       74 0B               jz +11 → 循环增量 inc esi（跳过参数装载 + call）
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>

#define GAMEDATA "hzs_bonefollowerfix.gamedata"

public Plugin myinfo =
{
	name        = "HZS Bone Follower Fix",
	author      = "Ducheese",
	description = "字节补丁：修复 CDynamicProp bone follower 更新循环空指针崩溃（换图竞态）",
	version     = "1.0",
	url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
	Handle gc = LoadGameConfigFile(GAMEDATA);
	if (gc == INVALID_HANDLE)
		SetFailState("[BoneFollowerFix] 无法加载 gamedata");

	bool isWin64 = (GameConfGetOffset(gc, "IsWin64") == 1);

	Address sig = GameConfGetAddress(gc, "BoneFollower_Update");
	if (sig == Address_Null)
	{
		// 无匹配平台签名（如 linux），该平台无此崩溃或未做补丁
		LogError("[BoneFollowerFix] 无法定位 BoneFollower_Update 签名");
		delete gc;
		return;
	}

	int crashOff = GameConfGetOffset(gc, "BoneFollower_CrashOffset");
	Address crash = sig + view_as<Address>(crashOff);   // 崩溃点 mov eax,[ecx] / mov rax,[rcx]

	// ===== 写入前验证原字节，防签名漂移误伤 =====

	if (isWin64)
	{
		// 64 位崩溃点应为 48 8B 01 (mov rax,[rcx])
		if (LoadFromAddress(crash, NumberType_Int8) != 0x48 ||
		    LoadFromAddress(crash + view_as<Address>(1), NumberType_Int8) != 0x8B ||
		    LoadFromAddress(crash + view_as<Address>(2), NumberType_Int8) != 0x01)
			SetFailState("[BoneFollowerFix] 崩溃点字节不匹配（期望 48 8B 01），拒绝打补丁 @ %X", crash);
	}
	else
	{
		// 32 位崩溃点应为 8B 01 (mov eax,[ecx])
		if (LoadFromAddress(crash, NumberType_Int8) != 0x8B ||
		    LoadFromAddress(crash + view_as<Address>(1), NumberType_Int8) != 0x01)
			SetFailState("[BoneFollowerFix] 崩溃点字节不匹配（期望 8B 01），拒绝打补丁 @ %X", crash);
	}

	// 已是补丁字节（32: 85 C9 / 64: E9）= 插件 reload 场景，幂等跳过
	if (LoadFromAddress(crash, NumberType_Int8) == 0x85 ||
	    LoadFromAddress(crash, NumberType_Int8) == 0xE9)
	{
		LogMessage("[BoneFollowerFix] 补丁已存在，跳过");
		delete gc;
		return;
	}

	if (isWin64)
	{
		// ===== 64 位：壳方案 =====
		int shellOff = GameConfGetOffset(gc, "BoneFollower_ShellOffset");
		Address shell = sig + view_as<Address>(shellOff);   // 补丁壳（int3 padding）

		if (LoadFromAddress(shell, NumberType_Int8) != 0xCC)
			SetFailState("[BoneFollowerFix] 壳位置字节不匹配（期望 CC padding），拒绝打补丁 @ %X", shell);

		// 1. crash 点: E9 rel32 → shell
		int rel1 = view_as<int>(shell) - view_as<int>(crash) - 5;
		StoreToAddress(crash, 0xE9, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(1), rel1, NumberType_Int32);
		LogMessage("[BoneFollowerFix] crash %X -> shell %X (rel1 %X)", crash, shell, rel1);

		// 2. 壳（19 字节）
		// test rcx,rcx (48 85 C9)
		StoreToAddress(shell,                      0x48, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(1), 0x85, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(2), 0xC9, NumberType_Int8);
		// jz +9 (74 09) → 物理对象为空，直接跳回循环继续
		StoreToAddress(shell + view_as<Address>(3), 0x74, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(4), 0x09, NumberType_Int8);
		// mov rax,[rcx] (48 8B 01)
		StoreToAddress(shell + view_as<Address>(5), 0x48, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(6), 0x8B, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(7), 0x01, NumberType_Int8);
		// call [rax+0x220] (FF 90 20 02 00 00)
		StoreToAddress(shell + view_as<Address>(8),  0xFF, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(9),  0x90, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(10), 0x20, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(11), 0x02, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(12), 0x00, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(13), 0x00, NumberType_Int8);
		// E9 rel32 → 循环增量 inc ebx（crash + 9）
		Address back = crash + view_as<Address>(9);
		int rel2 = view_as<int>(back) - (view_as<int>(shell) + 0x0E + 5);
		StoreToAddress(shell + view_as<Address>(0x0E), 0xE9, NumberType_Int8);
		StoreToAddress(shell + view_as<Address>(0x0F), rel2, NumberType_Int32);
		LogMessage("[BoneFollowerFix] shell @ %X patched (rel2 %X)", shell, rel2);
	}
	else
	{
		// ===== 32 位：4 字节就位补丁（无需壳）=====
		// test ecx,ecx (85 C9)；jz +11 (74 0B) → 循环增量 inc esi
		// 被跳过：push edx; lea edx; push edx; call [eax+0x110]
		StoreToAddress(crash,                      0x85, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(1), 0xC9, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(2), 0x74, NumberType_Int8);
		StoreToAddress(crash + view_as<Address>(3), 0x0B, NumberType_Int8);
		LogMessage("[BoneFollowerFix] 32-bit patched @ %X (test ecx,ecx; jz +11)", crash);
	}

	delete gc;
}
