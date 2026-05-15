#include "dusk/bossrush.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_gameover.h"
#include "d/d_meter2_info.h"
#include "d/d_stage.h"
#include "f_op/f_op_overlap_mng.h"
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
};

static const BossRushEntry kBossRushEntries[] = {
    {"D_MN05B", 0, 51, 0, dStage_SaveTbl_LV1, BossRushEntry::MiddleBoss},         // Ook
    {"D_MN05A", 0, 50, 0, dStage_SaveTbl_LV1, BossRushEntry::Boss},               // Diababa
    {"D_MN04B", 0, 51, 0, dStage_SaveTbl_LV2, BossRushEntry::MiddleBoss},         // Dangoro
    {"D_MN04A", 0, 50, 0, dStage_SaveTbl_LV2, BossRushEntry::Boss},               // Fyrus
    {"D_MN01B", 0, 51, 0, dStage_SaveTbl_LV3, BossRushEntry::MiddleBoss},         // Deku Toad
    {"D_MN01A", 0, 50, 0, dStage_SaveTbl_LV3, BossRushEntry::Boss},               // Morpheel
    {"D_MN10B", 0, 51, 0, dStage_SaveTbl_LV4, BossRushEntry::MiddleBoss},         // Death Sword
    {"D_MN10A", 0, 50, 0, dStage_SaveTbl_LV4, BossRushEntry::Boss},               // Stallord
    {"D_MN11B", 0, 51, 0, dStage_SaveTbl_LV5, BossRushEntry::MiddleBoss},         // Darkhammer
    {"D_MN11A", 0, 50, 0, dStage_SaveTbl_LV5, BossRushEntry::Boss},               // Blizzeta
    {"D_MN06B", 0, 51, 0, dStage_SaveTbl_LV6, BossRushEntry::MiddleBoss},         // Darknut
    {"D_MN06A", 0, 50, 0, dStage_SaveTbl_LV6, BossRushEntry::Boss},               // Armogohma
    {"D_MN07B", 0, 51, 0, dStage_SaveTbl_LV7, BossRushEntry::MiddleBoss},         // Aeralfos
    {"D_MN07A", 0, 50, 0, dStage_SaveTbl_LV7, BossRushEntry::Boss},               // Argorok
    {"D_MN08D", 0, 50, 0, dStage_SaveTbl_LV8, BossRushEntry::Boss},               // Zant
    {"D_MN09A", 0, 50, 0, dStage_SaveTbl_LV9, BossRushEntry::GanondorfSequence},  // Ganondorf
};

// Ganondorf is handled as a special sequence because the vanilla final battle spans multiple stages.

static fpc_ProcID sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
static bool sAdvancePending = false;

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

bool is_current_save_table(int saveTable) {
    stage_stag_info_class* stagInfo = dComIfGp_getStageStagInfo();
    return stagInfo != nullptr && saveTable == dStage_stagInfo_GetSaveTbl(stagInfo);
}

void ensure_story_state() {
    dComIfGs_onEventBit(dSv_event_flag_c::F_0250);
}

void clear_boss_flags(const BossRushEntry& entry) {
    dSv_memBit_c& bit = dComIfGs_getSaveData()->getSave(entry.saveTable).getBit();
    if (entry.clearMode == BossRushEntry::MiddleBoss) {
        bit.offStageBossEnemy2();
    } else {
        bit.offStageBossEnemy();
        bit.offStageLife();
        bit.offStageBossDemo();
    }

    if (is_current_save_table(entry.saveTable)) {
        if (entry.clearMode == BossRushEntry::MiddleBoss) {
            dComIfGs_offStageMiddleBoss();
        } else {
            dComIfGs_offStageBossEnemy();
            dComIfGs_offStageLife();
            dComIfGs_offStageBossDemo();
        }
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

void set_return_place(const BossRushEntry& entry) {
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(entry.stage, entry.room, 0);
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

void advance_to_next_entry() {
    u8 index = current_index();
    index++;

    if (index >= entry_count()) {
        index = 0;
        u8 loop = reserve().getBossRushLoop();
        if (loop < 0xff) {
            reserve().setBossRushLoop(loop + 1);
        }
    }

    reserve().setBossRushIndex(index);
    clear_boss_flags(current_entry());
    set_return_place(current_entry());
}

void finish_prompt_and_advance() {
    if (dComIfGp_getGameoverStatus() == 1) {
        d_GameOver_Delete(sSavePromptId);
        dComIfGp_setGameoverStatus(0);
        sAdvancePending = false;
        set_next_stage_for_current();
    }
}

}  // namespace

void apply_new_save_preset() {
    dSv_reserve_c& saveReserve = reserve();
    saveReserve.setBossRush(true);
    saveReserve.setBossRushIndex(0);
    saveReserve.setBossRushLoop(0);
    saveReserve.setBossRushState(0);

    dComIfGs_setMaxLife(25);
    dComIfGs_setLife(20);
    ensure_story_state();
    grant_core_items();

    for (u8 i = 0; i < entry_count(); i++) {
        clear_boss_flags(kBossRushEntries[i]);
    }

    set_return_place(current_entry());
}

void set_next_stage_for_current() {
    if (!reserve().isBossRush()) {
        return;
    }

    const BossRushEntry& entry = current_entry();
    ensure_story_state();
    clear_boss_flags(entry);
    set_return_place(entry);
    dComIfGp_setNextStage(entry.stage, entry.point, entry.room, entry.layer);
}

bool complete_ganondorf_sequence() {
    if (!reserve().isBossRush() || sAdvancePending || current_entry().clearMode != BossRushEntry::GanondorfSequence) {
        return false;
    }

    grant_victory_heart();
    advance_to_next_entry();
    sAdvancePending = true;
    return true;
}

void update() {
    if (!reserve().isBossRush()) {
        sAdvancePending = false;
        sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
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
        grant_victory_heart();
        advance_to_next_entry();
        sAdvancePending = true;
    }
}

}  // namespace dusk::bossrush
