// ============================================================================
// HZSMenu/guidemenu.inc — 玩法提示（左上角 HUD 小作文，显示 20 秒后自动消失）
//
// 引擎限制（SM 核心 smn_hudtext.cpp + czero client message.cpp 实证）：
//   - 每条 ShowHudText 的消息 buffer 上限 = 255-36 = 219 字节，超长被 SM 静默截断
//   - 6 行文案拆 3 段（每段 ≤219 字节）放 3 个通道，各自指定左上角 Y 位置
//   - 一次发送 hold 20 秒即消失，不做高频刷新（避免撑满引擎 HUD 队列影响 killfeed）
// 通道占用：killfeed 用 1/2；玩法提示用 3/4/5（0 空闲）
// ============================================================================

#define GUIDE_CHAN_A 3          // 段 A：行1+行2
#define GUIDE_CHAN_B 4          // 段 B：行3+行4
#define GUIDE_CHAN_C 5          // 段 C：行5+行6

#define GUIDE_X    0.16         // X：右移避开左上角雷达图（CS:S 雷达无法隐藏，无 convar 控制）
// 每行约 0.045 屏高，2 行一段 → 段间隔 0.09，紧凑贴行
// C 段底部 ≈ 0.38，在 killfeed 积分榜（Y 0.40）之上，不重叠
#define GUIDE_Y_A  0.13         // 段 A 起点（2 行）
#define GUIDE_Y_B  0.22         // 段 B 起点（2 行）
#define GUIDE_Y_C  0.31         // 段 C 起点（2 行）
#define GUIDE_HOLD 20.0         // 显示时长（秒），到时自动消失

void ShowGuideHUD(int client)
{
    SetHudTextParams(GUIDE_X, GUIDE_Y_A, GUIDE_HOLD, 0, 255, 255, 255);
    ShowHudText(client, GUIDE_CHAN_A, "初始资源 - 5000金钱、2000备弹量\n单局积分 - 普通+50、特殊+200、Boss+1000、连杀+100/200/300");

    SetHudTextParams(GUIDE_X, GUIDE_Y_B, GUIDE_HOLD, 0, 255, 255, 255);
    ShowHudText(client, GUIDE_CHAN_B, "机动提升 - 自动连跳(长按空格)、滑铲(长按蹲)、降落伞(长按E)\n场景提升 - 滑索(按E使用)、炮台(!menu部署)");

    SetHudTextParams(GUIDE_X, GUIDE_Y_C, GUIDE_HOLD, 0, 255, 255, 255);
    ShowHudText(client, GUIDE_CHAN_C, "快速近战 - 推荐bind MOUSE3 sm_quickmelee按鼠标中键使用\n僵尸技能 - 攻击神秘僵尸回甲回血、击杀伪人僵尸送子弹");
}
