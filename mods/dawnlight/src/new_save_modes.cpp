#include "config.hpp"
#include "save_compat.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "SSystem/SComponent/c_math.h"
#include "Z2AudioLib/Z2SeqMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_Reset.h"
class JPABaseEmitter;
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_b_gnd.h"
#include "d/actor/d_a_midna.h"
#include "d/d_drawlist.h"
#include "d/actor/d_a_mant.h"
#include "d/actor/d_a_obj_gb.h"
#include "d/actor/d_a_obj_bosswarp.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_debug_viewer.h"
#include "d/d_file_select.h"
#include "d/d_gameover.h"
#include "d/d_item.h"
#include "d/d_item_data.h"
#include "d/d_meter2.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_class.h"
#include "d/d_msg_flow.h"
#include "d/d_msg_object.h"
#include "d/d_msg_scrn_talk.h"
#include "d/d_s_name.h"
#include "d/d_save.h"
#include "d/d_stage.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_overlap_mng.h"
#include "f_pc/f_pc_name.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace dawnlight {
namespace {

DEFINE_HOOK(&dScnName_c::changeGameScene, NameSceneChangeGameSceneHook);
DEFINE_HOOK(&dFile_select_c::nameInput2, FileSelectNameInput2Hook);
DEFINE_HOOK(
    static_cast<void (*)(const char*, s16, s8, s8, f32, u32, int, s8, s16, int, int)>(
        &dComIfGp_setNextStage),
    SetNextStageHook);
DEFINE_HOOK(&dStage_changeScene, StageChangeSceneHook);
DEFINE_HOOK(&daObjBossWarp_c::execute, BossWarpExecuteHook);
DEFINE_HOOK(&dMsgObject_c::selectProc, MsgObjectSelectProcHook);
DEFINE_HOOK(&dMsgScrnBase_c::setString, MsgScrnBaseSetStringHook);
DEFINE_HOOK(&dMsgScrnTalk_c::setSelectString, MsgScrnTalkSetSelectStringHook);
DEFINE_HOOK(&dMeter2_c::_execute, MeterExecuteHook);
#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__APPLE__)
DEFINE_HOOK_SYMBOL("_ZL15dScnPly_ExecuteP9dScnPly_c", int(void*), PlaySceneUpdateHook);
DEFINE_HOOK_SYMBOL("_ZL12dScnPly_DrawP9dScnPly_c", int(void*), PlaySceneDrawHook);
DEFINE_HOOK_SYMBOL("_ZL15daB_GND_ExecuteP11b_gnd_class", int(b_gnd_class*), GanondorfExecuteHook);
DEFINE_HOOK_SYMBOL("_ZL16daObj_Gb_ExecuteP12obj_gb_class", int(obj_gb_class*), GanondorfBarrierExecuteHook);
#else
DEFINE_HOOK_SYMBOL("dScnPly_Execute", int(void*), PlaySceneUpdateHook);
DEFINE_HOOK_SYMBOL("dScnPly_Draw", int(void*), PlaySceneDrawHook);
DEFINE_HOOK_SYMBOL("daB_GND_Execute", int(b_gnd_class*), GanondorfExecuteHook);
DEFINE_HOOK_SYMBOL("daObj_Gb_Execute", int(obj_gb_class*), GanondorfBarrierExecuteHook);
#endif

constexpr size_t kReserveOffset = 0x8F0;
constexpr size_t kIntroSkipOffset = 16;
constexpr size_t kBossRushOffset = 32;
constexpr size_t kBossRushIndexOffset = 40;
constexpr size_t kBossRushLoopOffset = 41;
constexpr size_t kBossRushStateOffset = 42;
constexpr char kIntroSkipMagic[] = "DUSKSKP1";
constexpr char kBossRushMagic[] = "DUSKBR1";

constexpr char kIntroSkipStage[] = "F_SP108";
constexpr s8 kIntroSkipRoom = 0;

struct BossRushEntry {
    const char* stage;
    s16 point;
    s8 room;
    s8 layer;
    int saveTable;
    enum ClearMode {
        Boss,
        MiddleBoss,
        FinalSequence,
        BeastGanon,
        FinalGanondorf,
    } clearMode;
    const char* displayName;
    bool runSequence;
};

constexpr BossRushEntry kBossRushEntries[] = {
    {"D_MN05B", 0, 51, 0, dStage_SaveTbl_LV1, BossRushEntry::MiddleBoss, "Ook", true},
    {"D_MN05A", 0, 50, 0, dStage_SaveTbl_LV1, BossRushEntry::Boss, "Diababa", true},
    {"D_MN04B", 0, 51, 0, dStage_SaveTbl_LV2, BossRushEntry::MiddleBoss, "Dangoro", true},
    {"D_MN04A", 0, 50, 0, dStage_SaveTbl_LV2, BossRushEntry::Boss, "Fyrus", true},
    {"D_MN01B", 0, 51, 0, dStage_SaveTbl_LV3, BossRushEntry::MiddleBoss, "Deku Toad", true},
    {"D_MN01A", 0, 50, 0, dStage_SaveTbl_LV3, BossRushEntry::Boss, "Morpheel", true},
    {"D_MN10B", 0, 51, 0, dStage_SaveTbl_LV4, BossRushEntry::MiddleBoss, "Death Sword", true},
    {"D_MN10A", 0, 50, 0, dStage_SaveTbl_LV4, BossRushEntry::Boss, "Stallord", true},
    {"D_MN11B", 0, 51, 0, dStage_SaveTbl_LV5, BossRushEntry::MiddleBoss, "Darkhammer", true},
    {"D_MN11A", 0, 50, 0, dStage_SaveTbl_LV5, BossRushEntry::Boss, "Blizzeta", true},
    {"D_MN06B", 0, 51, 0, dStage_SaveTbl_LV6, BossRushEntry::MiddleBoss, "Darknut", true},
    {"D_MN06A", 0, 50, 0, dStage_SaveTbl_LV6, BossRushEntry::Boss, "Armogohma", true},
    {"D_MN07B", 0, 51, 0, dStage_SaveTbl_LV7, BossRushEntry::MiddleBoss, "Aeralfos", true},
    {"D_MN07A", 0, 50, 0, dStage_SaveTbl_LV7, BossRushEntry::Boss, "Argorok", true},
    {"D_MN08D", 0, 50, 0, dStage_SaveTbl_LV8, BossRushEntry::Boss, "Zant", true},
    {"D_MN09A", 0, 50, 0, dStage_SaveTbl_LV9, BossRushEntry::FinalSequence, "Puppet Zelda", true},
    {"D_MN09A", 0, 50, 0, dStage_SaveTbl_LV9, BossRushEntry::BeastGanon, "Beast Ganon", false},
    {"D_MN09C", 0, 0, 0, dStage_SaveTbl_LV9, BossRushEntry::FinalGanondorf, "Ganondorf", false},
};

constexpr size_t kBossRushEntryCount = std::size(kBossRushEntries);
constexpr const char* kBossRushRunName = "Boss Rush";
constexpr const char* kBossRushGameModeId = "bossrush";
constexpr s8 kBossRushReturnRoom = 0;
constexpr char kBossRushReturnStage[] = "D_MN09C";
constexpr s16 kBossRushReturnPoint = 0;
constexpr s8 kBossRushReturnLayer = 0;
constexpr u8 kBossRushStateHub = 0;
constexpr u8 kBossRushStateRun = 1;
constexpr u8 kBossRushStateReplay = 2;
constexpr u8 kBossRushCenterPortalIndex = static_cast<u8>(kBossRushEntryCount);
constexpr u8 kBossRushHubPortalCount = kBossRushCenterPortalIndex + 1;
constexpr u8 kBossRushHubPortalCreateBatch = 1;
constexpr u8 kBossRushHubWarpSceneListNo = 0;
constexpr f32 kBossRushHubY = 1100.0f;
constexpr f32 kBossRushHubPortalRadius = 1450.0f;
constexpr f32 kBossRushHubTriggerRadius = 150.0f;
constexpr s16 kGanondorfFacingAngle = 0x37FE;
constexpr s8 kFinalPuppetRoom = 50;
constexpr s8 kFinalBeastRoom = 51;
constexpr s16 kGanondorfActionWait = 10;
constexpr s16 kGanondorfActionDown = 21;
constexpr s16 kGanondorfActionEnd = 22;
constexpr int kGanondorfIntroCam = 92;
constexpr int kGanondorfIntroCamAfterBarrierSpawn = 2;
constexpr int kGanondorfEndDemoStart = 60;
constexpr int kDirectFinalBarrierOnSwitch = 15;
constexpr int kDirectFinalBarrierOffSwitch = 31;
constexpr s16 kDirectFinalBarrierAngleX =
    static_cast<s16>((kDirectFinalBarrierOffSwitch << 8) | kDirectFinalBarrierOnSwitch);
constexpr int kBossRushHazardIntervalFrames = 600;
constexpr int kBossRushHazardProjectileCount = 3;
constexpr f32 kBossRushHazardSpawnRadius = 1300.0f;
constexpr f32 kDirectFinalHazardSpawnRadius = 700.0f;
constexpr f32 kBossRushHazardSpawnHeight = 160.0f;
constexpr f32 kBossRushHazardAimHeight = 80.0f;
constexpr f32 kBossRushHazardProjectileSpeed = 35.0f;
constexpr f32 kBossRushHazardHitRadius = 90.0f;
constexpr f32 kBossRushHazardImpactScale = 0.75f;
constexpr int kBossRushHazardLifetimeFrames = 180;
constexpr int kBossRushHazardDamage = 4;
constexpr int kBossRushHazardHitCooldownFrames = 60;
constexpr int kBossRushHazardGuardCooldownFrames = 20;
constexpr int kBossRushTriangleInitialDelayFrames = 300;
constexpr int kBossRushTriangleIntervalFrames = 600;
constexpr int kBossRushTriangleDurationFrames = 90;
constexpr int kBossRushTriangleStrikeFrame = 60;
constexpr int kBossRushTriangleDamageStartFrame = 60;
constexpr int kBossRushTriangleDamageEndFrame = 90;
constexpr f32 kBossRushTriangleGroundOffset = 5.0f;
constexpr f32 kBossRushTriangleSize = 8.0f;
constexpr f32 kBossRushTriangleSqrt3Half = 0.8660254f;

#if VERSION == VERSION_GCN_PAL
constexpr size_t kNameSceneFileSelectOffset = 0x43C;
#else
constexpr size_t kNameSceneFileSelectOffset = 0x414;
#endif

struct DataNewRestore {
    dFile_select_c* fileSelect = nullptr;
    u8 slot = 0xff;
    u8 value = 0;
};

fpc_ProcID sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
bool sAdvancePending = false;
DataNewRestore sDataNewRestore;
fpc_ProcID sHubBarrierId = fpcM_ERROR_PROCESS_ID_e;
fpc_ProcID sHubPortalIds[kBossRushHubPortalCount];
fpc_ProcID sDirectFinalBossId = fpcM_ERROR_PROCESS_ID_e;
fpc_ProcID sDirectFinalBarrierId = fpcM_ERROR_PROCESS_ID_e;
int sDirectFinalBossIndex = -1;
bool sDirectFinalGanondorfStarted = false;
int sDirectFinalGanondorfReadyFrames = 0;
bool sDirectFinalSceneTransitionStarted = false;
bool sHubActorIdsInitialized = false;
bool sHubActorsSpawned = false;
bool sHubPortalsArmed = false;
u8 sHubNextPortalToSpawn = 0;
UiDialogHandle sHubConfirmDialog = 0;
int sPendingHubPortal = -1;
int sDismissedHubPortal = -1;
char sHubConfirmTitle[64] = {};
char sHubConfirmBody[128] = {};
char sHubMidnaPromptText[64] = {};
char const sMidnaHubWarpOptionText[] = "Garden of Twilight";
char const sMidnaYesText[] = "Yes";
char const sMidnaNoText[] = "No";
char const sMidnaEmptyText[] = "";
bool sMidnaHubWarpMenuOffered = false;
bool sHubPortalMidnaPromptOffered = false;
int sBossRushHazardTimer = kBossRushHazardIntervalFrames;
int sBossRushHazardHitCooldown = 0;
u32 sBossRushHazardWave = 0;
int sBossRushTriangleTimer = kBossRushTriangleInitialDelayFrames;
u32 sBossRushTriangleWave = 0;
bool sBossRushGameModeActive = false;
bool sBossRushHooksInstalled = false;

struct BossRushElectricOrb {
    bool active = false;
    cXyz pos;
    cXyz velocity;
    int timer = 0;
    u32 emitterKeys[3] = {};
};

BossRushElectricOrb sBossRushElectricOrbs[kBossRushHazardProjectileCount];

struct BossRushTriangleHazard {
    bool active = false;
    bool impactSpawned = false;
    bool damageApplied = false;
    cXyz pos;
    s16 rotY = 0;
    int frame = 0;
};

BossRushTriangleHazard sBossRushTriangleHazard;

using CreateActor7Fn =
    fpc_ProcID (*)(s16, u32, const cXyz*, int, const csXyz*, const cXyz*, s8);
using CreateActor8Fn =
    fpc_ProcID (*)(s16, u32, const cXyz*, int, const csXyz*, const cXyz*, s8, u32);

CreateActor7Fn sCreateActor7 = nullptr;
CreateActor8Fn sCreateActor8 = nullptr;
bool sCreateActorResolveAttempted = false;

void resolve_create_actor() {
    if (sCreateActorResolveAttempted) {
        return;
    }
    sCreateActorResolveAttempted = true;

    void* symbol = nullptr;
#if defined(_WIN32)
    if (svc_hook->resolve(
            mod_ctx, "?fopAcM_create@@YAIFIPEBUcXyz@@HPEBVcsXyz@@0CI@Z", &symbol,
            nullptr) == MOD_OK)
    {
        sCreateActor8 = reinterpret_cast<CreateActor8Fn>(symbol);
        return;
    }
    if (svc_hook->resolve(
            mod_ctx, "?fopAcM_create@@YAIFIPEBUcXyz@@HPEBVcsXyz@@0C@Z", &symbol,
            nullptr) == MOD_OK)
    {
        sCreateActor7 = reinterpret_cast<CreateActor7Fn>(symbol);
        return;
    }
#elif defined(__ANDROID__)
    if (svc_hook->resolve(mod_ctx, "_Z13fopAcM_createsjPK4cXyziPK5csXyzS1_a", &symbol,
            nullptr) == MOD_OK)
    {
        sCreateActor7 = reinterpret_cast<CreateActor7Fn>(symbol);
        return;
    }
    if (svc_hook->resolve(mod_ctx, "_Z13fopAcM_createsjPK4cXyziPK5csXyzS1_aj", &symbol,
            nullptr) == MOD_OK)
    {
        sCreateActor8 = reinterpret_cast<CreateActor8Fn>(symbol);
        return;
    }
#else
    if (svc_hook->resolve(mod_ctx, "_Z13fopAcM_createsjPK4cXyziPK5csXyzS1_aj", &symbol,
            nullptr) == MOD_OK)
    {
        sCreateActor8 = reinterpret_cast<CreateActor8Fn>(symbol);
        return;
    }
    if (svc_hook->resolve(mod_ctx, "_Z13fopAcM_createsjPK4cXyziPK5csXyzS1_a", &symbol,
            nullptr) == MOD_OK)
    {
        sCreateActor7 = reinterpret_cast<CreateActor7Fn>(symbol);
    }
#endif
}

fpc_ProcID create_actor(s16 procName, u32 parameters, const cXyz* pos, int roomNo,
    const csXyz* angle, const cXyz* scale, s8 argument) {
    resolve_create_actor();

    if (sCreateActor8 != nullptr) {
        return sCreateActor8(procName, parameters, pos, roomNo, angle, scale, argument, 0);
    }

    if (sCreateActor7 != nullptr) {
        return sCreateActor7(procName, parameters, pos, roomNo, angle, scale, argument);
    }

    return fpcM_ERROR_PROCESS_ID_e;
}

bool is_bossrush_game_mode_active() {
    return sBossRushGameModeActive;
}

bool can_update_bossrush_gameplay() {
    return dComIfGp_getPlayer(0) != nullptr && dComIfGp_getStageStagInfo() != nullptr;
}

u8* reserve_bytes(dSv_save_c* save) {
    return save == nullptr ? nullptr : reinterpret_cast<u8*>(save) + kReserveOffset;
}

const u8* reserve_bytes(const dSv_save_c* save) {
    return save == nullptr ? nullptr : reinterpret_cast<const u8*>(save) + kReserveOffset;
}

void write_marker(dSv_save_c* save, size_t offset, const char* magic, size_t length, bool enabled) {
    u8* reserve = reserve_bytes(save);
    if (reserve == nullptr) {
        return;
    }
    if (enabled) {
        std::memcpy(reserve + offset, magic, length);
    } else {
        std::memset(reserve + offset, 0, length);
    }
}

bool is_boss_rush(const dSv_save_c* save) {
    const u8* reserve = reserve_bytes(save);
    return reserve != nullptr &&
           std::memcmp(reserve + kBossRushOffset, kBossRushMagic, sizeof(kBossRushMagic) - 1) == 0;
}

void set_intro_skipped(dSv_save_c* save, bool enabled) {
    write_marker(save, kIntroSkipOffset, kIntroSkipMagic, sizeof(kIntroSkipMagic) - 1, enabled);
}

void set_boss_rush(dSv_save_c* save, bool enabled) {
    write_marker(save, kBossRushOffset, kBossRushMagic, sizeof(kBossRushMagic) - 1, enabled);
    if (!enabled) {
        u8* reserve = reserve_bytes(save);
        if (reserve != nullptr) {
            reserve[kBossRushIndexOffset] = 0;
            reserve[kBossRushLoopOffset] = 0;
        }
    }
}

u8 boss_rush_index() {
    dSv_save_c* save = dComIfGs_getSaveData();
    u8* reserve = reserve_bytes(save);
    if (reserve == nullptr || !is_boss_rush(save)) {
        return 0;
    }
    if (reserve[kBossRushIndexOffset] >= kBossRushEntryCount) {
        reserve[kBossRushIndexOffset] = 0;
    }
    return reserve[kBossRushIndexOffset];
}

void set_boss_rush_index(u8 index) {
    dSv_save_c* save = dComIfGs_getSaveData();
    u8* reserve = reserve_bytes(save);
    if (reserve != nullptr && is_boss_rush(save)) {
        reserve[kBossRushIndexOffset] = index;
    }
}

u8 boss_rush_state() {
    dSv_save_c* save = dComIfGs_getSaveData();
    u8* reserve = reserve_bytes(save);
    if (reserve == nullptr || !is_boss_rush(save)) {
        return kBossRushStateHub;
    }
    return reserve[kBossRushStateOffset];
}

void set_boss_rush_state(u8 state) {
    dSv_save_c* save = dComIfGs_getSaveData();
    u8* reserve = reserve_bytes(save);
    if (reserve != nullptr && is_boss_rush(save)) {
        reserve[kBossRushStateOffset] = state;
    }
}

f32 angle_sin(s16 angle) {
    return std::sin(static_cast<f32>(angle) * (6.2831853071795864769f / 65536.0f));
}

f32 angle_cos(s16 angle) {
    return std::cos(static_cast<f32>(angle) * (6.2831853071795864769f / 65536.0f));
}

cXyz hub_center() {
    return cXyz(0.0f, kBossRushHubY, 0.0f);
}

cXyz hub_portal_position(u8 portal) {
    cXyz pos = hub_center();
    if (portal < kBossRushCenterPortalIndex) {
        const s16 angle = static_cast<s16>((0x10000 * portal) / kBossRushCenterPortalIndex);
        pos.x += angle_sin(angle) * kBossRushHubPortalRadius;
        pos.z += angle_cos(angle) * kBossRushHubPortalRadius;
    }
    return pos;
}

csXyz hub_portal_rotation(u8 portal) {
    if (portal >= kBossRushCenterPortalIndex) {
        return csXyz(0, 0, 0);
    }
    const s16 angle = static_cast<s16>((0x10000 * portal) / kBossRushCenterPortalIndex);
    return csXyz(0, angle, 0);
}

bool is_boss_hub_stage_name() {
    return std::strcmp(dComIfGp_getStartStageName(), kBossRushReturnStage) == 0 &&
           dComIfGp_getStartStageRoomNo() == kBossRushReturnRoom;
}

bool is_bossrush_hub_active() {
    return is_boss_rush(dComIfGs_getSaveData()) && boss_rush_state() == kBossRushStateHub &&
           is_boss_hub_stage_name();
}

bool is_opening_stage(const char* stage, s16 point, s16 room, s16 layer) {
    return stage != nullptr && std::strcmp(stage, "F_SP102") == 0 && point == 100 &&
           room == 0 && layer == 10;
}

bool is_reset_to_opening_transition() {
    if (mDoRst::isReset() || mDoRst::isReturnToMenu()) {
        return true;
    }

    if (is_opening_stage(dComIfGp_getStartStageName(), dComIfGp_getStartStagePoint(),
            dComIfGp_getStartStageRoomNo(), dComIfGp_getStartStageLayer()))
    {
        return true;
    }

    return dComIfGp_isEnableNextStage() &&
           is_opening_stage(dComIfGp_getNextStageName(), dComIfGp_getNextStagePoint(),
               dComIfGp_getNextStageRoomNo(), dComIfGp_getNextStageLayer());
}

bool is_current_stage_name(const char* stage) {
    return std::strcmp(dComIfGp_getStartStageName(), stage) == 0;
}

void set_bossrush_return_place() {
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(
        kBossRushReturnStage, kBossRushReturnRoom, 0);
}

void reset_hub_actor_ids() {
    sHubBarrierId = fpcM_ERROR_PROCESS_ID_e;
    for (u8 i = 0; i < kBossRushHubPortalCount; i++) {
        sHubPortalIds[i] = fpcM_ERROR_PROCESS_ID_e;
    }
    sHubActorIdsInitialized = true;
    sHubActorsSpawned = false;
    sHubPortalsArmed = false;
    sHubNextPortalToSpawn = 0;
    sHubConfirmDialog = 0;
    sPendingHubPortal = -1;
    sDismissedHubPortal = -1;
}

void ensure_hub_actor_ids_initialized();
void arm_ganondorf_barrier(obj_gb_class* barrier);

void delete_hub_actor(fpc_ProcID id) {
    if (id != fpcM_ERROR_PROCESS_ID_e && fopAcM_SearchByID(id) != NULL) {
        fopAcM_delete(id);
    }
}

void delete_hub_actors() {
    ensure_hub_actor_ids_initialized();
    delete_hub_actor(sHubBarrierId);
    for (u8 i = 0; i < kBossRushHubPortalCount; i++) {
        delete_hub_actor(sHubPortalIds[i]);
    }
    reset_hub_actor_ids();
}

void reset_direct_final_boss_state() {
    delete_hub_actor(sDirectFinalBarrierId);
    sDirectFinalBossId = fpcM_ERROR_PROCESS_ID_e;
    sDirectFinalBarrierId = fpcM_ERROR_PROCESS_ID_e;
    sDirectFinalBossIndex = -1;
    sDirectFinalGanondorfStarted = false;
    sDirectFinalGanondorfReadyFrames = 0;
    sDirectFinalSceneTransitionStarted = false;
}

void ensure_hub_actor_ids_initialized() {
    if (!sHubActorIdsInitialized) {
        reset_hub_actor_ids();
    }
}

obj_gb_class* hub_barrier_actor() {
    if (sHubBarrierId == fpcM_ERROR_PROCESS_ID_e) {
        return NULL;
    }
    return static_cast<obj_gb_class*>(fopAcM_SearchByID(sHubBarrierId));
}

void spawn_hub_actors() {
    ensure_hub_actor_ids_initialized();
    if (sHubActorsSpawned) {
        bool actorsAlive = sHubBarrierId != fpcM_ERROR_PROCESS_ID_e &&
                           fopAcM_SearchByID(sHubBarrierId) != NULL;
        for (u8 i = 0; actorsAlive && i < kBossRushHubPortalCount; i++) {
            actorsAlive = sHubPortalIds[i] != fpcM_ERROR_PROCESS_ID_e &&
                          fopAcM_SearchByID(sHubPortalIds[i]) != NULL;
        }

        if (actorsAlive) {
            arm_ganondorf_barrier(hub_barrier_actor());
            return;
        }

        reset_hub_actor_ids();
    }

    cXyz center = hub_center();
    csXyz barrierAngle(kDirectFinalBarrierAngleX, 0, 0);
    if (sHubBarrierId == fpcM_ERROR_PROCESS_ID_e ||
        fopAcM_SearchByID(sHubBarrierId) == NULL)
    {
        dComIfGs_onOneZoneSwitch(kDirectFinalBarrierOnSwitch, kBossRushReturnRoom);
        dComIfGs_offOneZoneSwitch(kDirectFinalBarrierOffSwitch, kBossRushReturnRoom);
        sHubBarrierId = create_actor(
            fpcNm_OBJ_GB_e, 0xF0069600, &center, kBossRushReturnRoom, &barrierAngle, NULL, -1);
        if (sHubBarrierId == fpcM_ERROR_PROCESS_ID_e) {
            return;
        }
    }

    u8 portalsCreated = 0;
    while (sHubNextPortalToSpawn < kBossRushHubPortalCount &&
           portalsCreated < kBossRushHubPortalCreateBatch)
    {
        const u8 portal = sHubNextPortalToSpawn;
        cXyz pos = hub_portal_position(portal);
        csXyz rot = hub_portal_rotation(portal);
        const fpc_ProcID portalId = fopAcM_createWarpHole(
            &pos, &rot, kBossRushReturnRoom, kBossRushHubWarpSceneListNo, 0, 0xff);
        if (portalId == fpcM_ERROR_PROCESS_ID_e) {
            return;
        }
        sHubPortalIds[portal] = portalId;
        sHubNextPortalToSpawn++;
        portalsCreated++;
    }

    if (sHubNextPortalToSpawn < kBossRushHubPortalCount) {
        arm_ganondorf_barrier(hub_barrier_actor());
        return;
    }

    arm_ganondorf_barrier(hub_barrier_actor());
    sHubActorsSpawned = true;
}

int touched_hub_portal() {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == NULL) {
        return -1;
    }

