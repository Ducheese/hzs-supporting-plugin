//========================================================================================
// 实体监控（泄漏检测）
//
// 设计：两层，互不依赖。
//
// 第一层 — 技能归因（精确，本插件自有实体）
//   每个技能在「创建实体」处把实体记进所属系统的槽位（定长池 Push/Pop，或按
//   owner 索引的数组），在「销毁」处清空。Timer 里用 IsValidEntity 数每个系统
//   的存活实体。EntRef + IsValidEntity 自带有效性自检：就算某条销毁路径漏了
//   Pop，槽位也只会读成空，绝不会把计数虚高。
//
// 第二层 — reality 直扫（兜底，服务器范围）
//   每 30s 用 FindEntityByClassname("*") 把全图扫一遍，按 g_sTrackedClasses
//   统计各类绝对数量。它直接看现实，不依赖任何记账，所以哪怕某技能忘了入池 /
//   漏了清理，只要实体还在服务器上，这里就能暴露出来。同时给出服务器范围内的
//   同类实体总数（含地图 / 其他插件创建的，作为上下文噪声，无害）。
//
// ── 新增技能接入清单（务必全部完成）────────────────────────────────────────
//   1. 在创建处把实体写进所属系统槽位（Push 进池，或存入 owner 索引数组）。
//   2. 在每一条销毁路径清空该槽位（Pop，或置 -1）。
//   3. 在下方 Timer_EntityMonitor 的「技能归因」段加一行计数并打进 [EC-sys]。
//   4. 若创建的 classname 是本插件从未用过的，追加到 g_sTrackedClasses（否则
//      reality 扫描看不到它）。
//========================================================================================

//========================================================================================
// 定长池（多实例、无天然 owner 键的实体用 Push/Pop 池）
//========================================================================================

#define PANGZI_DUMMY_POOL  64     // 胖子投掷 dummy（prop_dynamic）
#define SMOKESTACK_POOL    64     // 迷雾僵尸 env_smokestack
#define EXPLOSION_POOL     64     // 自爆僵尸 env_explosion
#define PARTICLE_POOL      64     // info_particle_system（治疗光环 / 安哥拉自愈）
#define TESLA_POOL         64     // 神秘电弧 point_tesla

int g_iPangziDummy[PANGZI_DUMMY_POOL]   = {-1, ...};
int g_iSmokeStack[SMOKESTACK_POOL]      = {-1, ...};
int g_iExplosion[EXPLOSION_POOL]        = {-1, ...};
int g_iParticle[PARTICLE_POOL]          = {-1, ...};
int g_iTesla[TESLA_POOL]                = {-1, ...};

//========================================================================================
// 池操作
//========================================================================================

stock bool PushEntRef(int[] pool, int poolSize, int entity)
{
    int ref = EntIndexToEntRef(entity);
    for (int i = 0; i < poolSize; i++)
    {
        int cur = EntRefToEntIndex(pool[i]);
        if (cur == INVALID_ENT_REFERENCE || !IsValidEntity(cur))
        {
            pool[i] = ref;
            return true;
        }
    }
    return false;
}

stock void PopEntRef(int[] pool, int poolSize, int entity)
{
    for (int i = 0; i < poolSize; i++)
    {
        if (EntRefToEntIndex(pool[i]) == entity)
        {
            pool[i] = -1;
            return;
        }
    }
}

stock int CountPool(const int[] pool, int poolSize)
{
    int count = 0;
    for (int i = 0; i < poolSize; i++)
    {
        int ent = EntRefToEntIndex(pool[i]);
        if (ent != INVALID_ENT_REFERENCE && IsValidEntity(ent))
            count++;
    }
    return count;
}

//========================================================================================
// 本插件创建过的实体类注册表（reality 扫描的唯一白名单）
// 新增技能若用到新的 classname，必须在此追加，否则 reality 扫描忽略它。
//========================================================================================

char g_sTrackedClasses[][64] = {
    "prop_dynamic",            // 胖子 dummy / 伪装假身 / 屠夫视觉 / 神秘传送门
    "prop_dynamic_override",   // 恶魔投掷物
    "prop_physics_override",   // 屠夫受击体
    "trigger_multiple",        // 屠夫触发器
    "env_fog_controller",      // 安哥拉毒雾
    "color_correction",        // 安哥拉毒雾CC / 女巫灰度CC
    "env_shooter",             // 安哥拉碎片
    "env_smokestack",          // 迷雾僵尸
    "env_explosion",           // 自爆僵尸
    "info_particle_system",    // 治疗光环 / 安哥拉自愈
    "point_tesla",             // 神秘电弧
    "point_viewcontrol"        // 胖子视角
};

//========================================================================================
// 每秒采样：技能归因 + （每 30s）reality 直扫
//========================================================================================

public int StatsPanelHandler(Menu menu, MenuAction action, int param1, int param2)
{
    return 0;
}

