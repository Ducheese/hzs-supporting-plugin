//========================================================================================
// 实体追踪池（泄漏检测）
//========================================================================================

// 增量计数器
int g_iEntityCount;
char g_sEntityClass[2048][64];

int g_EC_Fog;            // env_fog_controller
int g_EC_CC;             // color_correction
int g_EC_Shooter;        // env_shooter
int g_EC_ViewCtrl;       // point_viewcontrol
int g_EC_Physics;        // prop_physics_override
int g_EC_Trigger;        // trigger_multiple
int g_EC_SmokeStack;     // env_smokestack
int g_EC_Explosion;      // env_explosion
int g_EC_Particle;       // info_particle_system
int g_EC_PropDynamic;    // prop_dynamic

// 系统归因追踪池
#define PANGZI_DUMMY_POOL  8      // 胖子投掷 dummy（prop_dynamic）
#define SMOKESTACK_POOL    16     // 迷雾僵尸 env_smokestack
#define EXPLOSION_POOL     16     // 自爆僵尸 env_explosion
#define PARTICLE_POOL      64     // info_particle_system（治疗光环 / 安哥拉自愈）

int g_iPangziDummy[PANGZI_DUMMY_POOL]   = {-1, ...};
int g_iSmokeStack[SMOKESTACK_POOL]      = {-1, ...};
int g_iExplosion[EXPLOSION_POOL]        = {-1, ...};
int g_iParticle[PARTICLE_POOL]          = {-1, ...};

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
// 调试：每秒实体总数记录（检测泄漏）
//========================================================================================

public void OnEntityCreated(int entity, const char[] classname)
{
    if (entity >= 0 && entity < sizeof(g_sEntityClass))
        strcopy(g_sEntityClass[entity], sizeof(g_sEntityClass[]), classname);

    g_iEntityCount++;

    if      (StrEqual(classname, "env_fog_controller"))    g_EC_Fog++;
    else if (StrEqual(classname, "color_correction"))      g_EC_CC++;
    else if (StrEqual(classname, "env_shooter"))           g_EC_Shooter++;
    else if (StrEqual(classname, "point_viewcontrol"))     g_EC_ViewCtrl++;
    else if (StrEqual(classname, "prop_physics_override")) g_EC_Physics++;
    else if (StrEqual(classname, "trigger_multiple"))      g_EC_Trigger++;
    else if (StrEqual(classname, "env_smokestack"))        g_EC_SmokeStack++;
    else if (StrEqual(classname, "env_explosion"))         g_EC_Explosion++;
    else if (StrEqual(classname, "info_particle_system"))  g_EC_Particle++;
    else if (StrEqual(classname, "prop_dynamic"))          g_EC_PropDynamic++;
}

public void OnEntityDestroyed(int entity)
{
    g_iEntityCount--;

    if (entity < 0 || entity >= sizeof(g_sEntityClass) || g_sEntityClass[entity][0] == '\0')
        return;

    char cls[64];
    strcopy(cls, sizeof(cls), g_sEntityClass[entity]);
    g_sEntityClass[entity][0] = '\0';

    if      (StrEqual(cls, "env_fog_controller"))    g_EC_Fog--;
    else if (StrEqual(cls, "color_correction"))      g_EC_CC--;
    else if (StrEqual(cls, "env_shooter"))           g_EC_Shooter--;
    else if (StrEqual(cls, "point_viewcontrol"))     g_EC_ViewCtrl--;
    else if (StrEqual(cls, "prop_physics_override")) g_EC_Physics--;
    else if (StrEqual(cls, "trigger_multiple"))      g_EC_Trigger--;
    else if (StrEqual(cls, "env_smokestack"))        g_EC_SmokeStack--;
    else if (StrEqual(cls, "env_explosion"))         g_EC_Explosion--;
    else if (StrEqual(cls, "info_particle_system"))  g_EC_Particle--;
    else if (StrEqual(cls, "prop_dynamic"))          g_EC_PropDynamic--;
}

