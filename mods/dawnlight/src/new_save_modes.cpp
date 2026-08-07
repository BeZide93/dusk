#include "config.hpp"
#include "save_compat.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "d/d_com_inf_game.h"
#include "d/d_gameover.h"
#include "d/d_item.h"
#include "d/d_item_data.h"
#include "d/d_meter2_info.h"
#include "d/d_s_name.h"
#include "d/d_save.h"
#include "d/d_stage.h"
#include "f_op/f_op_overlap_mng.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/save.h"

#include <algorithm>
#include <cstring>

namespace dawnlight {
namespace {

DEFINE_HOOK(&dScnName_c::changeGameScene, NameSceneChangeGameSceneHook);
DEFINE_HOOK_SYMBOL("dScnPly_Execute", int(void*), PlaySceneUpdateHook);

constexpr size_t kReserveOffset = 0x8F0;
constexpr size_t kIntroSkipOffset = 16;
constexpr size_t kBossRushOffset = 32;
constexpr size_t kBossRushIndexOffset = 40;
constexpr size_t kBossRushLoopOffset = 41;
constexpr char kIntroSkipMagic[] = "DUSKSKP1";
constexpr char kBossRushMagic[] = "DUSKBR1";

constexpr char kIntroSkipStage[] = "F_SP108";
constexpr s16 kIntroSkipPoint = 0;
constexpr s8 kIntroSkipRoom = 0;
constexpr s8 kIntroSkipLayer = -1;

struct BossRushEntry {
    const char* stage;
    s16 point;
    s8 room;
    s8 layer;
    int saveTable;
    enum ClearMode {
        Boss,
        MiddleBoss,
    } clearMode;
};

constexpr BossRushEntry kBossRushEntries[] = {
    {"D_MN05B", 0, 51, 0, dStage_SaveTbl_LV1, BossRushEntry::MiddleBoss},
    {"D_MN05A", 0, 50, 0, dStage_SaveTbl_LV1, BossRushEntry::Boss},
    {"D_MN04B", 0, 51, 0, dStage_SaveTbl_LV2, BossRushEntry::MiddleBoss},
    {"D_MN04A", 0, 50, 0, dStage_SaveTbl_LV2, BossRushEntry::Boss},
    {"D_MN01B", 0, 51, 0, dStage_SaveTbl_LV3, BossRushEntry::MiddleBoss},
    {"D_MN01A", 0, 50, 0, dStage_SaveTbl_LV3, BossRushEntry::Boss},
    {"D_MN10B", 0, 51, 0, dStage_SaveTbl_LV4, BossRushEntry::MiddleBoss},
    {"D_MN10A", 0, 50, 0, dStage_SaveTbl_LV4, BossRushEntry::Boss},
    {"D_MN11B", 0, 51, 0, dStage_SaveTbl_LV5, BossRushEntry::MiddleBoss},
    {"D_MN11A", 0, 50, 0, dStage_SaveTbl_LV5, BossRushEntry::Boss},
    {"D_MN06B", 0, 51, 0, dStage_SaveTbl_LV6, BossRushEntry::MiddleBoss},
    {"D_MN06A", 0, 50, 0, dStage_SaveTbl_LV6, BossRushEntry::Boss},
    {"D_MN07B", 0, 51, 0, dStage_SaveTbl_LV7, BossRushEntry::MiddleBoss},
    {"D_MN07A", 0, 50, 0, dStage_SaveTbl_LV7, BossRushEntry::Boss},
    {"D_MN08D", 0, 50, 0, dStage_SaveTbl_LV8, BossRushEntry::Boss},
};

constexpr size_t kBossRushEntryCount = std::size(kBossRushEntries);
constexpr s8 kBossRushReturnRoom = 0;
constexpr char kBossRushReturnStage[] = "D_MN09C";

fpc_ProcID sSavePromptId = fpcM_ERROR_PROCESS_ID_e;
bool sAdvancePending = false;

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

bool is_current_save_table(int saveTable) {
    stage_stag_info_class* stagInfo = dComIfGp_getStageStagInfo();
    return stagInfo != nullptr && saveTable == dStage_stagInfo_GetSaveTbl(stagInfo);
}

bool boss_is_cleared(const BossRushEntry& entry) {
    if (is_current_save_table(entry.saveTable)) {
        return entry.clearMode == BossRushEntry::MiddleBoss ? dComIfGs_isStageMiddleBoss() :
                                                              dComIfGs_isStageBossEnemy();
    }

    dSv_memBit_c& bit = dComIfGs_getSaveData()->getSave(entry.saveTable).getBit();
    return entry.clearMode == BossRushEntry::MiddleBoss ? bit.isStageBossEnemy2() :
                                                          bit.isStageBossEnemy();
}

void set_bossrush_next_stage() {
    if (!is_boss_rush(dComIfGs_getSaveData())) {
        return;
    }

    const BossRushEntry& entry = kBossRushEntries[boss_rush_index()];
    ensure_bossrush_story_state();
    clear_boss_flags(entry);
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
    index++;
    if (index >= kBossRushEntryCount) {
        index = 0;
        increment_boss_rush_loop();
    }
    set_boss_rush_index(index);
}

bool can_open_save_prompt() {
    return !dComIfGp_event_runCheck() && !fopOvlpM_IsPeek() && !dComIfGp_isEnableNextStage() &&
           dMeter2Info_getGameOverType() == 0;
}

void finish_prompt_and_advance() {
    if (dComIfGp_getGameoverStatus() == 1) {
        d_GameOver_Delete(sSavePromptId);
        dComIfGp_setGameoverStatus(0);
        sAdvancePending = false;
        set_bossrush_next_stage();
    }
}

void update_bossrush() {
    if (!is_boss_rush(dComIfGs_getSaveData())) {
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

    const BossRushEntry& entry = kBossRushEntries[boss_rush_index()];
    if (is_current_stage(entry) && boss_is_cleared(entry)) {
        grant_victory_heart();
        advance_bossrush();
        sAdvancePending = true;
    }
}

void clear_dawnlight_new_save_markers(dSv_save_c* save) {
    set_intro_skipped(save, false);
    set_boss_rush(save, false);
}

void on_new_save(ModContext*, uint32_t, void*) {
    dSv_save_c* save = dComIfGs_getSaveData();
    clear_dawnlight_new_save_markers(save);

    switch (new_save_mode()) {
    case NewSaveMode::IntroSkip:
        apply_intro_skip_preset(save);
        svc_log->info(mod_ctx, "Dawnlight New Save Mode: Intro Skip applied");
        break;
    case NewSaveMode::BossRush:
        apply_boss_rush_preset(save);
        svc_log->info(mod_ctx, "Dawnlight New Save Mode: Boss Rush applied");
        break;
    case NewSaveMode::Vanilla:
    default:
        break;
    }
}

void set_intro_skip_next_stage() {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (!is_intro_skipped(save)) {
        return;
    }

    if (!dComIfGs_isEventBit(dSv_event_flag_c::F_0226)) {
        dComIfGs_offSaveSwitch(dStage_SaveTbl_FARON, 12);
    }
    dComIfGs_onSaveSwitch(dStage_SaveTbl_FARON, 20);
    dComIfGp_setNextStage(
        kIntroSkipStage, kIntroSkipPoint, kIntroSkipRoom, kIntroSkipLayer, 0.0f, 0, 1, 0, 0, 0, 0);
}

void on_name_scene_change_post(ModContext*, void*, void*, void*) {
    dSv_save_c* save = dComIfGs_getSaveData();
    if (is_boss_rush(save)) {
        set_bossrush_next_stage();
    } else if (is_intro_skipped(save)) {
        set_intro_skip_next_stage();
    }
}

void on_play_scene_update_post(ModContext*, void*, void*, void*) {
    update_bossrush();
}

}  // namespace

ModResult install_new_save_mode_hooks(ModError* error) {
    ModResult result = svc_save->observe_saves(mod_ctx, on_new_save, nullptr, nullptr, nullptr, nullptr);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to observe Dawnlight new-save mode");
    }

    result = mods::hook_add_post<NameSceneChangeGameSceneHook>(svc_hook, on_name_scene_change_post);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight new-save start hook");
    }

    result = mods::hook_add_post<PlaySceneUpdateHook>(svc_hook, on_play_scene_update_post);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Boss Rush update hook");
    }

    return MOD_OK;
}

}  // namespace dawnlight
