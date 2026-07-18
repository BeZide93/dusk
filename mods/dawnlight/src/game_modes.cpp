#include "game_modes.hpp"
#include "bossrush.hpp"

#include "global.h"
#include "d/d_com_inf_game.h"
#include "d/d_file_select.h"
#include "d/d_item.h"
#include "d/d_save.h"
#include "d/d_stage.h"

#include <algorithm>
#include <cstring>

namespace dawnlight::game_modes {
namespace {

constexpr size_t kReserveOffset = 0x8F0;
constexpr size_t kNgPlusCountOffset = 8;
constexpr size_t kIntroSkipOffset = 16;
constexpr size_t kBossRushOffset = 32;
constexpr char kNgPlusMagic[] = "DUSKNGP1";
constexpr char kIntroSkipMagic[] = "DUSKSKP1";
constexpr char kBossRushMagic[] = "DUSKBR1";

u8* reserve_bytes(dSv_save_c* save) {
    return save == nullptr ? nullptr : reinterpret_cast<u8*>(save) + kReserveOffset;
}

const u8* reserve_bytes(const dSv_save_c* save) {
    return save == nullptr ? nullptr : reinterpret_cast<const u8*>(save) + kReserveOffset;
}

void set_marker(dSv_save_c* save, size_t offset, const char* magic, size_t length, bool enabled) {
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

void set_intro_skipped(dSv_save_c* save, bool enabled) {
    set_marker(save, kIntroSkipOffset, kIntroSkipMagic, sizeof(kIntroSkipMagic) - 1, enabled);
}

unsigned new_game_plus_count(const dSv_save_c* save) {
    const u8* reserve = reserve_bytes(save);
    if (reserve == nullptr ||
        std::memcmp(reserve, kNgPlusMagic, sizeof(kNgPlusMagic) - 1) != 0)
    {
        return 0;
    }
    return reserve[kNgPlusCountOffset] == 0 ? 1 : reserve[kNgPlusCountOffset];
}

void set_new_game_plus_count(dSv_save_c* save, unsigned count) {
    u8* reserve = reserve_bytes(save);
    if (reserve == nullptr) {
        return;
    }
    set_marker(save, 0, kNgPlusMagic, sizeof(kNgPlusMagic) - 1, count != 0);
    reserve[kNgPlusCountOffset] = static_cast<u8>(std::min(count, 255u));
}

bool valid_source(dFile_select_c* fileSelect, u8 slot, u8) {
    return fileSelect != nullptr && slot < 3 && !fileSelect->mIsNoData[slot] &&
           fileSelect->mIsDataNew[slot] == 0;
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

static bool isIntroSkipBottleItem(u8 i_itemNo) {
    return i_itemNo >= dItemNo_EMPTY_BOTTLE_e && i_itemNo <= dItemNo_DROP_BOTTLE_e;
}

static bool isNewGamePlusBombBagItem(u8 i_itemNo) {
    return i_itemNo == dItemNo_BOMB_BAG_LV1_e || i_itemNo == dItemNo_NORMAL_BOMB_e ||
           i_itemNo == dItemNo_WATER_BOMB_e || i_itemNo == dItemNo_POKE_BOMB_e;
}

static bool isNewGamePlusCarrySlot(u8 i_slotNo) {
    switch (i_slotNo) {
    case SLOT_0:  // Gale Boomerang
    case SLOT_1:  // Lantern
    case SLOT_2:  // Spinner
    case SLOT_3:  // Iron Boots
    case SLOT_4:  // Bow
    case SLOT_5:  // Hawkeye
    case SLOT_6:  // Ball and Chain
    case SLOT_10: // Double Clawshots
    case SLOT_11:
    case SLOT_12:
    case SLOT_13:
    case SLOT_14: // Bottles
    case SLOT_15:
    case SLOT_16:
    case SLOT_17: // Bomb bags
    case SLOT_23: // Slingshot
        return true;
    default:
        return false;
    }
}

static bool hasIntroSkipItem(u8 i_itemNo) {
    if (dComIfGs_isItemFirstBit(i_itemNo)) {
        return true;
    }

    for (int i = 0; i < MAX_ITEM_SLOTS; i++) {
        if (dComIfGs_getItem(i, false) == i_itemNo) {
            return true;
        }
    }

    return false;
}

static bool hasIntroSkipBottle() {
    for (int i = SLOT_11; i <= SLOT_14; i++) {
        if (isIntroSkipBottleItem(dComIfGs_getItem(i, false))) {
            return true;
        }
    }

    return false;
}

static void ensureIntroSkipItem(int i_slotNo, u8 i_itemNo) {
    if (!hasIntroSkipItem(i_itemNo)) {
        dComIfGs_setItem(i_slotNo, i_itemNo);
    }

    dComIfGs_onItemFirstBit(i_itemNo);
}

static void setIntroSkipSelectItemIfEmpty(int i_selectNo, u8 i_slotNo) {
    if (dComIfGs_getSelectItemIndex(i_selectNo) == dItemNo_NONE_e) {
        dComIfGs_setMixItemIndex(i_selectNo, dItemNo_NONE_e);
        dComIfGs_setSelectItemIndex(i_selectNo, i_slotNo);
    }
}

}  // namespace

void clear_markers(dSv_save_c* save) {
    set_new_game_plus_count(save, 0);
    set_intro_skipped(save, false);
    set_marker(save, kBossRushOffset, kBossRushMagic, sizeof(kBossRushMagic) - 1, false);
}

void apply_new_game_plus(dFile_select_c* fileSelect, u8 sourceSlot, u8 destinationSlot) {
    bool mNewGamePlusPending = true;
    u8 mNewGamePlusSourceSlot = sourceSlot;
    u8 mNewGamePlusTargetSlot = destinationSlot;
    if (!mNewGamePlusPending ||
        !valid_source(fileSelect, mNewGamePlusSourceSlot, mNewGamePlusTargetSlot))
    {
        mNewGamePlusPending = false;
        mNewGamePlusSourceSlot = 0xff;
        mNewGamePlusTargetSlot = 0xff;
        return;
    }

    u8 targetSlot = mNewGamePlusTargetSlot;
    bool isOverwriteNewGamePlus = mNewGamePlusSourceSlot == mNewGamePlusTargetSlot;

    static const u16 hiddenSkillFlags[] = {
        dSv_event_flag_c::F_0339,
        dSv_event_flag_c::F_0338,
        dSv_event_flag_c::F_0340,
        dSv_event_flag_c::F_0341,
        dSv_event_flag_c::F_0342,
        dSv_event_flag_c::F_0343,
        dSv_event_flag_c::F_0344,
    };

    dSv_save_c* srcSave = (dSv_save_c*)&fileSelect->mSaveData[mNewGamePlusSourceSlot];
    dSv_save_c* dstSave = dComIfGs_getSaveData();
    dSv_player_c& srcPlayer = srcSave->getPlayer();
    dSv_player_c& dstPlayer = dstSave->getPlayer();

    char playerName[17];
    char horseName[17];
    strncpy(playerName, dstPlayer.getPlayerInfo().getPlayerName(), sizeof(playerName) - 1);
    playerName[sizeof(playerName) - 1] = '\0';
    strncpy(horseName, dstPlayer.getPlayerInfo().getHorseName(), sizeof(horseName) - 1);
    horseName[sizeof(horseName) - 1] = '\0';

    dSv_player_status_a_c& srcStatus = srcPlayer.getPlayerStatusA();
    dSv_player_status_a_c& dstStatus = dstPlayer.getPlayerStatusA();
    dstStatus.setMaxLife(srcStatus.getMaxLife());
    dstStatus.setLife(srcStatus.getMaxLife());
    dstStatus.setRupee(srcStatus.getRupee());
    dstStatus.setMaxOil(srcStatus.getMaxOil());
    dstStatus.setOil(srcStatus.getOil());
    dstStatus.setWalletSize(srcStatus.getWalletSize());

    dSv_player_item_c& srcItems = srcPlayer.getItem();
    dSv_player_item_c& dstItems = dstPlayer.getItem();
    dSv_player_get_item_c& srcGetItems = srcPlayer.getGetItem();
    dSv_player_get_item_c& dstGetItems = dstPlayer.getGetItem();
    dSv_player_item_record_c& srcRecord = srcPlayer.getItemRecord();
    dSv_player_item_record_c& dstRecord = dstPlayer.getItemRecord();
    dSv_player_item_max_c& srcMax = srcPlayer.getItemMax();
    dSv_player_item_max_c& dstMax = dstPlayer.getItemMax();
    dSv_player_collect_c& srcCollect = srcPlayer.getCollect();
    dSv_player_collect_c& dstCollect = dstPlayer.getCollect();

    const struct {
        u8 slot;
        u8 item;
    } carriedItems[] = {
        {SLOT_0, dItemNo_BOOMERANG_e}, {SLOT_1, dItemNo_KANTERA_e},
        {SLOT_2, dItemNo_SPINNER_e},   {SLOT_3, dItemNo_HVY_BOOTS_e},
        {SLOT_4, dItemNo_BOW_e},       {SLOT_5, dItemNo_HAWK_EYE_e},
        {SLOT_6, dItemNo_IRONBALL_e},  {SLOT_10, dItemNo_W_HOOKSHOT_e},
        {SLOT_23, dItemNo_PACHINKO_e},
    };

    for (int i = 0; i < (int)(sizeof(carriedItems) / sizeof(carriedItems[0])); i++) {
        if (srcItems.getItem(carriedItems[i].slot, false) == carriedItems[i].item) {
            dstItems.setItem(carriedItems[i].slot, carriedItems[i].item);
        }
    }

    for (int slot = SLOT_11; slot <= SLOT_14; slot++) {
        u8 item = srcItems.getItem(slot, false);
        if (isIntroSkipBottleItem(item)) {
            dstItems.setItem(slot, item);
            dstRecord.setBottleNum(slot - SLOT_11, srcRecord.getBottleNum(slot - SLOT_11));
        }
    }

    bool hasBombBag = false;
    for (int slot = SLOT_15; slot <= SLOT_17; slot++) {
        u8 item = srcItems.getItem(slot, false);
        if (isNewGamePlusBombBagItem(item)) {
            dstItems.setItem(slot, item);
            dstRecord.setBombNum(slot - SLOT_15, srcRecord.getBombNum(slot - SLOT_15));
            hasBombBag = true;
        }
    }

    dstRecord.setArrowNum(srcRecord.getArrowNum());
    dstRecord.setPachinkoNum(srcRecord.getPachinkoNum());
    dstMax = srcMax;
    dstPlayer.getFishingInfo() = srcPlayer.getFishingInfo();

    bool hasMasterSword = srcCollect.isCollect(COLLECT_SWORD, COLLECT_MASTER_SWORD) ||
                          srcGetItems.isFirstBit(dItemNo_MASTER_SWORD_e);
    if (hasMasterSword) {
        dstCollect.setCollect(COLLECT_SWORD, COLLECT_MASTER_SWORD);
        dstGetItems.onFirstBit(dItemNo_MASTER_SWORD_e);
        dstStatus.setSelectEquip(COLLECT_SWORD, dItemNo_MASTER_SWORD_e);
    }

    bool hasHylianShield = srcCollect.isCollect(COLLECT_SHIELD, COLLECT_HYLIAN_SHIELD) ||
                           srcGetItems.isFirstBit(dItemNo_HYLIA_SHIELD_e);
    if (hasHylianShield) {
        dstCollect.setCollect(COLLECT_SHIELD, COLLECT_HYLIAN_SHIELD);
        dstGetItems.onFirstBit(dItemNo_HYLIA_SHIELD_e);
        dstStatus.setSelectEquip(COLLECT_SHIELD, dItemNo_HYLIA_SHIELD_e);
    }

    bool hasHeroClothes = srcCollect.isCollect(COLLECT_CLOTHING, KOKIRI_CLOTHES_FLAG) ||
                          srcGetItems.isFirstBit(dItemNo_WEAR_KOKIRI_e);
    bool hasMagicArmor = srcGetItems.isFirstBit(dItemNo_ARMOR_e);
    if (hasHeroClothes) {
        dstCollect.setCollect(COLLECT_CLOTHING, KOKIRI_CLOTHES_FLAG);
        dstGetItems.onFirstBit(dItemNo_WEAR_KOKIRI_e);
        dstStatus.setSelectEquip(COLLECT_CLOTHING, dItemNo_WEAR_KOKIRI_e);
    }
    if (hasMagicArmor) {
        dstGetItems.onFirstBit(dItemNo_ARMOR_e);
        if (srcStatus.getSelectEquip(COLLECT_CLOTHING) == dItemNo_ARMOR_e) {
            dstStatus.setSelectEquip(COLLECT_CLOTHING, dItemNo_ARMOR_e);
        }
    }

    if (dstItems.getItem(SLOT_4, false) == dItemNo_BOW_e) {
        dstGetItems.onFirstBit(dItemNo_BOW_e);
    }
    if (dstItems.getItem(SLOT_1, false) == dItemNo_KANTERA_e) {
        dstGetItems.onFirstBit(dItemNo_KANTERA_e);
    }
    if (dstItems.getItem(SLOT_5, false) == dItemNo_HAWK_EYE_e) {
        dstGetItems.onFirstBit(dItemNo_HAWK_EYE_e);
    }
    if (hasBombBag) {
        dstGetItems.onFirstBit(dItemNo_BOMB_BAG_LV1_e);
        if (srcGetItems.isFirstBit(dItemNo_BOMB_BAG_LV2_e)) {
            dstGetItems.onFirstBit(dItemNo_BOMB_BAG_LV2_e);
        }
        for (int slot = SLOT_15; slot <= SLOT_17; slot++) {
            u8 item = dstItems.getItem(slot, false);
            if (item == dItemNo_NORMAL_BOMB_e || item == dItemNo_WATER_BOMB_e ||
                item == dItemNo_POKE_BOMB_e)
            {
                dstGetItems.onFirstBit(item);
            }
        }
    }

    for (u8 item = dItemNo_M_BEETLE_e; item <= dItemNo_F_MAYFLY_e; item++) {
        if (srcGetItems.isFirstBit(item)) {
            dstGetItems.onFirstBit(item);
        }
    }

    for (int i = 0; i < MAX_SELECT_ITEM; i++) {
        u8 selectSlot = srcStatus.getSelectItemIndex(i);
        if (selectSlot < MAX_ITEM_SLOTS && isNewGamePlusCarrySlot(selectSlot) &&
            dstItems.getItem(selectSlot, false) != dItemNo_NONE_e)
        {
            dstStatus.setSelectItemIndex(i, selectSlot);
        }

        u8 mixSlot = srcStatus.getMixItemIndex(i);
        if (mixSlot < MAX_ITEM_SLOTS && isNewGamePlusCarrySlot(mixSlot) &&
            dstItems.getItem(mixSlot, false) != dItemNo_NONE_e)
        {
            dstStatus.setMixItemIndex(i, mixSlot);
        }
    }

    dSv_event_c& srcEvent = srcSave->getEvent();
    dSv_event_c& dstEvent = dstSave->getEvent();
    for (int i = 0; i < (int)(sizeof(hiddenSkillFlags) / sizeof(hiddenSkillFlags[0])); i++) {
        if (srcEvent.isEventBit(hiddenSkillFlags[i])) {
            dstEvent.onEventBit(hiddenSkillFlags[i]);
        } else {
            dstEvent.offEventBit(hiddenSkillFlags[i]);
        }
    }

    dstPlayer.getPlayerInfo().setPlayerName(playerName);
    dstPlayer.getPlayerInfo().setHorseName(horseName);

    u8 newGamePlusCount = new_game_plus_count(srcSave);
    if (newGamePlusCount < 0xff) {
        newGamePlusCount++;
    }
    set_new_game_plus_count(dstSave, newGamePlusCount);
    dComIfGs_setLineUpItem();

    if (isOverwriteNewGamePlus && targetSlot < 3) {
        fileSelect->mIsDataNew[targetSlot] = true;
    }

    mNewGamePlusPending = false;
    mNewGamePlusSourceSlot = 0xff;
    mNewGamePlusTargetSlot = 0xff;
}

void apply_intro_skip(dSv_save_c* save) {
    bool mSkipIntroPending = true;
    if (!mSkipIntroPending) {
        set_intro_skipped(save, false);
        return;
    }

    static const u16 introEventBits[] = {
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

    static const u8 ordonSwitches[] = {
        1, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 17, 19, 20, 23, 24, 25, 26, 27, 28, 32,
        33, 34, 35, 37, 41, 42, 43, 45, 47, 64, 66, 67, 68, 73, 84, 88, 95, 101, 103,
        104, 119,
    };
    static const u8 prisonSwitches[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 33, 34, 35, 36, 37,
    };
    static const u8 faronSwitches[] = {
        2, 3, 5, 6, 8, 9, 13, 14, 15, 16, 19, 20, 21, 22, 23, 24, 30, 33, 34, 35, 36, 37,
        38, 39, 40, 41, 42, 43, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
        59, 60, 62, 64, 67, 70, 71, 73, 75, 76, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
        89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 100, 101, 102, 103, 104, 105, 106, 107,
        108, 109, 110, 111,
    };

    for (int i = 0; i < (int)(sizeof(introEventBits) / sizeof(introEventBits[0])); i++) {
        dComIfGs_onEventBit(introEventBits[i]);
    }

    for (int i = 0; i < (int)(sizeof(ordonSwitches) / sizeof(ordonSwitches[0])); i++) {
        save->getSave(dStage_SaveTbl_ORDON).getBit().onSwitch(ordonSwitches[i]);
    }
    for (int i = 0; i < (int)(sizeof(prisonSwitches) / sizeof(prisonSwitches[0])); i++) {
        save->getSave(dStage_SaveTbl_PRISON).getBit().onSwitch(prisonSwitches[i]);
    }
    for (int i = 0; i < (int)(sizeof(faronSwitches) / sizeof(faronSwitches[0])); i++) {
        save->getSave(dStage_SaveTbl_FARON).getBit().onSwitch(faronSwitches[i]);
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

    ensureIntroSkipItem(SLOT_1, dItemNo_KANTERA_e);
    ensureIntroSkipItem(SLOT_20, dItemNo_FISHING_ROD_1_e);
    ensureIntroSkipItem(SLOT_23, dItemNo_PACHINKO_e);

    if (!hasIntroSkipBottle()) {
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
    setIntroSkipSelectItemIfEmpty(SELECT_ITEM_X, SLOT_1);
    setIntroSkipSelectItemIfEmpty(SELECT_ITEM_Y, SLOT_23);

    if (dComIfGs_getMaxLife() < 16) {
        dComIfGs_setMaxLife(16);
        dComIfGs_setLife(16);
    }

    if (dComIfGs_getRupee() < 100) {
        dComIfGs_setRupee(100);
    }

    save->getPlayer().getPlayerReturnPlace().set("F_SP108", 0, 0);
    set_intro_skipped(save, true);
    repair_intro_skip_faron_tears(save);
    dComIfGs_setLineUpItem();
    mSkipIntroPending = false;
}

void apply_boss_rush(dSv_save_c* save) {
    if (save == nullptr) {
        return;
    }
    bossrush::apply_new_save_preset();
}

}  // namespace dawnlight::game_modes