    for (u8 i = 0; i < kBossRushHubPortalCount; i++) {
        cXyz pos = hub_portal_position(i);
        f32 distXZ = player->current.pos.absXZ(pos);
        f32 distY = player->current.pos.y - pos.y;
        if (distXZ < kBossRushHubTriggerRadius && distY < 200.0f && distY > -100.0f) {
            return i;
        }
    }

    return -1;
}

void reset_hub_runtime_when_away() {
    if (!is_boss_hub_stage_name()) {
        sHubActorsSpawned = false;
        sHubPortalsArmed = false;
        sHubConfirmDialog = 0;
        sPendingHubPortal = -1;
        sDismissedHubPortal = -1;
    }
}

void increment_boss_rush_loop() {
    dSv_save_c* save = dComIfGs_getSaveData();
    u8* reserve = reserve_bytes(save);
    if (reserve != nullptr && is_boss_rush(save) && reserve[kBossRushLoopOffset] < 0xff) {
        reserve[kBossRushLoopOffset]++;
    }
}

bool is_intro_skip_bottle_item(u8 item) {
    return item >= dItemNo_EMPTY_BOTTLE_e && item <= dItemNo_DROP_BOTTLE_e;
}

bool has_intro_skip_item(u8 item) {
    if (dComIfGs_isItemFirstBit(item)) {
        return true;
    }

    for (int i = 0; i < MAX_ITEM_SLOTS; i++) {
        if (dComIfGs_getItem(i, false) == item) {
            return true;
        }
    }
    return false;
}

