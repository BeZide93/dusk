#include "dusk/bossrush.hpp"

#include "SSystem/SComponent/c_math.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_gameover.h"
#include "d/d_meter2_info.h"
#include "d/d_stage.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_overlap_mng.h"
#include "f_pc/f_pc_name.h"
#include "Z2AudioLib/Z2SeqMgr.h"
#include <cstdio>
#include <cstring>

namespace dusk::bossrush {
namespace {

struct BossRushEntry {
    const char* stage;
    s16 point;
    s8 room;
    s8 layer;
    int saveTable;
    enum ClearMode {
        Boss,
        MiddleBoss,
        GanondorfSequence,
    } clearMode;
    const char* displayName;
};

static const BossRushEntry kBossRushEntries[] = {
    {"D_MN05B", 0, 51, 0, dStage_SaveTbl_LV1, BossRushEntry::MiddleBoss, "Ook"},
    {"D_MN05A", 0, 50, 0, dStage_SaveTbl_LV1, BossRushEntry::Boss, "Diababa"},
    {"D_MN04B", 0, 51, 0, dStage_SaveTbl_LV2, BossRushEntry::MiddleBoss, "Dangoro"},
    {"D_MN04A", 0, 50, 0, dStage_SaveTbl_LV2, BossRushEntry::Boss, "Fyrus"},
    {"D_MN01B", 0, 51, 0, dStage_SaveTbl_LV3, BossRushEntry::MiddleBoss, "Deku Toad"},
    {"D_MN01A", 0, 50, 0, dStage_SaveTbl_LV3, BossRushEntry::Boss, "Morpheel"},
    {"D_MN10B", 0, 51, 0, dStage_SaveTbl_LV4, BossRushEntry::MiddleBoss, "Death Sword"},
    {"D_MN10A", 0, 50, 0, dStage_SaveTbl_LV4, BossRushEntry::Boss, "Stallord"},
    {"D_MN11B", 0, 51, 0, dStage_SaveTbl_LV5, BossRushEntry::MiddleBoss, "Darkhammer"},
    {"D_MN11A", 0, 50, 0, dStage_SaveTbl_LV5, BossRushEntry::Boss, "Blizzeta"},
    {"D_MN06B", 0, 51, 0, dStage_SaveTbl_LV6, BossRushEntry::MiddleBoss, "Darknut"},
    {"D_MN06A", 0, 50, 0, dStage_SaveTbl_LV6, BossRushEntry::Boss, "Armogohma"},
    {"D_MN07B", 0, 51, 0, dStage_SaveTbl_LV7, BossRushEntry::MiddleBoss, "Aeralfos"},
    {"D_MN07A", 0, 50, 0, dStage_SaveTbl_LV7, BossRushEntry::Boss, "Argorok"},
    {"D_MN08D", 0, 50, 0, dStage_SaveTbl_LV8, BossRushEntry::Boss, "Zant"},
    {"D_MN09A", 0, 50, 0, dStage_SaveTbl_LV9, BossRushEntry::GanondorfSequence, "Ganondorf"},
};

// Ganondorf is handled as a special sequence because the vanilla final battle spans multiple stages.

static constexpr u8 kBossRushStateHub = 0;
static constexpr u8 kBossRushStateRun = 1;
static constexpr u8 kBossRushStateReplay = 2;
static constexpr u8 kBossRushCenterPortalIndex = 16;
static constexpr u8 kBossRushHubPortalCount = kBossRushCenterPortalIndex + 1;
static constexpr char kBossRushHubStage[] = "D_MN09C";
static constexpr s16 kBossRushHubPoint = 0;
static constexpr s8 kBossRushHubRoom = 0;
static constexpr s8 kBossRushHubLayer = 0;
static constexpr f32 kBossRushHubY = 1100.0f;
static constexpr f32 kBossRushHubPortalRadius = 1200.0f;
static constexpr f32 kBossRushHubTriggerRadius = 150.0f;

static fpc_ProcID sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
static bool sAdvancePending = false;
static fpc_ProcID sHubBarrierId = fpcM_ERROR_PROCESS_ID_e;
static fpc_ProcID sHubPortalIds[kBossRushHubPortalCount];
static bool sHubActorIdsInitialized = false;
static bool sHubActorsSpawned = false;
static bool sHubPortalsArmed = false;
static bool sHubMidnaPromptActive = false;
static bool sHubMidnaPromptResolved = false;
static bool sMidnaHubWarpPromptActive = false;
static bool sMidnaHubWarpPromptResolved = false;
static bool sMidnaHubWarpRequested = false;
static int sHubPendingPortal = -1;
static char sHubPromptText[64] = "Fight Boss?";

cXyz hub_center() {
    return cXyz(0.0f, kBossRushHubY, 0.0f);
}

void reset_hub_actor_ids() {
    sHubBarrierId = fpcM_ERROR_PROCESS_ID_e;
    for (u8 i = 0; i < kBossRushHubPortalCount; i++) {
        sHubPortalIds[i] = fpcM_ERROR_PROCESS_ID_e;
    }
    sHubActorIdsInitialized = true;
    sHubActorsSpawned = false;
    sHubPortalsArmed = false;
    sHubMidnaPromptActive = false;
    sHubMidnaPromptResolved = false;
    sMidnaHubWarpPromptActive = false;
    sMidnaHubWarpPromptResolved = false;
    sMidnaHubWarpRequested = false;
    sHubPendingPortal = -1;
}

void ensure_hub_actor_ids_initialized() {
    if (!sHubActorIdsInitialized) {
        reset_hub_actor_ids();
    }
}

u8 entry_count() {
    return static_cast<u8>(sizeof(kBossRushEntries) / sizeof(kBossRushEntries[0]));
}

dSv_reserve_c& reserve() {
    return dComIfGs_getSaveData()->getReserve();
}

u8 current_index() {
    u8 index = reserve().getBossRushIndex();
    if (index >= entry_count()) {
        index = 0;
        reserve().setBossRushIndex(index);
    }

    return index;
}

const BossRushEntry& current_entry() {
    return kBossRushEntries[current_index()];
}

bool is_current_stage(const BossRushEntry& entry) {
    return strcmp(dComIfGp_getStartStageName(), entry.stage) == 0 &&
           dComIfGp_getStartStageRoomNo() == entry.room;
}

bool can_open_save_prompt() {
    return !dComIfGp_event_runCheck() && !fopOvlpM_IsPeek() && !dComIfGp_isEnableNextStage() &&
           dMeter2Info_getGameOverType() == 0;
}

bool can_hub_warp() {
    return can_open_save_prompt() && dComIfGp_getGameoverStatus() == 0;
}

bool can_offer_midna_hub_warp() {
    return reserve().isBossRush() && !has_hub_midna_prompt();
}

bool is_current_save_table(int saveTable) {
    stage_stag_info_class* stagInfo = dComIfGp_getStageStagInfo();
    return stagInfo != nullptr && saveTable == dStage_stagInfo_GetSaveTbl(stagInfo);
}

void ensure_story_state() {
    dComIfGs_onEventBit(dSv_event_flag_c::F_0250);
}

void clear_boss_flags(const BossRushEntry& entry) {
    dComIfGs_getSaveData()->getSave(entry.saveTable).getBit().init();

    if (is_current_save_table(entry.saveTable)) {
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
    for (u8 i = 0; i < entry_count(); i++) {
        clear_boss_flags(kBossRushEntries[i]);
    }
}

bool boss_is_cleared(const BossRushEntry& entry) {
    if (entry.clearMode == BossRushEntry::GanondorfSequence) {
        return false;
    }

    if (is_current_save_table(entry.saveTable)) {
        return entry.clearMode == BossRushEntry::MiddleBoss ? dComIfGs_isStageMiddleBoss() :
                                                              dComIfGs_isStageBossEnemy();
    }

    dSv_memBit_c& bit = dComIfGs_getSaveData()->getSave(entry.saveTable).getBit();
    return entry.clearMode == BossRushEntry::MiddleBoss ? bit.isStageBossEnemy2() : bit.isStageBossEnemy();
}

bool is_boss_hub_stage_name() {
    return strcmp(dComIfGp_getStartStageName(), kBossRushHubStage) == 0 &&
           dComIfGp_getStartStageRoomNo() == kBossRushHubRoom;
}

void set_return_place_hub() {
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(kBossRushHubStage, kBossRushHubRoom, 0);
}

void reset_audio_for_warp() {
    Z2SeqMgr* seqMgr = Z2GetSeqMgr();
    if (seqMgr == NULL) {
        return;
    }

    seqMgr->setBattleBgmOff(true);
    seqMgr->resetBattleBgmParams();
    seqMgr->bgmAllUnMute(0);
    seqMgr->unMuteSceneBgm(0);
    seqMgr->bgmStop(0, 0);
    seqMgr->subBgmStop();
    seqMgr->subBgmStopInner();
    seqMgr->bgmStreamStop(0);
    if (seqMgr->mFanfareHandle) {
        seqMgr->mFanfareHandle->stop(0);
    }
    seqMgr->mFanfareID.setAnonymous();
    seqMgr->mFanfareCount = 0;
}

void warp_to_hub() {
    reserve().setBossRushState(kBossRushStateHub);
    reserve().setBossRushIndex(0);
    set_return_place_hub();
    reset_audio_for_warp();
    dComIfGp_setNextStage(kBossRushHubStage, kBossRushHubPoint, kBossRushHubRoom, kBossRushHubLayer);
}

void prepare_hub_warp_item() {
    reserve().setBossRushState(kBossRushStateHub);
    reserve().setBossRushIndex(0);
    set_return_place_hub();
    reset_audio_for_warp();
    dComIfGs_setWarpItemData(kBossRushHubStage, hub_center(), 0, kBossRushHubRoom, 0, 1);
    dComIfGs_setItem(SLOT_18, dItemNo_DUNGEON_BACK_e);
}

void set_next_stage_for_entry(const BossRushEntry& entry) {
    ensure_story_state();
    clear_boss_flags(entry);
    set_return_place_hub();
    reset_audio_for_warp();
    dComIfGp_setNextStage(entry.stage, entry.point, entry.room, entry.layer);
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

void grant_core_items() {
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
    set_select_item(SELECT_ITEM_DOWN, SLOT_6);

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

    static const u16 kUtilityEventBits[] = {
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

    for (int i = 0; i < static_cast<int>(sizeof(kUtilityEventBits) / sizeof(kUtilityEventBits[0])); i++) {
        dComIfGs_onEventBit(kUtilityEventBits[i]);
    }

    dComIfGs_setLineUpItem();
}

void grant_victory_heart() {
    u16 maxLife = dComIfGs_getMaxLife();
    if (maxLife < 100) {
        dComIfGs_setMaxLife(static_cast<u8>(maxLife + 5));
    }

    dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
}

bool advance_to_next_entry() {
    u8 index = current_index();
    index++;

    if (index >= entry_count()) {
        u8 loop = reserve().getBossRushLoop();
        if (loop < 0xff) {
            reserve().setBossRushLoop(loop + 1);
        }
        reserve().setBossRushIndex(0);
        reserve().setBossRushState(kBossRushStateHub);
        set_return_place_hub();
        return false;
    }

    reserve().setBossRushIndex(index);
    clear_boss_flags(current_entry());
    set_return_place_hub();
    return true;
}

void finish_prompt_and_advance() {
    if (dComIfGp_getGameoverStatus() == 1) {
        d_GameOver_Delete(sSavePromptId);
        dComIfGp_setGameoverStatus(0);
        sAdvancePending = false;
        set_next_stage_for_current();
    }
}

void start_hub_entry(u8 index, u8 state) {
    if (index >= entry_count()) {
        return;
    }

    reserve().setBossRushState(state);
    reserve().setBossRushIndex(index);
    if (state == kBossRushStateRun) {
        reserve().setBossRushLoop(0);
        clear_all_boss_flags();
    }

    set_next_stage_for_entry(kBossRushEntries[index]);
}

const char* hub_portal_name(int portal) {
    if (portal == kBossRushCenterPortalIndex) {
        return "Boss Rush";
    }
    if (portal >= 0 && portal < entry_count()) {
        return kBossRushEntries[portal].displayName;
    }
    return "Boss";
}

void set_hub_pending_portal(int portal) {
    if (portal < 0 || portal >= kBossRushHubPortalCount) {
        sHubPendingPortal = -1;
        sHubMidnaPromptActive = false;
        return;
    }

    sHubPendingPortal = portal;
    std::snprintf(sHubPromptText, sizeof(sHubPromptText), "Fight %s?", hub_portal_name(portal));
}

void clear_hub_midna_prompt() {
    sHubPendingPortal = -1;
    sHubMidnaPromptActive = false;
    sHubPortalsArmed = false;
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
            return;
        }

        reset_hub_actor_ids();
    }

    cXyz center = hub_center();
    sHubBarrierId =
        fopAcM_create(fpcNm_OBJ_GB_e, 0xF0069600, &center, kBossRushHubRoom, NULL, NULL, -1);

    for (u8 i = 0; i < entry_count(); i++) {
        s16 angle = static_cast<s16>((0x10000 * i) / entry_count());
        cXyz pos(
            center.x + (cM_ssin(angle) * kBossRushHubPortalRadius),
            center.y,
            center.z + (cM_scos(angle) * kBossRushHubPortalRadius)
        );
        csXyz rot(0, angle, 0);
        sHubPortalIds[i] = fopAcM_createWarpHole(&pos, &rot, kBossRushHubRoom, i, 0, 0xff);
    }

    csXyz centerRot(0, 0, 0);
    sHubPortalIds[kBossRushCenterPortalIndex] =
        fopAcM_createWarpHole(&center, &centerRot, kBossRushHubRoom, kBossRushCenterPortalIndex, 0, 0xff);

    sHubActorsSpawned = true;
}

int touched_hub_portal() {
    daPy_py_c* player = daPy_getPlayerActorClass();
    if (player == NULL) {
        return -1;
    }

    cXyz center = hub_center();
    for (u8 i = 0; i < kBossRushHubPortalCount; i++) {
        cXyz pos = center;
        if (i < entry_count()) {
            s16 angle = static_cast<s16>((0x10000 * i) / entry_count());
            pos.x += cM_ssin(angle) * kBossRushHubPortalRadius;
            pos.z += cM_scos(angle) * kBossRushHubPortalRadius;
        }

        f32 distXZ = player->current.pos.absXZ(pos);
        f32 distY = player->current.pos.y - pos.y;
        if (distXZ < kBossRushHubTriggerRadius && distY < 200.0f && distY > -100.0f) {
            return i;
        }
    }

    return -1;
}

void update_hub() {
    sAdvancePending = false;
    sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
    set_return_place_hub();

    if (!is_boss_hub_stage_name()) {
        if (!dComIfGp_isEnableNextStage()) {
            dComIfGp_setNextStage(kBossRushHubStage, kBossRushHubPoint, kBossRushHubRoom, kBossRushHubLayer);
        }
        return;
    }

    spawn_hub_actors();

    if (sHubMidnaPromptActive) {
        return;
    }

    int portal = touched_hub_portal();
    if (portal < 0) {
        sHubPortalsArmed = true;
        if (!sHubMidnaPromptActive) {
            sHubPendingPortal = -1;
        }
        return;
    }

    if (!sHubPortalsArmed || !can_hub_warp()) {
        return;
    }

    sHubPortalsArmed = false;
    set_hub_pending_portal(portal);
}

void reset_hub_runtime_when_away() {
    if (!is_boss_hub_stage_name()) {
        sHubActorsSpawned = false;
        sHubPortalsArmed = false;
        sHubMidnaPromptActive = false;
        sHubMidnaPromptResolved = false;
        sHubPendingPortal = -1;
    }
}

}  // namespace

void apply_new_save_preset() {
    dSv_reserve_c& saveReserve = reserve();
    saveReserve.setBossRush(true);
    saveReserve.setBossRushIndex(0);
    saveReserve.setBossRushLoop(0);
    saveReserve.setBossRushState(kBossRushStateHub);

    dComIfGs_setMaxLife(25);
    dComIfGs_setLife(20);
    ensure_story_state();
    grant_core_items();

    clear_all_boss_flags();

    set_return_place_hub();
}

void set_next_stage_for_current() {
    if (!reserve().isBossRush()) {
        return;
    }

    if (reserve().getBossRushState() == kBossRushStateHub) {
        set_return_place_hub();
        reset_audio_for_warp();
        dComIfGp_setNextStage(kBossRushHubStage, kBossRushHubPoint, kBossRushHubRoom, kBossRushHubLayer);
        return;
    }

    const BossRushEntry& entry = current_entry();
    set_next_stage_for_entry(entry);
}

bool complete_ganondorf_sequence() {
    if (!reserve().isBossRush() || sAdvancePending || current_entry().clearMode != BossRushEntry::GanondorfSequence) {
        return false;
    }

    if (reserve().getBossRushState() == kBossRushStateReplay) {
        clear_boss_flags(current_entry());
        warp_to_hub();
        return true;
    }

    grant_victory_heart();
    advance_to_next_entry();
    sAdvancePending = true;
    return true;
}

bool is_hub_stage() {
    return reserve().isBossRush() && reserve().getBossRushState() == kBossRushStateHub &&
           is_boss_hub_stage_name();
}

bool is_hub_center_portal(u8 sceneListNo) {
    return sceneListNo == kBossRushCenterPortalIndex;
}

bool has_hub_midna_prompt() {
    return reserve().isBossRush() && reserve().getBossRushState() == kBossRushStateHub &&
           is_boss_hub_stage_name() && sHubPendingPortal >= 0 &&
           sHubPendingPortal < kBossRushHubPortalCount;
}

const char* hub_midna_prompt_text() {
    return sHubPromptText;
}

bool begin_hub_midna_prompt() {
    if (!has_hub_midna_prompt()) {
        return false;
    }

    sHubMidnaPromptActive = true;
    return true;
}

bool finish_hub_midna_prompt(int choice) {
    if (!sHubMidnaPromptActive || !has_hub_midna_prompt()) {
        return false;
    }

    int portal = sHubPendingPortal;
    clear_hub_midna_prompt();

    if (choice == 0) {
        if (portal == kBossRushCenterPortalIndex) {
            start_hub_entry(0, kBossRushStateRun);
        } else if (portal >= 0 && portal < entry_count()) {
            start_hub_entry(static_cast<u8>(portal), kBossRushStateReplay);
        }
    }

    return true;
}

bool resolve_hub_midna_prompt(int choice) {
    if (!finish_hub_midna_prompt(choice)) {
        return false;
    }

    sHubMidnaPromptResolved = true;
    return true;
}

bool consume_hub_midna_prompt_resolution() {
    if (!sHubMidnaPromptResolved) {
        return false;
    }

    sHubMidnaPromptResolved = false;
    return true;
}

bool has_midna_hub_warp_prompt() {
    return sMidnaHubWarpPromptActive && can_offer_midna_hub_warp();
}

const char* midna_hub_warp_option_text() {
    return "Warp to Garden of Twilight";
}

bool begin_midna_hub_warp_prompt() {
    if (!can_offer_midna_hub_warp()) {
        return false;
    }

    sMidnaHubWarpPromptActive = true;
    return true;
}

bool resolve_midna_hub_warp_prompt(int choice) {
    if (!sMidnaHubWarpPromptActive) {
        return false;
    }

    sMidnaHubWarpPromptActive = false;
    if (choice != 2) {
        return false;
    }

    sMidnaHubWarpPromptResolved = true;
    sMidnaHubWarpRequested = true;
    return true;
}

bool cancel_midna_hub_warp_prompt() {
    if (!sMidnaHubWarpPromptActive) {
        return false;
    }

    sMidnaHubWarpPromptActive = false;
    sMidnaHubWarpPromptResolved = true;
    return true;
}

bool consume_midna_hub_warp_prompt_resolution() {
    if (!sMidnaHubWarpPromptResolved) {
        return false;
    }

    sMidnaHubWarpPromptResolved = false;
    return true;
}

bool consume_midna_hub_warp_request() {
    if (!sMidnaHubWarpRequested) {
        return false;
    }

    sMidnaHubWarpRequested = false;
    return true;
}

bool prepare_midna_hub_warp() {
    if (!reserve().isBossRush()) {
        return false;
    }

    prepare_hub_warp_item();
    return true;
}

void warp_to_hub_now() {
    if (reserve().isBossRush()) {
        warp_to_hub();
    }
}

void update() {
    if (!reserve().isBossRush()) {
        sAdvancePending = false;
        sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
        reset_hub_actor_ids();
        return;
    }

    if (reserve().getBossRushState() == kBossRushStateHub) {
        update_hub();
        return;
    }

    reset_hub_runtime_when_away();

    if (dComIfGs_getLife() == 0 && !dComIfGp_isEnableNextStage()) {
        reserve().setBossRushState(kBossRushStateHub);
        reserve().setBossRushIndex(0);
        set_return_place_hub();
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

    const BossRushEntry& entry = current_entry();
    if (is_current_stage(entry) && boss_is_cleared(entry)) {
        if (reserve().getBossRushState() == kBossRushStateReplay) {
            clear_boss_flags(entry);
            warp_to_hub();
        } else {
            grant_victory_heart();
            advance_to_next_entry();
            sAdvancePending = true;
        }
    }
}

}  // namespace dusk::bossrush