public Action Timer_EntityMonitor(Handle timer)
{
    // ── 第一层：技能归因 ──
    int angela_fog     = (g_iFog   > 0 && IsValidEntity(g_iFog))   ? 1 : 0;
    int angela_cc      = (g_iFogCC > 0 && IsValidEntity(g_iFogCC)) ? 1 : 0;
    int angela_shooter = CountPool(g_iDebrisShooter, DEBRIS_SHOOTER_POOL);

    int witch_cc;
    for (int i = 1; i <= MaxClients; i++)
    {
        if (g_iWitchMildCC[i]   > 0 && IsValidEntity(g_iWitchMildCC[i]))   witch_cc++;
        if (g_iWitchSevereCC[i] > 0 && IsValidEntity(g_iWitchSevereCC[i])) witch_cc++;
    }

    int trap;            // 每个陷阱 = visual + phys + trigger 各一
    for (int i = 0; i < 2048; i++)
        if (g_iTrapVisual[i] > 0 && IsValidEntity(g_iTrapVisual[i])) trap++;

    int imposter;
    for (int i = 0; i < 2048; i++)
        if (g_iFakeBody[i] > 0 && IsValidEntity(g_iFakeBody[i])) imposter++;

    int pangzi_view;
    for (int i = 1; i <= MaxClients; i++)
        if (g_iViewControl[i] > 0 && IsValidEntity(g_iViewControl[i])) pangzi_view++;

    int pangzi_dummy = CountPool(g_iPangziDummy, PANGZI_DUMMY_POOL);
    int smoke        = CountPool(g_iSmokeStack,  SMOKESTACK_POOL);
    int explode      = CountPool(g_iExplosion,   EXPLOSION_POOL);
    int particle     = CountPool(g_iParticle,    PARTICLE_POOL);
    int tesla        = CountPool(g_iTesla,       TESLA_POOL);

    int deimos_proj;
    for (int i = 0; i < 2048; i++)
        if (g_iDeimosProjectile[i] > 0 && IsValidEntity(g_iDeimosProjectile[i])) deimos_proj++;

    int mystery_portal;
    for (int i = 0; i < 2048; i++)
        if (g_iPortal[i] > 0 && IsValidEntity(g_iPortal[i])) mystery_portal++;

    // 本插件存活实体总数（陷阱按 3 实体计：visual/phys/trigger）
    int plugin_total = angela_fog + angela_cc + angela_shooter + witch_cc + trap * 3
                     + imposter + pangzi_view + pangzi_dummy + smoke + explode
                     + particle + tesla + deimos_proj + mystery_portal;

    // ── 僵尸种类统计（HAN API 实时 tally）──
    StringMap zCounts = new StringMap();
    int zTotal = 0;
    int zc = Han_GetZombieCount();
    for (int i = 0; i < zc; i++)
    {
        int z = Han_GetZombieByIndex(i);
        if (Han_IsZombie(z))
        {
            char zname[64];
            Han_GetZombieName(z, zname, sizeof(zname));
            int v = 0;
            zCounts.GetValue(zname, v);
            zCounts.SetValue(zname, v + 1);
            zTotal++;
        }
    }

    // ── 构建左侧 Panel ──
    Panel panel = new Panel();
    char line[128];

    Format(line, sizeof(line), "僵尸%d / 技能实体%d", zTotal, plugin_total);
    panel.SetTitle(line);

    // 12 种僵尸：每行 = 僵尸计数 + 其相关实体（按实体本质 classname 标注）
    int zn;
    zn = 0; zCounts.GetValue(ZOMBIE_SMOKE, zn);
    Format(line, sizeof(line), "迷雾:%d / env_smokestack:%d", zn, smoke);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_EXPLODE, zn);
    Format(line, sizeof(line), "自爆:%d / env_explosion:%d", zn, explode);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_HEAL, zn);
    Format(line, sizeof(line), "治疗:%d / info_particle:%d", zn, particle);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_GHOST, zn);
    Format(line, sizeof(line), "幽灵:%d", zn);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_DEIMOS, zn);
    Format(line, sizeof(line), "恶魔:%d / projectile:%d", zn, deimos_proj);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_BUTCHER, zn);
    Format(line, sizeof(line), "屠夫:%d / trap:%d", zn, trap);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_WITCH, zn);
    Format(line, sizeof(line), "女巫:%d / witch_cc:%d", zn, witch_cc);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_IMPOSTER, zn);
    Format(line, sizeof(line), "伪人:%d / fakebody:%d", zn, imposter);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(ZOMBIE_MYSTERY, zn);
    Format(line, sizeof(line), "神秘:%d / portal:%d tesla:%d", zn, mystery_portal, tesla);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(BOSS_ANGELA, zn);
    Format(line, sizeof(line), "牢安:%d / env_fog:%d fog_cc:%d env_shooter:%d",
        zn, angela_fog, angela_cc, angela_shooter);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(BOSS_PANGZI, zn);
    Format(line, sizeof(line), "巨型:%d / viewcontrol:%d dummy:%d",
        zn, pangzi_view, pangzi_dummy);
    panel.DrawText(line);

    zn = 0; zCounts.GetValue(BOSS_YIXING, zn);
    Format(line, sizeof(line), "异形:%d", zn);
    panel.DrawText(line);

    delete zCounts;

    // 每秒重发给所有未在菜单中的真实玩家，保持常驻
    for (int client = 1; client <= MaxClients; client++)
    {
        if (IsClientInGame(client) && !IsFakeClient(client) && GetClientMenu(client) == MenuSource_None)
            SendPanelToClient(panel, client, StatsPanelHandler, 2);
    }
    delete panel;

    // ── 第二层：reality 直扫（每 30s，日志兜底，泄漏检测靠它）──
    static int tick;
    tick++;
    if (tick % 30 == 0)
    {
        int counts[sizeof(g_sTrackedClasses)];
        int ent = -1;
        while ((ent = FindEntityByClassname(ent, "*")) != -1)
        {
            if (!IsValidEntity(ent))
                continue;
            char cls[64];
            GetEntityClassname(ent, cls, sizeof(cls));
            for (int i = 0; i < sizeof(g_sTrackedClasses); i++)
            {
                if (StrEqual(cls, g_sTrackedClasses[i]))
                {
                    counts[i]++;
                    break;
                }
            }
        }

        char buf[1024];
        buf[0] = '\0';
        for (int i = 0; i < sizeof(g_sTrackedClasses); i++)
            Format(buf, sizeof(buf), "%s %s:%d", buf, g_sTrackedClasses[i], counts[i]);
        LogMessage("[EC-scan]%s", buf);
    }

    return Plugin_Continue;
}