bool has_intro_skip_bottle() {
    for (int i = SLOT_11; i <= SLOT_14; i++) {
        if (is_intro_skip_bottle_item(dComIfGs_getItem(i, false))) {
            return true;
        }
    }
    return false;
}

void ensure_intro_skip_item(int slot, u8 item) {
    if (!has_intro_skip_item(item)) {
        dComIfGs_setItem(slot, item);
    }
    dComIfGs_onItemFirstBit(item);
}

void set_select_item_if_empty(int select, u8 slot) {
    if (dComIfGs_getSelectItemIndex(select) == dItemNo_NONE_e) {
        dComIfGs_setMixItemIndex(select, dItemNo_NONE_e);
        dComIfGs_setSelectItemIndex(select, slot);
    }
}

void repair_intro_skip_faron_tears(dSv_save_c* save) {
    static constexpr u8 kFaronTearTboxes[] = {
        0, 1, 4, 5, 6, 8, 9, 11, 12, 13, 14, 17, 18, 20, 21, 23,
    };
    dSv_memBit_c& faron = save->getSave(dStage_SaveTbl_FARON).getBit();
    for (u8 tbox : kFaronTearTboxes) {
        faron.onTbox(tbox);
    }
}

void apply_intro_skip_preset(dSv_save_c* save) {
    if (save == nullptr) {
        return;
    }

    static constexpr u16 kIntroEventBits[] = {
        dSv_event_flag_c::D_0001,
        dSv_event_flag_c::F_0008,
        dSv_event_flag_c::F_0010,
        dSv_event_flag_c::F_0014,
        dSv_event_flag_c::F_0015,
        dSv_event_flag_c::F_0019,
        dSv_event_flag_c::F_0023,
        dSv_event_flag_c::F_0024,
        dSv_event_flag_c::F_0025,
        dSv_event_flag_c::F_0026,
        dSv_event_flag_c::F_0027,
        dSv_event_flag_c::F_0032,
        dSv_event_flag_c::F_0036,
        dSv_event_flag_c::F_0037,
        dSv_event_flag_c::F_0038,
        dSv_event_flag_c::F_0044,
        dSv_event_flag_c::F_0046,
        dSv_event_flag_c::F_0051,
        dSv_event_flag_c::F_0053,
        dSv_event_flag_c::F_0055,
        dSv_event_flag_c::F_0067,
        dSv_event_flag_c::F_0069,
        dSv_event_flag_c::F_0072,
        dSv_event_flag_c::F_0085,
        dSv_event_flag_c::F_0094,
        dSv_event_flag_c::F_0205,
        dSv_event_flag_c::F_0207,
        dSv_event_flag_c::F_0208,
        dSv_event_flag_c::F_0211,
        dSv_event_flag_c::F_0215,
        dSv_event_flag_c::F_0220,
        dSv_event_flag_c::F_0223,
        dSv_event_flag_c::F_0345,
        dSv_event_flag_c::F_0364,
        dSv_event_flag_c::F_0550,
        dSv_event_flag_c::F_0565,
        dSv_event_flag_c::F_0573,
        dSv_event_flag_c::F_0577,
        dSv_event_flag_c::F_0580,
        dSv_event_flag_c::F_0581,
        dSv_event_flag_c::F_0582,
        dSv_event_flag_c::F_0583,
        dSv_event_flag_c::F_0585,
        dSv_event_flag_c::F_0600,
        dSv_event_flag_c::F_0608,
        dSv_event_flag_c::F_0611,
        dSv_event_flag_c::F_0614,
        dSv_event_flag_c::F_0625,
        dSv_event_flag_c::F_0630,
        dSv_event_flag_c::F_0651,
        dSv_event_flag_c::F_0700,
        dSv_event_flag_c::F_0701,
        dSv_event_flag_c::F_0702,
        dSv_event_flag_c::F_0748,
        dSv_event_flag_c::M_001,
        dSv_event_flag_c::M_002,
        dSv_event_flag_c::M_008,
        dSv_event_flag_c::M_009,
        dSv_event_flag_c::M_010,
        dSv_event_flag_c::M_011,
        dSv_event_flag_c::M_012,
        dSv_event_flag_c::M_013,
        dSv_event_flag_c::M_014,
        dSv_event_flag_c::M_015,
        dSv_event_flag_c::M_016,
        dSv_event_flag_c::M_017,
        dSv_event_flag_c::M_019,
        dSv_event_flag_c::M_067,
        dSv_event_flag_c::M_068,
        dSv_event_flag_c::M_072,
        dSv_event_flag_c::M_079,
        dSv_event_flag_c::M_095,
    };

    static constexpr u8 kOrdonSwitches[] = {
        1, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 17, 19, 20, 23, 24, 25, 26,
        27, 28, 32, 33, 34, 35, 37, 41, 42, 43, 45, 47, 64, 66, 67, 68,
        73, 84, 88, 95, 101, 103, 104, 119,
    };
    static constexpr u8 kPrisonSwitches[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 33,
        34, 35, 36, 37,
    };
    static constexpr u8 kFaronSwitches[] = {
        2, 3, 5, 6, 8, 9, 13, 14, 15, 16, 19, 20, 21, 22, 23, 24, 30, 33,
        34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 62, 64, 67, 70, 71, 73, 75, 76,
        79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
        111,
    };

    for (u16 bit : kIntroEventBits) {
        dComIfGs_onEventBit(bit);
    }
    for (u8 sw : kOrdonSwitches) {
        save->getSave(dStage_SaveTbl_ORDON).getBit().onSwitch(sw);
    }
    for (u8 sw : kPrisonSwitches) {
        save->getSave(dStage_SaveTbl_PRISON).getBit().onSwitch(sw);
    }
    for (u8 sw : kFaronSwitches) {
        save->getSave(dStage_SaveTbl_FARON).getBit().onSwitch(sw);
    }

    dComIfGs_onTransformLV(0);
    dComIfGs_onDarkClearLV(0);
    dComIfGs_offSaveSwitch(dStage_SaveTbl_FARON, 12);
    dComIfGs_setTransformStatus(TF_STATUS_HUMAN);
    dComIfGs_setLightDropNum(FARON_VESSEL, 16);
    dComIfGs_onLightDropGetFlag(FARON_VESSEL);

    cXyz horsePos(-2030.36511f, 242.41f, -9671.569f);
    save->getPlayer().getHorsePlace().set("F_SP104", horsePos, 0, 1);

    dComIfGs_onItemFirstBit(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_setCollectClothes(KOKIRI_CLOTHES_FLAG);
    if (dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_CASUAL_e ||
        dComIfGs_getSelectEquipClothes() == dItemNo_NONE_e)
    {
        dComIfGs_setSelectEquipClothes(dItemNo_WEAR_KOKIRI_e);
    }

    dComIfGs_onItemFirstBit(dItemNo_SWORD_e);
    dComIfGs_setCollectSword(COLLECT_ORDON_SWORD);
    dComIfGs_setCollectSword(COLLECT_WOODEN_SWORD);
    if (!dComIfGs_isItemFirstBit(dItemNo_MASTER_SWORD_e) &&
        dComIfGs_getSelectEquipSword() == dItemNo_NONE_e)
    {
        dComIfGs_setSelectEquipSword(dItemNo_SWORD_e);
    }

    dComIfGs_onItemFirstBit(dItemNo_WOOD_SHIELD_e);
    dComIfGs_setCollectShield(COLLECT_WOODEN_SHIELD);
    if (!dComIfGs_isItemFirstBit(dItemNo_HYLIA_SHIELD_e) &&
        dComIfGs_getSelectEquipShield() == dItemNo_NONE_e)
    {
        dComIfGs_setSelectEquipShield(dItemNo_WOOD_SHIELD_e);
    }

    ensure_intro_skip_item(SLOT_1, dItemNo_KANTERA_e);
    ensure_intro_skip_item(SLOT_20, dItemNo_FISHING_ROD_1_e);
    ensure_intro_skip_item(SLOT_23, dItemNo_PACHINKO_e);

    if (!has_intro_skip_bottle()) {
        dComIfGs_setItem(SLOT_11, dItemNo_HALF_MILK_BOTTLE_e);
        dComIfGs_onItemFirstBit(dItemNo_HALF_MILK_BOTTLE_e);
    }

    if (dComIfGs_getMaxOil() < 21600) {
        dComIfGs_setMaxOil(21600);
    }
    if (dComIfGs_getOil() < 21600) {
        dComIfGs_setOil(21600);
    }

    dComIfGs_setPachinkoNum(dComIfGs_getPachinkoMax());
    set_select_item_if_empty(SELECT_ITEM_X, SLOT_1);
    set_select_item_if_empty(SELECT_ITEM_Y, SLOT_23);

    if (dComIfGs_getMaxLife() < 16) {
        dComIfGs_setMaxLife(16);
        dComIfGs_setLife(16);
    }
    if (dComIfGs_getRupee() < 100) {
        dComIfGs_setRupee(100);
    }

    save->getPlayer().getPlayerReturnPlace().set(kIntroSkipStage, kIntroSkipRoom, 0);
    set_intro_skipped(save, true);
    set_boss_rush(save, false);
    repair_intro_skip_faron_tears(save);
    dComIfGs_setLineUpItem();
}

void set_item(int slot, u8 item) {
    dComIfGs_setItem(slot, item);
    if (item != dItemNo_NONE_e) {
        dComIfGs_onItemFirstBit(item);
    }
}

void set_select_item(int select, u8 slot) {
    dComIfGs_setMixItemIndex(select, dItemNo_NONE_e);
    dComIfGs_setSelectItemIndex(select, slot);
}

void clear_boss_flags(const BossRushEntry& entry) {
    dComIfGs_getSaveData()->getSave(entry.saveTable).getBit().init();

    if (std::strcmp(dComIfGp_getStartStageName(), entry.stage) == 0 &&
        dComIfGp_getStartStageRoomNo() == entry.room)
    {
        for (int i = 0; i < dSv_info_c::MEMORY_SWITCH + dSv_info_c::DAN_SWITCH; i++) {
            dComIfGs_offSwitch(i, entry.room);
        }

        if (entry.clearMode == BossRushEntry::MiddleBoss) {
            dComIfGs_offStageMiddleBoss();
        } else {
            dComIfGs_offStageBossEnemy();
            dComIfGs_offStageLife();
            dComIfGs_offStageBossDemo();
        }
    }
}

void clear_all_boss_flags() {
    for (const BossRushEntry& entry : kBossRushEntries) {
        clear_boss_flags(entry);
    }
}

void set_saved_dungeon_switch(int saveTable, int sw, bool enabled) {
    dSv_memBit_c& bit = dComIfGs_getSaveData()->getSave(saveTable).getBit();
    if (enabled) {
        bit.onSwitch(sw);
    } else {
        bit.offSwitch(sw);
    }
}

void ensure_bossrush_story_state() {
    dComIfGs_onEventBit(dSv_event_flag_c::F_0250);
}

void grant_bossrush_items() {
    for (int i = 0; i < MAX_ITEM_SLOTS; i++) {
        dComIfGs_setItem(i, dItemNo_NONE_e);
    }

    set_item(SLOT_0, dItemNo_BOOMERANG_e);
    set_item(SLOT_1, dItemNo_KANTERA_e);
    set_item(SLOT_2, dItemNo_SPINNER_e);
    set_item(SLOT_3, dItemNo_HVY_BOOTS_e);
    set_item(SLOT_4, dItemNo_BOW_e);
    set_item(SLOT_5, dItemNo_HAWK_EYE_e);
    set_item(SLOT_6, dItemNo_IRONBALL_e);
    set_item(SLOT_8, dItemNo_COPY_ROD_e);
    set_item(SLOT_10, dItemNo_W_HOOKSHOT_e);
    set_item(SLOT_11, dItemNo_BLUE_BOTTLE_e);
    set_item(SLOT_12, dItemNo_RED_BOTTLE_e);
    set_item(SLOT_13, dItemNo_FAIRY_e);
    set_item(SLOT_14, dItemNo_EMPTY_BOTTLE_e);
    set_item(SLOT_15, dItemNo_NORMAL_BOMB_e);
    set_item(SLOT_16, dItemNo_WATER_BOMB_e);
    set_item(SLOT_17, dItemNo_POKE_BOMB_e);
    set_item(SLOT_20, dItemNo_FISHING_ROD_1_e);
    set_item(SLOT_23, dItemNo_PACHINKO_e);

    dComIfGs_onItemFirstBit(dItemNo_SWORD_e);
    dComIfGs_onItemFirstBit(dItemNo_MASTER_SWORD_e);
    dComIfGs_onItemFirstBit(dItemNo_HYLIA_SHIELD_e);
    dComIfGs_onItemFirstBit(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_onItemFirstBit(dItemNo_ARMOR_e);
    dComIfGs_onItemFirstBit(dItemNo_WEAR_ZORA_e);
    dComIfGs_onItemFirstBit(dItemNo_WALLET_LV3_e);

    dComIfGs_setSelectEquipSword(dItemNo_MASTER_SWORD_e);
    dComIfGs_setSelectEquipShield(dItemNo_HYLIA_SHIELD_e);
    dComIfGs_setSelectEquipClothes(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_setBButtonItemKey(dItemNo_SWORD_e);
    dComIfGs_setCollectSword(COLLECT_MASTER_SWORD);
    dComIfGs_setCollectShield(COLLECT_HYLIAN_SHIELD);
    dComIfGs_setCollectClothes(KOKIRI_CLOTHES_FLAG);

    for (int i = 0; i < MAX_SELECT_ITEM; i++) {
        dComIfGs_setMixItemIndex(i, dItemNo_NONE_e);
        dComIfGs_setSelectItemIndex(i, dItemNo_NONE_e);
    }
    set_select_item(SELECT_ITEM_X, SLOT_4);
    set_select_item(SELECT_ITEM_Y, SLOT_10);

    dComIfGs_setArrowMax(60);
    dComIfGs_setArrowNum(60);
    dComIfGs_setPachinkoNum(dComIfGs_getPachinkoMax());
    dComIfGs_setBombMax(dItemNo_NORMAL_BOMB_e, 30);
    dComIfGs_setBombMax(dItemNo_WATER_BOMB_e, 15);
    dComIfGs_setBombMax(dItemNo_POKE_BOMB_e, 10);
    dComIfGs_setBombNum(0, 30);
    dComIfGs_setBombNum(1, 15);
    dComIfGs_setBombNum(2, 10);

    dComIfGs_setWalletSize(GIANT_WALLET);
    dComIfGs_setRupee(GIANT_WALLET_MAX);
    dComIfGs_setMaxOil(21600);
    dComIfGs_setOil(21600);
    dComIfGs_setMaxMagic(0);
    dComIfGs_setMagic(0);
    dComIfGs_setRodTypeLevelUp();

    dComIfGs_onTransformLV(0);
    dComIfGs_onTransformLV(1);
    dComIfGs_onTransformLV(2);
    dComIfGs_onTransformLV(3);
    dComIfGs_onDarkClearLV(0);
    dComIfGs_onDarkClearLV(1);
    dComIfGs_onDarkClearLV(2);
    dComIfGs_setTransformStatus(TF_STATUS_HUMAN);

    static constexpr u16 kUtilityEventBits[] = {
        dSv_event_flag_c::F_0250,
        dSv_event_flag_c::F_0339,
        dSv_event_flag_c::F_0338,
        dSv_event_flag_c::F_0340,
        dSv_event_flag_c::F_0341,
        dSv_event_flag_c::F_0342,
        dSv_event_flag_c::F_0343,
        dSv_event_flag_c::F_0344,
        dSv_event_flag_c::F_0550,
        dSv_event_flag_c::M_067,
        dSv_event_flag_c::M_068,
        dSv_event_flag_c::M_077,
    };
    for (u16 bit : kUtilityEventBits) {
        dComIfGs_onEventBit(bit);
    }

    dComIfGs_setLineUpItem();
}

void apply_boss_rush_preset(dSv_save_c* save) {
    if (save == nullptr) {
        return;
    }

    set_intro_skipped(save, false);
    set_boss_rush(save, true);
    set_boss_rush_index(0);
    u8* reserve = reserve_bytes(save);
    if (reserve != nullptr) {
        reserve[kBossRushLoopOffset] = 0;
        reserve[kBossRushStateOffset] = kBossRushStateHub;
    }

    dComIfGs_setMaxLife(25);
    dComIfGs_setLife(20);
    ensure_bossrush_story_state();
    grant_bossrush_items();
    clear_all_boss_flags();
    save->getPlayer().getPlayerReturnPlace().set(kBossRushReturnStage, kBossRushReturnRoom, 0);
}

bool is_current_stage(const BossRushEntry& entry) {
    return std::strcmp(dComIfGp_getStartStageName(), entry.stage) == 0 &&
           dComIfGp_getStartStageRoomNo() == entry.room;
}

bool is_current_direct_boss_stage(const BossRushEntry& entry) {
    if (entry.clearMode == BossRushEntry::FinalGanondorf) {
        return is_current_stage_name(entry.stage);
    }
    return is_current_stage(entry);
}

bool is_current_save_table(int saveTable) {
    stage_stag_info_class* stagInfo = dComIfGp_getStageStagInfo();
    return stagInfo != nullptr && saveTable == dStage_stagInfo_GetSaveTbl(stagInfo);
}

void clear_final_ganondorf_setup_switches() {
    set_saved_dungeon_switch(dStage_SaveTbl_LV9, 1, false);
    set_saved_dungeon_switch(dStage_SaveTbl_LV9, 2, false);
    dComIfGs_offSaveDunSwitch(1);
    dComIfGs_offSaveDunSwitch(2);
}

bool boss_is_cleared(const BossRushEntry& entry) {
    if (entry.clearMode == BossRushEntry::FinalSequence ||
        entry.clearMode == BossRushEntry::BeastGanon ||
        entry.clearMode == BossRushEntry::FinalGanondorf)
    {
        return false;
    }

    if (is_current_save_table(entry.saveTable)) {
        return entry.clearMode == BossRushEntry::MiddleBoss ? dComIfGs_isStageMiddleBoss() :
                                                              dComIfGs_isStageBossEnemy();
    }

    dSv_memBit_c& bit = dComIfGs_getSaveData()->getSave(entry.saveTable).getBit();
    return entry.clearMode == BossRushEntry::MiddleBoss ? bit.isStageBossEnemy2() :
                                                          bit.isStageBossEnemy();
}

void prepare_final_battle_state(const BossRushEntry& entry) {
    if (entry.clearMode == BossRushEntry::BeastGanon) {
        dComIfGs_setTransformStatus(TF_STATUS_WOLF);
    } else if (entry.clearMode == BossRushEntry::FinalSequence ||
               entry.clearMode == BossRushEntry::FinalGanondorf)
    {
        dComIfGs_setTransformStatus(TF_STATUS_HUMAN);
    }

    clear_final_ganondorf_setup_switches();
}

void prepare_bossrush_entry(const BossRushEntry& entry) {
    ensure_bossrush_story_state();
    clear_boss_flags(entry);

    if (entry.saveTable == dStage_SaveTbl_LV9) {
        prepare_final_battle_state(entry);
    }
}

bool direct_final_boss_request_active() {
    return sDirectFinalBossId != fpcM_ERROR_PROCESS_ID_e &&
           (fpcM_IsCreating(sDirectFinalBossId) || fopAcM_IsExecuting(sDirectFinalBossId));
}

bool is_direct_final_ganondorf_active();

void start_beast_ganon_transition_if_ready() {
    if (sDirectFinalSceneTransitionStarted || dComIfGp_isEnableNextStage() || fopOvlpM_IsPeek() ||
        dComIfGp_event_runCheck() || !is_current_stage_name("D_MN09A") ||
        dComIfGp_getStartStageRoomNo() != kFinalPuppetRoom)
    {
        return;
    }

    sDirectFinalSceneTransitionStarted = true;
    dStage_changeScene(1, 0.0f, 0, kFinalPuppetRoom, 0, -1);
}

void create_final_ganondorf_if_needed(const BossRushEntry& entry) {
    if (fopAcM_SearchByName(fpcNm_B_GND_e) != NULL || direct_final_boss_request_active()) {
        return;
    }

    cXyz bossPos(-600.0f, kBossRushHubY, 0.0f);
    csXyz bossAngle(0, kGanondorfFacingAngle, 0);
    sDirectFinalBossId =
        create_actor(fpcNm_B_GND_e, 0, &bossPos, entry.room, &bossAngle, NULL, -1);
}

bool direct_final_barrier_active() {
    if (sDirectFinalBarrierId != fpcM_ERROR_PROCESS_ID_e &&
        (fpcM_IsCreating(sDirectFinalBarrierId) || fopAcM_IsExecuting(sDirectFinalBarrierId)))
    {
        return true;
    }

    obj_gb_class* barrier = static_cast<obj_gb_class*>(fopAcM_SearchByName(fpcNm_OBJ_GB_e));
    if (barrier != NULL) {
        sDirectFinalBarrierId = fopAcM_GetID(static_cast<fopAc_ac_c*>(barrier));
        return true;
    }

    sDirectFinalBarrierId = fpcM_ERROR_PROCESS_ID_e;
    return false;
}

bool should_force_ganondorf_barrier_visible() {
    return is_bossrush_hub_active() || is_direct_final_ganondorf_active();
}

void arm_ganondorf_barrier(obj_gb_class* barrier) {
    if (barrier == NULL || !should_force_ganondorf_barrier_visible()) {
        return;
    }

    const int roomNo = fopAcM_GetRoomNo(static_cast<fopAc_ac_c*>(barrier));
    dComIfGs_onOneZoneSwitch(kDirectFinalBarrierOnSwitch, roomNo);
    dComIfGs_offOneZoneSwitch(kDirectFinalBarrierOffSwitch, roomNo);
    barrier->mSw1 = kDirectFinalBarrierOnSwitch;
    barrier->mSw2 = kDirectFinalBarrierOffSwitch;
    barrier->mIsFinalBattle = 0;
    barrier->mBrkFrame = 29.0f;
    barrier->mColorAlpha = 0xF0;
    barrier->scale.x = 1.5f;
    barrier->scale.y = 1.0f;
}

void create_direct_final_barrier_if_needed(b_gnd_class* ganondorf) {
    if (ganondorf == NULL || direct_final_barrier_active()) {
        return;
    }

    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(ganondorf);
    cXyz barrierPos(0.0f, kBossRushHubY, 0.0f);
    csXyz barrierAngle(kDirectFinalBarrierAngleX, 0, 0);
    sDirectFinalBarrierId = create_actor(
        fpcNm_OBJ_GB_e, 0xF0069600, &barrierPos, fopAcM_GetRoomNo(actor), &barrierAngle, NULL,
        -1);
    dComIfGs_onOneZoneSwitch(kDirectFinalBarrierOnSwitch, fopAcM_GetRoomNo(actor));
    dComIfGs_offOneZoneSwitch(kDirectFinalBarrierOffSwitch, fopAcM_GetRoomNo(actor));
}

bool is_direct_final_ganondorf_active() {
    if (!is_boss_rush(dComIfGs_getSaveData()) || boss_rush_state() != kBossRushStateReplay) {
        return false;
    }

    const u8 index = boss_rush_index();
    if (index >= kBossRushEntryCount) {
        return false;
    }

    const BossRushEntry& entry = kBossRushEntries[index];
    return entry.clearMode == BossRushEntry::FinalGanondorf &&
           is_current_direct_boss_stage(entry);
}

bool force_final_ganondorf_ground_start(b_gnd_class* ganondorf) {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (ganondorf == NULL || player == NULL) {
        sDirectFinalGanondorfReadyFrames = 0;
        return false;
    }

    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(ganondorf);
    if (!fopAcM_IsExecuting(fopAcM_GetID(actor))) {
        sDirectFinalGanondorfReadyFrames = 0;
        return false;
    }

    mant_class* mant = static_cast<mant_class*>(fopAcM_SearchByID(ganondorf->mMantChildID));
    if (mant == NULL) {
        sDirectFinalGanondorfReadyFrames = 0;
        return false;
    }

    if (sDirectFinalGanondorfReadyFrames < 2) {
        sDirectFinalGanondorfReadyFrames++;
        return false;
    }

    player->onForceHorseGetOff();

    actor->current.pos.set(-600.0f, kBossRushHubY, 0.0f);
    actor->old.pos = actor->current.pos;
    actor->speed.zero();
    actor->speedF = 0.0f;
    actor->shape_angle.x = actor->current.angle.x = 0;
    actor->shape_angle.y = actor->current.angle.y = kGanondorfFacingAngle;
    actor->shape_angle.z = actor->current.angle.z = 0;
    actor->health = 100;
    ganondorf->mNoDrawTimer = 0;
    ganondorf->mActionMode = kGanondorfActionWait;
    ganondorf->mMoveMode = 0;
    ganondorf->mDrawHorse = FALSE;
    ganondorf->mDemoCamMode = kGanondorfIntroCam;
    ganondorf->mDemoCamTimer = kGanondorfIntroCamAfterBarrierSpawn;
    ganondorf->mDamageInvulnerabilityTimer = 0;
    ganondorf->field_0x1e08 = 0;
    ganondorf->field_0x1e0a = 0;
    ganondorf->field_0xc44[0] = 30;
    ganondorf->field_0xc44[8] = 100;
    ganondorf->mGakeChkType = 0;
    ganondorf->field_0xc7d = 1;
    ganondorf->field_0x2740 = 0;
    ganondorf->field_0x2710.x = 55.0f;
    mant->field_0x3969 = 1;
    create_direct_final_barrier_if_needed(ganondorf);
    Z2GetAudioMgr()->bgmStart(Z2BGM_VS_GANON_04, 0, 0);

    sDirectFinalGanondorfStarted = true;
    return true;
}

void start_final_ganondorf_if_ready() {
    b_gnd_class* ganondorf = static_cast<b_gnd_class*>(fopAcM_SearchByName(fpcNm_B_GND_e));
    if (ganondorf == NULL) {
        sDirectFinalGanondorfReadyFrames = 0;
        return;
    }

    if (!sDirectFinalGanondorfStarted || ganondorf->mDrawHorse ||
        ganondorf->mActionMode < kGanondorfActionWait)
    {
        force_final_ganondorf_ground_start(ganondorf);
    }
}

void ensure_direct_final_boss_started() {
    if (boss_rush_state() != kBossRushStateReplay) {
        reset_direct_final_boss_state();
        return;
    }

    const u8 index = boss_rush_index();
    if (index >= kBossRushEntryCount) {
        reset_direct_final_boss_state();
        return;
    }

    const BossRushEntry& entry = kBossRushEntries[index];
    if ((entry.clearMode != BossRushEntry::BeastGanon &&
         entry.clearMode != BossRushEntry::FinalGanondorf) ||
        !is_current_direct_boss_stage(entry))
    {
        return;
    }

    if (sDirectFinalBossIndex != index) {
        sDirectFinalBossIndex = index;
        sDirectFinalBossId = fpcM_ERROR_PROCESS_ID_e;
        sDirectFinalGanondorfStarted = false;
        sDirectFinalSceneTransitionStarted = false;
        prepare_final_battle_state(entry);
    }

    if (entry.clearMode == BossRushEntry::BeastGanon) {
        start_beast_ganon_transition_if_ready();
    } else {
        create_final_ganondorf_if_needed(entry);
        start_final_ganondorf_if_ready();
    }
}

void set_bossrush_next_stage() {
    if (!is_boss_rush(dComIfGs_getSaveData())) {
        return;
    }

    set_bossrush_return_place();

    if (boss_rush_state() == kBossRushStateHub) {
        dComIfGp_setNextStage(
            kBossRushReturnStage, kBossRushReturnPoint, kBossRushReturnRoom, kBossRushReturnLayer);
        return;
    }

    const BossRushEntry& entry = kBossRushEntries[boss_rush_index()];
    prepare_bossrush_entry(entry);
    dComIfGp_setNextStage(entry.stage, entry.point, entry.room, entry.layer);
}

void grant_victory_heart() {
    const u16 maxLife = dComIfGs_getMaxLife();
    if (maxLife < 100) {
        dComIfGs_setMaxLife(static_cast<u8>(maxLife + 5));
    }
    dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
}

void advance_bossrush() {
    u8 index = boss_rush_index();
    do {
        index++;
    } while (index < kBossRushEntryCount && !kBossRushEntries[index].runSequence);

    if (index >= kBossRushEntryCount) {
        index = 0;
        increment_boss_rush_loop();
        set_boss_rush_state(kBossRushStateHub);
    } else {
        set_boss_rush_state(kBossRushStateRun);
    }
    set_boss_rush_index(index);
}

bool can_open_save_prompt() {
    return !dComIfGp_event_runCheck() && !fopOvlpM_IsPeek() && !dComIfGp_isEnableNextStage() &&
           dMeter2Info_getGameOverType() == 0;
}

bool ui_document_visible() {
    bool visible = false;
    return svc_ui->is_any_document_visible(mod_ctx, &visible) == MOD_OK && visible;
}

bool can_offer_midna_hub_warp() {
    return is_boss_rush(dComIfGs_getSaveData()) && boss_rush_state() != kBossRushStateHub &&
           !is_boss_hub_stage_name() && !fopOvlpM_IsPeek() && !dComIfGp_isEnableNextStage() &&
           dMeter2Info_getGameOverType() == 0 && dComIfGp_getGameoverStatus() == 0;
}

bool has_hub_portal_midna_prompt() {
    return is_boss_rush(dComIfGs_getSaveData()) && boss_rush_state() == kBossRushStateHub &&
           is_boss_hub_stage_name() && sPendingHubPortal >= 0 &&
           sPendingHubPortal < static_cast<int>(kBossRushHubPortalCount);
}

void set_hub_portal_midna_meter_prompt() {
    if (!has_hub_portal_midna_prompt()) {
        return;
    }

    dComIfGp_setZStatus(BUTTON_STATUS_CHECK, BUTTON_STATUS_FLAG_EMPHASIS);
    dMeter2Info_onUseButton(METER2_USEBUTTON_Z);
}

bool is_midna_menu_message() {
    dMsgObject_c* msg = dMsgObject_getMsgObjectClass();
    if (msg == NULL || msg->mpRenProc == NULL || msg->getFukiKind() != 13) {
        return false;
    }

    auto* ref = const_cast<jmessage_tReference*>(
        static_cast<const jmessage_tReference*>(msg->mpRenProc->getReference()));
    if (ref == NULL || ref->getSelectNum() < 2) {
        return false;
    }

    const u16 msgId = ref->getMsgID();
    return msgId == 0x7d3 || msgId == 0x7f6;
}

bool is_midna_actor(fopAc_ac_c* actor) {
    return actor != NULL && fopAcM_GetName(actor) == fpcNm_MIDNA_e;
}

void reset_bossrush_warp_audio() {
    Z2SeqMgr* seqMgr = Z2GetSeqMgr();
    if (seqMgr == NULL) {
        return;
    }

    seqMgr->mFanfareID.setAnonymous();
    seqMgr->mFanfareCount = 0;
    seqMgr->setBattleBgmOff(true);
    seqMgr->resetBattleBgmParams();
    seqMgr->bgmAllUnMute(0);
}

void prepare_midna_hub_warp_item() {
    set_boss_rush_state(kBossRushStateHub);
    set_boss_rush_index(0);
    set_bossrush_return_place();
    reset_bossrush_warp_audio();
    dComIfGs_setWarpItemData(kBossRushReturnStage, hub_center(), 0, kBossRushReturnRoom, 0, 1);
    dComIfGs_setItem(SLOT_18, dItemNo_DUNGEON_BACK_e);
}

void close_midna_custom_dialog(daMidna_c* midna) {
    dMsgObject_onKillMessageFlag();

    if (midna != NULL) {
        dComIfGp_getEvent()->reset(midna);
        midna->offStateFlg0(daMidna_c::FLG0_UNK_8000);
    } else {
        dComIfGp_event_reset();
    }
}

void warp_to_bossrush_hub_from_midna(daMidna_c* midna) {
    reset_direct_final_boss_state();
    delete_hub_actors();
    prepare_midna_hub_warp_item();
    close_midna_custom_dialog(midna);

    daAlink_c* player = daAlink_getAlinkActorClass();
    if (player != NULL && player->procDungeonWarpReadyInit()) {
        return;
    }

    dComIfGp_setNextStage(kBossRushReturnStage, kBossRushReturnPoint, kBossRushReturnRoom,
        kBossRushReturnLayer);
}

const char* bossrush_portal_name(int portal) {
    if (portal == kBossRushCenterPortalIndex) {
        return kBossRushRunName;
    }
    if (portal >= 0 && portal < static_cast<int>(kBossRushEntryCount)) {
        return kBossRushEntries[portal].displayName;
    }
    return "Boss";
}

void start_bossrush_entry(int portal) {
    reset_direct_final_boss_state();
    delete_hub_actors();

    if (portal == kBossRushCenterPortalIndex) {
        set_boss_rush_state(kBossRushStateRun);
        set_boss_rush_index(0);
        u8* reserve = reserve_bytes(dComIfGs_getSaveData());
        if (reserve != nullptr) {
            reserve[kBossRushLoopOffset] = 0;
        }
        clear_all_boss_flags();
        set_bossrush_next_stage();
        return;
    }

    if (portal >= 0 && portal < static_cast<int>(kBossRushEntryCount)) {
        set_boss_rush_state(kBossRushStateReplay);
        set_boss_rush_index(static_cast<u8>(portal));
        clear_boss_flags(kBossRushEntries[portal]);
        set_bossrush_next_stage();
    }
}

void clear_hub_confirm_state() {
    sHubConfirmDialog = 0;
    sPendingHubPortal = -1;
    sHubPortalMidnaPromptOffered = false;
}

void on_hub_confirm_yes(ModContext*, UiDialogHandle, void*) {
    const int portal = sPendingHubPortal;
    clear_hub_confirm_state();
    sHubPortalsArmed = false;

    if (portal >= 0 && is_boss_rush(dComIfGs_getSaveData()) &&
        boss_rush_state() == kBossRushStateHub && is_boss_hub_stage_name())
    {
        start_bossrush_entry(portal);
    }
}

void on_hub_confirm_no(ModContext*, UiDialogHandle, void*) {
    if (sPendingHubPortal >= 0) {
        sDismissedHubPortal = sPendingHubPortal;
    }
    clear_hub_confirm_state();
    sHubPortalsArmed = false;
}

void on_hub_confirm_dismiss(ModContext*, UiDialogHandle, void*) {
    if (sPendingHubPortal >= 0) {
        sDismissedHubPortal = sPendingHubPortal;
    }
    clear_hub_confirm_state();
    sHubPortalsArmed = false;
}

bool open_bossrush_confirm_dialog(int portal) {
    static UiDialogAction actions[] = {
        {"Yes", on_hub_confirm_yes, nullptr, false},
        {"No", on_hub_confirm_no, nullptr, false},
    };

    std::snprintf(sHubConfirmTitle, sizeof(sHubConfirmTitle), "Fight %s?", bossrush_portal_name(portal));
    std::snprintf(
        sHubConfirmBody, sizeof(sHubConfirmBody), "<p>Start %s?</p>", bossrush_portal_name(portal));

    UiDialogDesc desc = UI_DIALOG_DESC_INIT;
    desc.title = sHubConfirmTitle;
    desc.body_rml = sHubConfirmBody;
    desc.variant = UI_DIALOG_NORMAL;
    desc.icon = "question-mark";
    desc.actions = actions;
    desc.action_count = std::size(actions);
    desc.on_dismiss = on_hub_confirm_dismiss;

    sPendingHubPortal = portal;
    const ModResult result = svc_ui->dialog_push(mod_ctx, &desc, &sHubConfirmDialog);
    if (result != MOD_OK) {
        sPendingHubPortal = -1;
        svc_log->warn(mod_ctx, "Dawnlight Boss Rush: failed to open portal confirmation");
        return false;
    }

    return true;
}

void set_hub_midna_prompt_portal(int portal) {
    if (portal < 0 || portal >= static_cast<int>(kBossRushHubPortalCount)) {
        clear_hub_confirm_state();
        return;
    }

    sPendingHubPortal = portal;
    std::snprintf(sHubMidnaPromptText, sizeof(sHubMidnaPromptText), "Fight %s?",
        bossrush_portal_name(portal));
}

void resolve_hub_midna_prompt(bool accepted) {
    const int portal = sPendingHubPortal;
    clear_hub_confirm_state();
    sHubPortalsArmed = false;
    close_midna_custom_dialog(daPy_py_c::getMidnaActor());

    if (!accepted) {
        sDismissedHubPortal = portal;
        return;
    }

    if (portal >= 0 && is_boss_rush(dComIfGs_getSaveData()) &&
        boss_rush_state() == kBossRushStateHub && is_boss_hub_stage_name())
    {
        start_bossrush_entry(portal);
    }
}

void update_bossrush_hub() {
    sAdvancePending = false;
    sSavePromptId = fpcM_ERROR_PROCESS_ID_e;

    if (is_reset_to_opening_transition()) {
        reset_hub_actor_ids();
        clear_hub_confirm_state();
        return;
    }

    set_bossrush_return_place();

    if (!is_boss_hub_stage_name()) {
        if (!dComIfGp_isEnableNextStage()) {
            dComIfGp_setNextStage(
                kBossRushReturnStage, kBossRushReturnPoint, kBossRushReturnRoom, kBossRushReturnLayer);
        }
        return;
    }

    spawn_hub_actors();
    arm_ganondorf_barrier(hub_barrier_actor());

    int portal = touched_hub_portal();
    if (portal < 0) {
        sHubPortalsArmed = true;
        sDismissedHubPortal = -1;
        clear_hub_confirm_state();
        return;
    }

    if (portal == sDismissedHubPortal || !sHubPortalsArmed || !can_open_save_prompt() ||
        ui_document_visible())
    {
        return;
    }

    sHubPortalsArmed = false;
    set_hub_midna_prompt_portal(portal);
}

void finish_prompt_and_advance() {
    if (dComIfGp_getGameoverStatus() == 1) {
        d_GameOver_Delete(sSavePromptId);
        dComIfGp_setGameoverStatus(0);
        sAdvancePending = false;
        set_bossrush_next_stage();
    }
}

bool restore_bossrush_hub_load_state() {
    if (!is_boss_hub_stage_name() || boss_rush_state() == kBossRushStateHub ||
        dComIfGp_isEnableNextStage() || fopOvlpM_IsPeek())
    {
        return false;
    }

    const u8 index = boss_rush_index();
    if (boss_rush_state() == kBossRushStateRun && index < kBossRushEntryCount &&
        kBossRushEntries[index].runSequence)
    {
        sAdvancePending = false;
        sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
        reset_direct_final_boss_state();
        reset_hub_actor_ids();
        clear_hub_confirm_state();
        set_bossrush_next_stage();
        return true;
    }

    if (index < kBossRushEntryCount &&
        kBossRushEntries[index].clearMode == BossRushEntry::FinalGanondorf)
    {
        return false;
    }

    sAdvancePending = false;
    sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
    set_boss_rush_state(kBossRushStateHub);
    set_bossrush_return_place();
    reset_direct_final_boss_state();
    reset_hub_actor_ids();
    clear_hub_confirm_state();
    update_bossrush_hub();
    return true;
}

bool bossrush_hazards_can_run() {
    if (!bossrush_hardmode_hazards_enabled() || !is_boss_rush(dComIfGs_getSaveData()) ||
        boss_rush_state() == kBossRushStateHub || dComIfGs_getLife() == 0 ||
        sSavePromptId != fpcM_ERROR_PROCESS_ID_e || sAdvancePending || fopOvlpM_IsPeek() ||
        dComIfGp_isEnableNextStage() || dMeter2Info_getGameOverType() != 0 ||
        dComIfGp_getGameoverStatus() != 0 || dComIfGp_isPauseFlag() || ui_document_visible())
    {
        return false;
    }

    const u8 index = boss_rush_index();
    if (index >= kBossRushEntryCount) {
        return false;
    }

    const BossRushEntry& entry = kBossRushEntries[index];
    if (!is_current_direct_boss_stage(entry)) {
        return false;
    }

    if (entry.clearMode != BossRushEntry::FinalGanondorf) {
        return false;
    }

    b_gnd_class* ganondorf = static_cast<b_gnd_class*>(fopAcM_SearchByName(fpcNm_B_GND_e));
    return sDirectFinalGanondorfStarted && ganondorf != NULL && ganondorf->mDemoCamMode == 0 &&
           !dComIfGp_event_runCheck() && !ganondorf->mDrawHorse &&
           ganondorf->mActionMode >= kGanondorfActionWait &&
           ganondorf->mActionMode < kGanondorfActionEnd;
}

f32 bossrush_hazard_spawn_radius() {
    const u8 index = boss_rush_index();
    if (index < kBossRushEntryCount &&
        kBossRushEntries[index].clearMode == BossRushEntry::FinalGanondorf &&
        is_current_direct_boss_stage(kBossRushEntries[index]))
    {
        return kDirectFinalHazardSpawnRadius;
    }

    return kBossRushHazardSpawnRadius;
}

s16 bossrush_hazard_ring_angle(u32 wave, int slot) {
    return static_cast<s16>(
        static_cast<s16>(wave * 0x2345) + (0x10000 / kBossRushHazardProjectileCount) * slot);
}

cXyz bossrush_hazard_ring_position(daPy_py_c* player, f32 yOffset, u32 wave, int slot) {
    const s16 angle = bossrush_hazard_ring_angle(wave, slot);
    const f32 spawnRadius = bossrush_hazard_spawn_radius();
    return cXyz(
        player->current.pos.x + cM_ssin(angle) * spawnRadius,
        player->current.pos.y + yOffset,
        player->current.pos.z + cM_scos(angle) * spawnRadius);
}

void stop_bossrush_electric_orb(BossRushElectricOrb& orb) {
    for (u32& emitterKey : orb.emitterKeys) {
        if (emitterKey == 0) {
            continue;
        }

        JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(emitterKey);
        if (emitter != NULL) {
            emitter->deleteAllParticle();
            dComIfGp_particle_levelEmitterOnEventMove(emitterKey);
        }

        emitterKey = 0;
    }

    orb.active = false;
    orb.timer = 0;
}

void stop_bossrush_triangle_hazard(BossRushTriangleHazard& triangle) {
    triangle = BossRushTriangleHazard();
}

void reset_bossrush_triangle_hazard() {
    sBossRushTriangleTimer = kBossRushTriangleInitialDelayFrames;
    stop_bossrush_triangle_hazard(sBossRushTriangleHazard);
}

void play_bossrush_hazard_sound(JAISoundID soundId, const cXyz& pos) {
    Z2AudioMgr* audioMgr = Z2GetAudioMgr();
    if (audioMgr != NULL) {
        audioMgr->seStart(soundId, &pos, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }
}

void play_bossrush_hazard_level_sound(JAISoundID soundId, const cXyz& pos) {
    Z2AudioMgr* audioMgr = Z2GetAudioMgr();
    if (audioMgr != NULL) {
        audioMgr->seStartLevel(soundId, &pos, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }
}

void spawn_bossrush_electric_impact(const cXyz& pos) {
    static constexpr u16 kImpactEffects[] = {0x8915, 0x8916, 0x8917};
    const cXyz scale(
        kBossRushHazardImpactScale, kBossRushHazardImpactScale, kBossRushHazardImpactScale);

    for (u16 effect : kImpactEffects) {
        dComIfGp_particle_set(effect, &pos, NULL, &scale);
    }
}

void reset_bossrush_hazards() {
    sBossRushHazardTimer = kBossRushHazardIntervalFrames;
    sBossRushHazardHitCooldown = 0;
    reset_bossrush_triangle_hazard();

    for (BossRushElectricOrb& orb : sBossRushElectricOrbs) {
        stop_bossrush_electric_orb(orb);
    }
}

void apply_bossrush_electric_hit() {
    daPy_py_c* player = daPy_getPlayerActorClass();
    daAlink_c* alink = daAlink_getAlinkActorClass();
    if (player == nullptr || alink == nullptr || sBossRushHazardHitCooldown > 0) {
        return;
    }

    if (player->checkPlayerGuard()) {
        dComIfGp_getVibration().StartShock(VIBMODE_S_POWER3, 0x1F, cXyz(0.0f, 1.0f, 0.0f));
        sBossRushHazardHitCooldown = kBossRushHazardGuardCooldownFrames;
        return;
    }

    dComIfGp_getVibration().StartShock(VIBMODE_S_POWER4, 0x1F, cXyz(0.0f, 1.0f, 0.0f));
    alink->setDamagePointNormal(kBossRushHazardDamage);
    sBossRushHazardHitCooldown = kBossRushHazardHitCooldownFrames;
}

void spawn_bossrush_hazard_projectile(int slot, const cXyz& spawn, const cXyz& target) {
    if (slot < 0 || slot >= kBossRushHazardProjectileCount) {
        return;
    }

    cXyz delta = target - spawn;
    const f32 distance = delta.abs();
    if (distance < 1.0f) {
        return;
    }

    BossRushElectricOrb& orb = sBossRushElectricOrbs[slot];
    stop_bossrush_electric_orb(orb);
    orb.active = true;
    orb.pos = spawn;
    orb.velocity = delta * (kBossRushHazardProjectileSpeed / distance);
    orb.timer = kBossRushHazardLifetimeFrames;

    play_bossrush_hazard_sound(Z2SE_EN_ZAN_FIRE_OUT, orb.pos);
}

void spawn_bossrush_hazard_wave() {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == nullptr) {
        return;
    }

    const cXyz target(
        player->current.pos.x,
        player->current.pos.y + kBossRushHazardAimHeight,
        player->current.pos.z);

    for (int i = 0; i < kBossRushHazardProjectileCount; ++i) {
        cXyz spawn =
            bossrush_hazard_ring_position(player, kBossRushHazardSpawnHeight, sBossRushHazardWave, i);
        spawn_bossrush_hazard_projectile(i, spawn, target);
    }

    ++sBossRushHazardWave;
}

void spawn_bossrush_triangle_effect(const BossRushTriangleHazard& triangle) {
    static constexpr u16 kTriangleEffects[] = {0x8945, 0x8946, 0x8947, 0x8948, 0x8949};
    csXyz rot(0, static_cast<s16>(triangle.rotY + 0x8000), 0);
    const cXyz scale(kBossRushTriangleSize, kBossRushTriangleSize, kBossRushTriangleSize);

    for (u16 effect : kTriangleEffects) {
        dComIfGp_particle_set(effect, &triangle.pos, &rot, &scale);
    }

    play_bossrush_hazard_sound(Z2SE_EN_HZE_ATK_B_LIGHTWALL, triangle.pos);
}

void make_bossrush_triangle_points(const BossRushTriangleHazard& triangle, cXyz* points) {
    const f32 inRadius = 50.0f * kBossRushTriangleSize;
    const f32 circumRadius = inRadius * 2.0f;
    const cXyz localPoints[] = {
        cXyz(-kBossRushTriangleSqrt3Half * circumRadius, 0.0f, inRadius),
        cXyz(kBossRushTriangleSqrt3Half * circumRadius, 0.0f, inRadius),
        cXyz(0.0f, 0.0f, -circumRadius),
    };

    const f32 sinY = cM_ssin(triangle.rotY);
    const f32 cosY = cM_scos(triangle.rotY);
    for (int i = 0; i < 3; ++i) {
        points[i].x = triangle.pos.x + localPoints[i].x * cosY - localPoints[i].z * sinY;
        points[i].y = triangle.pos.y;
        points[i].z = triangle.pos.z + localPoints[i].x * sinY + localPoints[i].z * cosY;
    }
}

f32 bossrush_triangle_edge(const cXyz& a, const cXyz& b, const cXyz& p) {
    return (p.x - a.x) * (b.z - a.z) - (p.z - a.z) * (b.x - a.x);
}

void draw_bossrush_triangle_warning() {
    if (!sBossRushTriangleHazard.active ||
        sBossRushTriangleHazard.frame >= kBossRushTriangleStrikeFrame)
    {
        return;
    }

    cXyz points[3];
    make_bossrush_triangle_points(sBossRushTriangleHazard, points);
    const f32 progress =
        static_cast<f32>(sBossRushTriangleHazard.frame) /
        static_cast<f32>(kBossRushTriangleStrikeFrame);
    const u8 alpha = static_cast<u8>(0x50 + (0x40 * std::clamp(progress, 0.0f, 1.0f)));
    const GXColor color = {0xFF, 0xD8, 0x34, alpha};
    dDbVw_drawTriangleXlu(points, color, TRUE);
}

void spawn_bossrush_triangle_hazard() {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == nullptr) {
        return;
    }

    sBossRushTriangleHazard.active = true;
    sBossRushTriangleHazard.impactSpawned = false;
    sBossRushTriangleHazard.damageApplied = false;
    sBossRushTriangleHazard.frame = 0;
    sBossRushTriangleHazard.pos = cXyz(
        player->current.pos.x,
        player->current.pos.y + kBossRushTriangleGroundOffset,
        player->current.pos.z);
    sBossRushTriangleHazard.rotY = bossrush_hazard_ring_angle(sBossRushTriangleWave, 0);
    play_bossrush_hazard_sound(Z2SE_EN_HZE_ATK_B_LIGHT, sBossRushTriangleHazard.pos);

    ++sBossRushTriangleWave;
}

bool player_in_bossrush_triangle(const BossRushTriangleHazard& triangle) {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == nullptr) {
        return false;
    }

    cXyz points[3];
    make_bossrush_triangle_points(triangle, points);
    cXyz playerPos(player->current.pos.x, 0.0f, player->current.pos.z);
    const f32 e0 = bossrush_triangle_edge(points[0], points[1], playerPos);
    const f32 e1 = bossrush_triangle_edge(points[1], points[2], playerPos);
    const f32 e2 = bossrush_triangle_edge(points[2], points[0], playerPos);
    const bool hasNegative = e0 < 0.0f || e1 < 0.0f || e2 < 0.0f;
    const bool hasPositive = e0 > 0.0f || e1 > 0.0f || e2 > 0.0f;
    return !(hasNegative && hasPositive);
}

void update_bossrush_triangle_hazard() {
    if (sBossRushTriangleHazard.active) {
        ++sBossRushTriangleHazard.frame;

        if (!sBossRushTriangleHazard.impactSpawned &&
            sBossRushTriangleHazard.frame >= kBossRushTriangleStrikeFrame)
        {
            spawn_bossrush_triangle_effect(sBossRushTriangleHazard);
            sBossRushTriangleHazard.impactSpawned = true;
        }

        if (!sBossRushTriangleHazard.damageApplied &&
            sBossRushTriangleHazard.frame >= kBossRushTriangleDamageStartFrame &&
            sBossRushTriangleHazard.frame <= kBossRushTriangleDamageEndFrame &&
            player_in_bossrush_triangle(sBossRushTriangleHazard))
        {
            apply_bossrush_electric_hit();
            sBossRushTriangleHazard.damageApplied = true;
        }

        if (sBossRushTriangleHazard.frame >= kBossRushTriangleDurationFrames) {
            stop_bossrush_triangle_hazard(sBossRushTriangleHazard);
        }
    }

    if (sBossRushTriangleTimer > 0) {
        --sBossRushTriangleTimer;
        return;
    }

    spawn_bossrush_triangle_hazard();
    sBossRushTriangleTimer = kBossRushTriangleIntervalFrames;
}

void update_bossrush_electric_orbs() {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == nullptr) {
        return;
    }

    static constexpr u16 kFlightEffects[] = {0x8918, 0x8919, 0x891A};
    const cXyz playerTarget(
        player->current.pos.x,
        player->current.pos.y + kBossRushHazardAimHeight,
        player->current.pos.z);

    for (BossRushElectricOrb& orb : sBossRushElectricOrbs) {
        if (!orb.active) {
            continue;
        }

        orb.pos += orb.velocity;
        play_bossrush_hazard_level_sound(Z2SE_EN_ZAN_FIRE, orb.pos);
        for (size_t i = 0; i < std::size(kFlightEffects); ++i) {
            orb.emitterKeys[i] =
                dComIfGp_particle_set(orb.emitterKeys[i], kFlightEffects[i], &orb.pos, NULL, NULL);
        }

        --orb.timer;
        if (orb.pos.abs(playerTarget) <= kBossRushHazardHitRadius) {
            play_bossrush_hazard_sound(Z2SE_EN_ZAN_FIRE_BURST, orb.pos);
            spawn_bossrush_electric_impact(orb.pos);
            apply_bossrush_electric_hit();
            stop_bossrush_electric_orb(orb);
        } else if (orb.timer <= 0) {
            stop_bossrush_electric_orb(orb);
        }
    }
}

void update_bossrush_hazards() {
    if (!bossrush_hazards_can_run()) {
        reset_bossrush_hazards();
        return;
    }

    if (sBossRushHazardHitCooldown > 0) {
        --sBossRushHazardHitCooldown;
    }

    update_bossrush_electric_orbs();
    update_bossrush_triangle_hazard();

    if (sBossRushHazardTimer > 0) {
        --sBossRushHazardTimer;
        return;
    }

    spawn_bossrush_hazard_wave();
    sBossRushHazardTimer = kBossRushHazardIntervalFrames;
}

bool bossrush_should_return_to_hub_after_death() {
    if (dComIfGp_isEnableNextStage()) {
        return false;
    }

    daAlink_c* player = daAlink_getAlinkActorClass();
    return player != nullptr && player->checkDeadHP();
}

void update_bossrush() {
    if (!is_boss_rush(dComIfGs_getSaveData())) {
        sAdvancePending = false;
        sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
        reset_hub_actor_ids();
        reset_direct_final_boss_state();
        reset_bossrush_hazards();
        return;
    }

    if (is_reset_to_opening_transition()) {
        sAdvancePending = false;
        sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
        reset_hub_actor_ids();
        clear_hub_confirm_state();
        reset_bossrush_hazards();
        return;
    }

    if (restore_bossrush_hub_load_state()) {
        return;
    }

    if (boss_rush_state() == kBossRushStateHub) {
        reset_direct_final_boss_state();
        reset_bossrush_hazards();
        update_bossrush_hub();
        return;
    }

    reset_hub_runtime_when_away();
    ensure_direct_final_boss_started();
    update_bossrush_hazards();

    if (bossrush_should_return_to_hub_after_death()) {
        set_boss_rush_state(kBossRushStateHub);
        set_boss_rush_index(0);
        set_bossrush_return_place();
        reset_direct_final_boss_state();
        return;
    }

    if (sSavePromptId != fpcM_ERROR_PROCESS_ID_e) {
        if (d_GameOver_CheckDelete(sSavePromptId)) {
            finish_prompt_and_advance();
        }
        return;
    }

    if (sAdvancePending) {
        if (can_open_save_prompt()) {
            sSavePromptId = d_GameOver_Create(1);
        }
        return;
    }

    const BossRushEntry& entry = kBossRushEntries[boss_rush_index()];
    if (is_current_stage(entry) && boss_is_cleared(entry)) {
        if (boss_rush_state() == kBossRushStateReplay) {
            clear_boss_flags(entry);
            set_boss_rush_state(kBossRushStateHub);
            set_boss_rush_index(0);
            set_bossrush_next_stage();
            return;
        }

        grant_victory_heart();
        advance_bossrush();
        sAdvancePending = true;
    }
}

void clear_dawnlight_new_save_markers(dSv_save_c* save) {
    set_intro_skipped(save, false);
    set_boss_rush(save, false);
}

dFile_select_c* name_scene_file_select(void* nameScene) {
    if (nameScene == nullptr) {
        return nullptr;
    }
    auto** fileSelect = reinterpret_cast<dFile_select_c**>(
        reinterpret_cast<u8*>(nameScene) + kNameSceneFileSelectOffset);
    return fileSelect == nullptr ? nullptr : *fileSelect;
}

void apply_selected_new_save_mode() {
    if (is_bossrush_game_mode_active()) {
        return;
    }

    dSv_save_c* save = dComIfGs_getSaveData();
    clear_dawnlight_new_save_markers(save);

    switch (new_save_mode()) {
    case NewSaveMode::IntroSkip:
        apply_intro_skip_preset(save);
        svc_log->info(mod_ctx, "Dawnlight New Save Mode: Intro Skip applied");
        break;
    case NewSaveMode::Vanilla:
    default:
        break;
    }
}

void on_file_select_name_input2_post(ModContext*, void* args, void*, void*) {
    auto* fileSelect = mods::arg<dFile_select_c*>(args, 0);
    if (fileSelect == nullptr || !fileSelect->mIsSelectEnd ||
        fileSelect->mDataSelProc != dFile_select_c::DATASELPROC_NEXT_MODE_WAIT)
    {
        return;
    }

    apply_selected_new_save_mode();
}

void prepare_intro_skip_start() {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (!is_intro_skipped(save)) {
        return;
    }

    if (!dComIfGs_isEventBit(dSv_event_flag_c::F_0226)) {
        dComIfGs_offSaveSwitch(dStage_SaveTbl_FARON, 12);
    }
    dComIfGs_onSaveSwitch(dStage_SaveTbl_FARON, 20);
    save->getPlayer().getPlayerReturnPlace().set(kIntroSkipStage, kIntroSkipRoom, 0);
}

void prepare_bossrush_start() {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (!is_boss_rush(save)) {
        return;
    }

    save->getPlayer().getPlayerReturnPlace().set(kBossRushReturnStage, kBossRushReturnRoom, 0);
}

HookAction before_midna_select_string(ModContext*, void* args, void*, void*) {
    if (!is_midna_menu_message()) {
        return HOOK_CONTINUE;
    }

    if (has_hub_portal_midna_prompt()) {
        sHubPortalMidnaPromptOffered = true;
        mods::arg_ref<char DUSK_CONST*>(args, 1) = sMidnaEmptyText;
        mods::arg_ref<char DUSK_CONST*>(args, 2) = sMidnaYesText;
        mods::arg_ref<char DUSK_CONST*>(args, 3) = sMidnaNoText;
        return HOOK_CONTINUE;
    }

    if (!can_offer_midna_hub_warp()) {
        return HOOK_CONTINUE;
    }

    sMidnaHubWarpMenuOffered = true;
    mods::arg_ref<char DUSK_CONST*>(args, 3) = sMidnaHubWarpOptionText;
    return HOOK_CONTINUE;
}

HookAction before_midna_text_string(ModContext*, void* args, void*, void*) {
    if (!has_hub_portal_midna_prompt() || !is_midna_menu_message()) {
        return HOOK_CONTINUE;
    }

    sHubPortalMidnaPromptOffered = true;
    mods::arg_ref<char DUSK_CONST*>(args, 1) = sHubMidnaPromptText;
    mods::arg_ref<char DUSK_CONST*>(args, 2) = sHubMidnaPromptText;
    return HOOK_CONTINUE;
}

HookAction before_meter_execute(ModContext*, void*, void*, void*) {
    set_hub_portal_midna_meter_prompt();
    return HOOK_CONTINUE;
}

void after_midna_select_proc(ModContext*, void* args, void*, void*) {
    auto* msg = mods::arg<dMsgObject_c*>(args, 0);
    if (msg != nullptr && sHubPortalMidnaPromptOffered && has_hub_portal_midna_prompt() &&
        is_midna_menu_message())
    {
        if (msg->getSelectPushFlag() != 1) {
            if (msg->getSelectPushFlag() == 2) {
                resolve_hub_midna_prompt(false);
            }
            return;
        }

        resolve_hub_midna_prompt(msg->getSelectCursorPosLocal() == 0);
        return;
    }

    if (msg == nullptr || !sMidnaHubWarpMenuOffered || !can_offer_midna_hub_warp() ||
        !is_midna_menu_message())
    {
        return;
    }

    if (msg->getSelectPushFlag() != 1) {
        if (msg->getSelectPushFlag() == 2) {
            sMidnaHubWarpMenuOffered = false;
        }
        return;
    }

    if (msg->getSelectCursorPosLocal() != 1) {
        sMidnaHubWarpMenuOffered = false;
        return;
    }

    sMidnaHubWarpMenuOffered = false;
    warp_to_bossrush_hub_from_midna(daPy_py_c::getMidnaActor());
}

HookAction on_name_scene_change_pre(ModContext*, void* args, void*, void*) {
    dSv_save_c* save = dComIfGs_getSaveData();
    const bool bossRushActive = is_bossrush_game_mode_active() && is_boss_rush(save);
    const bool introSkipActive = is_intro_skipped(save);
    if (!bossRushActive && !introSkipActive) {
        return HOOK_CONTINUE;
    }

    if (introSkipActive) {
        prepare_intro_skip_start();
    }
    if (bossRushActive) {
        prepare_bossrush_start();
    }

    auto* fileSelect = name_scene_file_select(mods::arg<void*>(args, 0));
    if (fileSelect != nullptr && fileSelect->mSelectNum < 3) {
        sDataNewRestore = {
            fileSelect,
            fileSelect->mSelectNum,
            fileSelect->mIsDataNew[fileSelect->mSelectNum],
        };
        fileSelect->mIsDataNew[fileSelect->mSelectNum] = 0;
    }

    return HOOK_CONTINUE;
}

void on_name_scene_change_post(ModContext*, void*, void*, void*) {
    if (sDataNewRestore.fileSelect != nullptr && sDataNewRestore.slot < 3) {
        sDataNewRestore.fileSelect->mIsDataNew[sDataNewRestore.slot] = sDataNewRestore.value;
        sDataNewRestore = {};
    }
}

bool is_vanilla_new_file_stage(const char* stage, s16 point, s8 room, s8 layer) {
    return stage != nullptr && std::strcmp(stage, "F_SP108") == 0 && point == 21 && room == 1 &&
           layer == 13;
}

void set_next_stage_args(void* args, const char* stage, s16 point, s8 room, s8 layer) {
    mods::arg_ref<const char*>(args, 0) = stage;
    mods::arg_ref<s16>(args, 1) = point;
    mods::arg_ref<s8>(args, 2) = room;
    mods::arg_ref<s8>(args, 3) = layer;
}

HookAction on_set_next_stage_pre(ModContext*, void* args, void*, void*) {
    const char* stage = mods::arg<const char*>(args, 0);
    const s16 point = mods::arg<s16>(args, 1);
    const s8 room = mods::arg<s8>(args, 2);
    const s8 layer = mods::arg<s8>(args, 3);

    dSv_save_c* save = dComIfGs_getSaveData();
    if (is_bossrush_game_mode_active() && is_boss_rush(save) &&
        is_vanilla_new_file_stage(stage, point, room, layer)) {
        prepare_bossrush_start();
        set_next_stage_args(args, kBossRushReturnStage, kBossRushReturnPoint, kBossRushReturnRoom,
            kBossRushReturnLayer);
    } else if (is_intro_skipped(save) && is_vanilla_new_file_stage(stage, point, room, layer)) {
        prepare_intro_skip_start();
        set_next_stage_args(args, kIntroSkipStage, 0, kIntroSkipRoom, -1);
    }

    return HOOK_CONTINUE;
}

HookAction on_bosswarp_execute_pre(ModContext*, void* args, void* retval, void*) {
    auto* warp = mods::arg<daObjBossWarp_c*>(args, 0);
    if (warp == nullptr || !is_bossrush_game_mode_active() ||
        !is_boss_rush(dComIfGs_getSaveData()) ||
        boss_rush_state() != kBossRushStateHub || !is_boss_hub_stage_name())
    {
        return HOOK_CONTINUE;
    }

    const bool needsAppear = warp->scale.y < 0.99f;
    warp->scale.y = 1.0f;
    if (needsAppear) {
        warp->set_appear();
    }
    warp->setBaseMtx();
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

bool redirect_replay_to_hub(const BossRushEntry& entry) {
    if (boss_rush_state() != kBossRushStateReplay) {
        return false;
    }

    clear_boss_flags(entry);
    set_boss_rush_state(kBossRushStateHub);
    set_boss_rush_index(0);
    dComIfGp_event_reset();
    set_bossrush_next_stage();
    return true;
}

bool complete_direct_final_ganondorf() {
    const u8 index = boss_rush_index();
    if (index >= kBossRushEntryCount) {
        return false;
    }

    const BossRushEntry& entry = kBossRushEntries[index];
    if (entry.clearMode != BossRushEntry::FinalGanondorf) {
        return false;
    }

    return redirect_replay_to_hub(entry);
}

bool should_complete_direct_final_ganondorf(b_gnd_class* ganondorf) {
    if (ganondorf == NULL || !is_direct_final_ganondorf_active()) {
        return false;
    }

    if ((ganondorf->mDemoCamMode >= kGanondorfEndDemoStart &&
         ganondorf->mDemoCamMode < kGanondorfIntroCam) ||
        ganondorf->mActionMode == kGanondorfActionEnd)
    {
        return true;
    }

    daPy_py_c* player = daPy_getPlayerActorClass();
    return player != NULL && ganondorf->mActionMode == kGanondorfActionDown &&
           ganondorf->mMoveMode == 2 &&
           player->getCutType() == daPy_py_c::CUT_TYPE_DOWN;
}

bool complete_bossrush_final_sequence() {
    if (boss_rush_state() == kBossRushStateReplay) {
        return redirect_replay_to_hub(kBossRushEntries[boss_rush_index()]);
    }

    if (sAdvancePending) {
        return true;
    }

    grant_victory_heart();
    advance_bossrush();
    dComIfGp_event_reset();
    sAdvancePending = true;
    return true;
}

bool should_handle_final_scene_change(int exitId, s8 roomNo) {
    if (!is_bossrush_game_mode_active() || !is_boss_rush(dComIfGs_getSaveData()) ||
        boss_rush_state() == kBossRushStateHub) {
        return false;
    }

    const BossRushEntry& entry = kBossRushEntries[boss_rush_index()];
    const s8 activeRoom = roomNo == -1 ? dComIfGp_getStartStageRoomNo() : roomNo;

    if (entry.clearMode == BossRushEntry::FinalSequence) {
        if (boss_rush_state() == kBossRushStateReplay && is_current_stage_name("D_MN09A") &&
            activeRoom == kFinalPuppetRoom && exitId == 1)
        {
            return redirect_replay_to_hub(entry);
        }

        if (boss_rush_state() == kBossRushStateRun && exitId == 0 &&
            (is_current_stage_name("D_MN09B") || is_current_stage_name("D_MN09C")))
        {
            return complete_bossrush_final_sequence();
        }
    } else if (entry.clearMode == BossRushEntry::BeastGanon) {
        if (is_current_stage_name("D_MN09A") && activeRoom == kFinalBeastRoom && exitId == 2) {
            return redirect_replay_to_hub(entry);
        }
    } else if (entry.clearMode == BossRushEntry::FinalGanondorf) {
        if (is_current_stage_name("D_MN09C") && exitId == 0) {
            return redirect_replay_to_hub(entry);
        }
    }

    return false;
}

HookAction on_stage_change_pre(ModContext*, void* args, void* retval, void*) {
    const int exitId = mods::arg<int>(args, 0);
    const s8 roomNo = mods::arg<s8>(args, 3);

    if (!should_handle_final_scene_change(exitId, roomNo)) {
        return HOOK_CONTINUE;
    }

    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

void on_play_scene_update_post(ModContext*, void*, void*, void*) {
    if (is_bossrush_game_mode_active() && can_update_bossrush_gameplay()) {
        update_bossrush();
    }
}

HookAction on_play_scene_draw_pre(ModContext*, void*, void*, void*) {
    if (is_bossrush_game_mode_active()) {
        draw_bossrush_triangle_warning();
    }
    return HOOK_CONTINUE;
}

HookAction on_ganondorf_execute_pre(ModContext*, void* args, void* retval, void*) {
    if (!is_bossrush_game_mode_active() || !is_direct_final_ganondorf_active()) {
        return HOOK_CONTINUE;
    }

    auto* ganondorf = mods::arg<b_gnd_class*>(args, 0);
    if (ganondorf == NULL) {
        return HOOK_CONTINUE;
    }

    if (should_complete_direct_final_ganondorf(ganondorf) && complete_direct_final_ganondorf()) {
        if (retval != nullptr) {
            *static_cast<int*>(retval) = 1;
        }
        return HOOK_SKIP_ORIGINAL;
    }

    create_direct_final_barrier_if_needed(ganondorf);

    if (!sDirectFinalGanondorfStarted || ganondorf->mDrawHorse ||
        ganondorf->mActionMode < kGanondorfActionWait)
    {
        force_final_ganondorf_ground_start(ganondorf);
    }

    return HOOK_CONTINUE;
}

HookAction on_ganondorf_barrier_execute_pre(ModContext*, void* args, void*, void*) {
    if (!is_bossrush_game_mode_active() || !is_direct_final_ganondorf_active()) {
        return HOOK_CONTINUE;
    }

    arm_ganondorf_barrier(mods::arg<obj_gb_class*>(args, 0));
    return HOOK_CONTINUE;
}

void reset_bossrush_runtime_state(bool deleteActors) {
    sAdvancePending = false;
    sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
    sMidnaHubWarpMenuOffered = false;
    sHubPortalMidnaPromptOffered = false;
    clear_hub_confirm_state();
    if (deleteActors) {
        delete_hub_actors();
    } else {
        reset_hub_actor_ids();
    }
    reset_direct_final_boss_state();
    reset_bossrush_hazards();
}

ModResult install_bossrush_runtime_hooks(ModError* error) {
    if (sBossRushHooksInstalled) {
        return MOD_OK;
    }

    ModResult result = mods::hook_add_pre<StageChangeSceneHook>(svc_hook, on_stage_change_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Boss Rush scene hook");
    }

    result = mods::hook_add_pre<BossWarpExecuteHook>(svc_hook, on_bosswarp_execute_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight bossrush portal hook");
    }

    result = mods::hook_add_pre<MsgScrnTalkSetSelectStringHook>(svc_hook, before_midna_select_string);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Midna hub option talk hook");
    }

    result = mods::hook_add_pre<MsgScrnBaseSetStringHook>(svc_hook, before_midna_text_string);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Midna hub prompt text hook");
    }

    result = mods::hook_add_post<MsgObjectSelectProcHook>(svc_hook, after_midna_select_proc);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Midna hub warp hook");
    }

    result = mods::hook_add_pre<MeterExecuteHook>(svc_hook, before_meter_execute);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Midna hub prompt meter hook");
    }

    result = mods::hook_add_post<PlaySceneUpdateHook>(svc_hook, on_play_scene_update_post);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Boss Rush update hook");
    }

    result = mods::hook_add_pre<PlaySceneDrawHook>(svc_hook, on_play_scene_draw_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Boss Rush draw hook");
    }

    result = mods::hook_add_pre<GanondorfExecuteHook>(svc_hook, on_ganondorf_execute_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Ganondorf direct-start hook");
    }

    result =
        mods::hook_add_pre<GanondorfBarrierExecuteHook>(svc_hook, on_ganondorf_barrier_execute_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Ganondorf barrier hook");
    }

    sBossRushHooksInstalled = true;
    return MOD_OK;
}