public Action Timer_EntityMonitor(Handle timer)
{
    // ── 第一行：增量计数器 ──
    LogMessage("[EC] total:%d  fog:%d  cc:%d  shooter:%d  view:%d  phys:%d  trig:%d  propdy:%d  smk:%d  exp:%d  ptc:%d",
        g_iEntityCount,
        g_EC_Fog, g_EC_CC, g_EC_Shooter, g_EC_ViewCtrl,
        g_EC_Physics, g_EC_Trigger, g_EC_PropDynamic,
        g_EC_SmokeStack, g_EC_Explosion, g_EC_Particle
    );

    // ── 第二行：僵尸系统归因 ──
    int angela_fog     = (g_iFog     > 0 && IsValidEntity(g_iFog))     ? 1 : 0;
    int angela_cc      = (g_iFogCC   > 0 && IsValidEntity(g_iFogCC))   ? 1 : 0;
    int angela_shooter = CountPool(g_iDebrisShooter, DEBRIS_SHOOTER_POOL);

    int witch_cc;
    for (int i = 1; i <= MaxClients; i++)
    {
        if (g_iWitchMildCC[i]   > 0 && IsValidEntity(g_iWitchMildCC[i]))   witch_cc++;
        if (g_iWitchSevereCC[i] > 0 && IsValidEntity(g_iWitchSevereCC[i])) witch_cc++;
    }

    int trap_vis, trap_phys, trap_trig;
    for (int i = 0; i < 2048; i++)
    {
        if (g_iTrapVisual[i] > 0 && IsValidEntity(g_iTrapVisual[i]))
        {
            trap_vis++;
            trap_phys++;
        }
        if (g_iTrapTrigger[i] > 0 && IsValidEntity(g_iTrapTrigger[i]))
            trap_trig++;
    }

    int imposter;
    for (int i = 0; i < 2048; i++)
    {
        if (g_iFakeBody[i] > 0 && IsValidEntity(g_iFakeBody[i]))
            imposter++;
    }

    int pangzi_view;
    for (int i = 1; i <= MaxClients; i++)
    {
        if (g_iViewControl[i] > 0 && IsValidEntity(g_iViewControl[i]))
            pangzi_view++;
    }

    int pangzi_dummy = CountPool(g_iPangziDummy, PANGZI_DUMMY_POOL);
    int smoke        = CountPool(g_iSmokeStack,  SMOKESTACK_POOL);
    int explode      = CountPool(g_iExplosion,   EXPLOSION_POOL);
    int particle     = CountPool(g_iParticle,    PARTICLE_POOL);

    LogMessage("[EC-sys] angela_fog:%d  angela_cc:%d  angela_shooter:%d  witch_cc:%d  trap_vis:%d  trap_phys:%d  trap_trig:%d  imposter:%d  pangzi_view:%d  pangzi_dummy:%d  smoke:%d  explode:%d  particle:%d",
        angela_fog, angela_cc, angela_shooter, witch_cc, trap_vis, trap_phys, trap_trig,
        imposter, pangzi_view, pangzi_dummy, smoke, explode, particle
    );

    // ── 每 30s 全量交叉校验 ──
    static int tick;
    tick++;
    if (tick % 30 == 0)
    {
        int real_fog, real_cc, real_shooter, real_view, real_phys, real_trig, real_smk, real_exp, real_ptc, real_propdy;
        int ent = -1;
        while ((ent = FindEntityByClassname(ent, "*")) != -1)
        {
            if (!IsValidEntity(ent))
                continue;
            char cls[64];
            GetEntityClassname(ent, cls, sizeof(cls));
            if      (StrEqual(cls, "env_fog_controller"))    real_fog++;
            else if (StrEqual(cls, "color_correction"))      real_cc++;
            else if (StrEqual(cls, "env_shooter"))           real_shooter++;
            else if (StrEqual(cls, "point_viewcontrol"))     real_view++;
            else if (StrEqual(cls, "prop_physics_override")) real_phys++;
            else if (StrEqual(cls, "trigger_multiple"))      real_trig++;
            else if (StrEqual(cls, "env_smokestack"))        real_smk++;
            else if (StrEqual(cls, "env_explosion"))         real_exp++;
            else if (StrEqual(cls, "info_particle_system"))  real_ptc++;
            else if (StrEqual(cls, "prop_dynamic"))          real_propdy++;
        }

        if (g_EC_Fog      != real_fog    || g_EC_CC        != real_cc    || g_EC_Shooter   != real_shooter ||
            g_EC_ViewCtrl != real_view   || g_EC_Physics   != real_phys  || g_EC_Trigger   != real_trig    ||
            g_EC_SmokeStack != real_smk  || g_EC_Explosion != real_exp   || g_EC_Particle  != real_ptc     ||
            g_EC_PropDynamic != real_propdy)
        {
            LogMessage("[EC-chk] MISMATCH fog: inc=%d real=%d  cc: inc=%d real=%d  shooter: inc=%d real=%d  view: inc=%d real=%d  phys: inc=%d real=%d  trig: inc=%d real=%d  smk: inc=%d real=%d  exp: inc=%d real=%d  ptc: inc=%d real=%d  propdy: inc=%d real=%d",
                g_EC_Fog, real_fog, g_EC_CC, real_cc, g_EC_Shooter, real_shooter,
                g_EC_ViewCtrl, real_view, g_EC_Physics, real_phys, g_EC_Trigger, real_trig,
                g_EC_SmokeStack, real_smk, g_EC_Explosion, real_exp, g_EC_Particle, real_ptc,
                g_EC_PropDynamic, real_propdy
            );
        }
    }

    return Plugin_Continue;
}
