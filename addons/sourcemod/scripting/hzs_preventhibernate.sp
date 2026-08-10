//========================================================================================
// hzs_preventhibernate.sp
//
// 字节补丁:阻止服务器休眠,纯 bot 服也照样跑 (仅 Steam 正版 64 位引擎)
//
// 引擎休眠判定 (IDA 实测, Steam 正版 bin\x64\engine.dll):
//   UpdateHibernationState (0x18013D780) 硬编码 SetHibernating(!bHaveAnyClients || ...),
//   正版引擎没有 sv_hibernate_when_empty cvar, 只能补丁代码。
//
// 补丁点: engine.dll RVA 0x13D85A — bHaveAnyClients = false 初始化
//   xor bpl,bpl (40 32 ED) → mov bpl,1 (40 B5 01)
//   bHaveAnyClients 恒真 → SetHibernating 永远 false → 不休眠、不踢 bot
//
// 注意: bot 不算客户端 (循环里 !IsFakeClient), 纯 bot 服不处理必休眠踢 bot
//       ("Punting bot, server is hibernating")
// 写入前 Check 字节验证, 不匹配直接 SetFailState, 绝不乱写。
//========================================================================================

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <sdktools>

#define GAMEDATA "hzs_preventhibernate.gamedata"

Address g_PatchAddr;

public Plugin myinfo =
{
    name        = "HZS Prevent Hibernate",
    author      = "Ducheese",
    description = "字节补丁:阻止服务器休眠 (Steam 正版 64 位引擎), 纯 bot 服也照样跑",
    version     = "1.0",
    url         = "https://space.bilibili.com/1889622121"
};

public void OnPluginStart()
{
    Handle gc = LoadGameConfigFile(GAMEDATA);
    if (gc == INVALID_HANDLE)
        SetFailState("[PreventHibernate] 无法加载 gamedata");

    g_PatchAddr = GameConfGetAddress(gc, "bHaveAnyClients");
    delete gc;

    if (g_PatchAddr == Address_Null)
        SetFailState("[PreventHibernate] 无法定位 bHaveAnyClients 补丁点");

    if (!Check())
        SetFailState("[PreventHibernate] 补丁点字节不匹配 (offset 可能随引擎版本变了), 已拒绝写入");

    Patch();
    LogMessage("[PreventHibernate] 字节补丁成功 @ %X (xor bpl,bpl → mov bpl,1)", g_PatchAddr);
}

//========================================================================================
// 补丁工具
//========================================================================================

bool Check()
{
    // 验证 RVA 0x13D85A 处是 xor bpl,bpl (40 32 ED) 或已补丁的 mov bpl,1 (40 B5 01)
    int bytes = LoadFromAddress(g_PatchAddr, NumberType_Int32);
    bytes &= 0x00FFFFFF;

    if (bytes == 0xED3240 || bytes == 0x01B540)
        return true;

    return false;
}

void Patch()
{
    // 保留最高字节 (不破坏相邻指令), 低 24 位改写为 mov bpl,1 = 40 B5 01
    int bytes = LoadFromAddress(g_PatchAddr, NumberType_Int32);
    bytes = (bytes & 0xFF000000) | 0x01B540;
    StoreToAddress(g_PatchAddr, bytes, NumberType_Int32);
}