template <class Entry>
ModResult uninstall_bossrush_hook(ModError* error, const char* message) {
    const ModResult result = mods::hook_uninstall<Entry>(svc_hook);
    if (result != MOD_OK) {
        return mods::set_error(error, result, message);
    }
    return MOD_OK;
}

ModResult uninstall_bossrush_runtime_hooks(ModError* error) {
    if (!sBossRushHooksInstalled) {
        return MOD_OK;
    }

    if (const ModResult result = uninstall_bossrush_hook<StageChangeSceneHook>(
            error, "failed to uninstall Dawnlight Boss Rush scene hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<BossWarpExecuteHook>(
            error, "failed to uninstall Dawnlight bossrush portal hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<MsgScrnTalkSetSelectStringHook>(
            error, "failed to uninstall Dawnlight Midna hub option talk hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<MsgScrnBaseSetStringHook>(
            error, "failed to uninstall Dawnlight Midna hub prompt text hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<MsgObjectSelectProcHook>(
            error, "failed to uninstall Dawnlight Midna hub warp hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<MeterExecuteHook>(
            error, "failed to uninstall Dawnlight Midna hub prompt meter hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<PlaySceneUpdateHook>(
            error, "failed to uninstall Dawnlight Boss Rush update hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<PlaySceneDrawHook>(
            error, "failed to uninstall Dawnlight Boss Rush draw hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<GanondorfExecuteHook>(
            error, "failed to uninstall Dawnlight Ganondorf direct-start hook");
        result != MOD_OK)
    {
        return result;
    }
    if (const ModResult result = uninstall_bossrush_hook<GanondorfBarrierExecuteHook>(
            error, "failed to uninstall Dawnlight Ganondorf barrier hook");
        result != MOD_OK)
    {
        return result;
    }

    sBossRushHooksInstalled = false;
    return MOD_OK;
}

ModResult on_bossrush_game_mode_activated(void*, ModError* error) {
    sBossRushGameModeActive = true;
    reset_bossrush_runtime_state(false);
    const ModResult result = install_bossrush_runtime_hooks(error);
    if (result != MOD_OK) {
        sBossRushGameModeActive = false;
    }
    return result;
}

ModResult on_bossrush_game_mode_deactivated(void*, ModError* error) {
    sBossRushGameModeActive = false;
    reset_bossrush_runtime_state(true);
    return uninstall_bossrush_runtime_hooks(error);
}

ModResult on_bossrush_game_mode_play(void*, ModError*) {
    reset_bossrush_runtime_state(false);
    return MOD_OK;
}

ModResult on_bossrush_save_loaded(void*, ModError*) {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (is_boss_rush(save)) {
        prepare_bossrush_start();
        reset_bossrush_runtime_state(false);
    }
    return MOD_OK;
}

ModResult on_bossrush_new_save(void*, ModError*) {
    apply_boss_rush_preset(dComIfGs_getSaveData());
    svc_log->info(mod_ctx, "Dawnlight Game Mode: Boss Rush new save applied");
    return MOD_OK;
}

ModResult on_bossrush_game_reset(void*, ModError*) {
    if (is_boss_rush(dComIfGs_getSaveData())) {
        set_bossrush_return_place();
    }
    reset_bossrush_runtime_state(true);
    return MOD_OK;
}

ModResult on_bossrush_tick(void*, ModError*) {
    return MOD_OK;
}

}  // namespace

ModResult register_new_save_modes(ModError* error) {
    ModResult result =
        mods::hook_add_post<FileSelectNameInput2Hook>(svc_hook, on_file_select_name_input2_post);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight new-save apply hook");
    }

    result = mods::hook_add_pre<NameSceneChangeGameSceneHook>(svc_hook, on_name_scene_change_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight new-save start pre-hook");
    }

    result = mods::hook_add_post<NameSceneChangeGameSceneHook>(svc_hook, on_name_scene_change_post);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight new-save start hook");
    }

    result = mods::hook_add_pre<SetNextStageHook>(svc_hook, on_set_next_stage_pre);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight new-save stage hook");
    }

    const GameModeDesc bossRushMode = {
        .struct_size = sizeof(GameModeDesc),
        .game_mode_id = kBossRushGameModeId,
        .full_name = "Boss Rush",
        .save_name = "gczelda2-bossrush",
        .user_data = nullptr,
        .on_activated = on_bossrush_game_mode_activated,
        .on_deactivated = on_bossrush_game_mode_deactivated,
        .on_play = on_bossrush_game_mode_play,
        .on_save_loaded = on_bossrush_save_loaded,
        .on_new_save = on_bossrush_new_save,
        .on_new_save_select = nullptr,
        .on_game_reset = on_bossrush_game_reset,
        .on_tick = on_bossrush_tick,
    };
    result = svc_game_mode->register_game_mode(mod_ctx, &bossRushMode);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight Boss Rush game mode");
    }

    return MOD_OK;
}

void shutdown_new_save_modes() {
    if (svc_game_mode != nullptr) {
        ModResult result = svc_game_mode->unregister_game_mode(mod_ctx, kBossRushGameModeId);
        if (result != MOD_OK) {
            svc_log->warn(mod_ctx, "Dawnlight Boss Rush: failed to unregister game mode");
        }
    }

    ModError error = MOD_ERROR_INIT;
    ModResult result = uninstall_bossrush_runtime_hooks(&error);
    if (result != MOD_OK) {
        svc_log->warn(mod_ctx, "Dawnlight Boss Rush: failed to uninstall runtime hooks");
    }
    sBossRushGameModeActive = false;
}

}  // namespace dawnlight
