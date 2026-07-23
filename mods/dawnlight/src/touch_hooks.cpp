#include "config.hpp"
#include "touch_hooks.hpp"

#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_meter2_info.h"
#include "mods/service.hpp"
#include "mods/svc/touch_controls.h"

#include <array>
#include <cstdio>
#include <string>

IMPORT_SERVICE(TouchControlsService, svc_touch_controls);

namespace dawnlight {
namespace {

constexpr int kZItemSlot = SELECT_ITEM_DOWN;
TouchControlsProviderHandle s_touch_controls_provider = 0;
bool s_midnaTouchHeld = false;
bool s_midnaTouchTriggered = false;

std::array<char, 64> s_zItemSource{};
std::array<char, 64> s_midnaSource{};
uint64_t s_midnaRevision = 0;

bool valid_icon_item(u8 itemNo) {
    return itemNo != 0 && itemNo != dItemNo_NONE_e;
}

u8 icon_texture_item(u8 itemNo) {
    return itemNo == dItemNo_LIGHT_ARROW_e ? dItemNo_BOW_e : itemNo;
}

uint32_t item_icon_revision(u8 itemNo) {
    itemNo = icon_texture_item(itemNo);
    uint32_t revision = itemNo;
    revision = revision * 131u + g_meter2_info.getItemType(itemNo);
    if (itemNo == dItemNo_KANTERA_e || itemNo == dItemNo_KANTERA2_e) {
        revision = revision * 131u + (dComIfGs_getOil() == 0 ? 0u : 1u);
    }
    return revision;
}

const char* z_item_icon_source(u8 itemNo) {
    if (!valid_icon_item(itemNo)) {
        s_zItemSource[0] = '\0';
        return s_zItemSource.data();
    }

    std::snprintf(s_zItemSource.data(), s_zItemSource.size(), "item://item/%02x?rev=%08x",
        icon_texture_item(itemNo), item_icon_revision(itemNo));
    return s_zItemSource.data();
}

const char* midna_meter_source() {
    std::snprintf(s_midnaSource.data(), s_midnaSource.size(), "meter://midna?slot=%llu",
        static_cast<unsigned long long>(s_midnaRevision % 8u));
    ++s_midnaRevision;
    return s_midnaSource.data();
}

int display_override(
    ModContext*, int32_t control, TouchControlsDisplayOverride* out, void*) {
    if (!z_item_slot_enabled() || out == nullptr) {
        return 0;
    }

    if (control == DUSK_MOD_TOUCH_CONTROL_Z) {
        const bool itemMode = dComIfGp_getLinkPlayer() != nullptr && daPy_py_c::checkNowWolf() == 0;
        const u8 itemNo = dComIfGp_getSelectItem(kZItemSlot);
        const char* source = itemMode ? z_item_icon_source(itemNo) : "";
        *out = {
            .struct_size = sizeof(TouchControlsDisplayOverride),
            .icon_source = source,
            .icon_revision = itemMode && valid_icon_item(itemNo) ? item_icon_revision(itemNo) : 0,
            .visible = 1,
            .show_icon = source[0] != '\0',
        };
        return 1;
    }

    if (control == DUSK_MOD_TOUCH_CONTROL_DPAD_DOWN) {
        const char* source = midna_meter_source();
        *out = {
            .struct_size = sizeof(TouchControlsDisplayOverride),
            .icon_source = source,
            .icon_revision = s_midnaRevision,
            .visible = 1,
            .show_icon = 1,
        };
        return 1;
    }

    return 0;
}

uint16_t pad_button(ModContext*, int32_t control, uint16_t fallback, void*) {
    if (z_item_slot_enabled() && control == DUSK_MOD_TOUCH_CONTROL_DPAD_DOWN) {
        return 0;
    }
    return fallback;
}

void control_event(ModContext*, int32_t control, int pressed, void*) {
    if (!z_item_slot_enabled() || control != DUSK_MOD_TOUCH_CONTROL_DPAD_DOWN) {
        return;
    }

    const bool isPressed = pressed != 0;
    if (isPressed && !s_midnaTouchHeld) {
        s_midnaTouchTriggered = true;
    }
    s_midnaTouchHeld = isPressed;
}

int32_t aim_input_mode(ModContext*, void*) {
    return aim_movement_enabled() ? DUSK_MOD_TOUCH_AIM_INPUT_SPLIT_STICKS :
                                   DUSK_MOD_TOUCH_AIM_INPUT_DEFAULT;
}

}  // namespace

bool consume_touch_midna_trigger() {
    if (!s_midnaTouchTriggered) {
        return false;
    }

    s_midnaTouchTriggered = false;
    return true;
}

ModResult install_touch_hooks(ModError* error) {
    TouchControlsProviderDesc desc = TOUCH_CONTROLS_PROVIDER_DESC_INIT;
    desc.display_override = display_override;
    desc.pad_button = pad_button;
    desc.control_event = control_event;
    desc.aim_input_mode = aim_input_mode;

    const ModResult result =
        svc_touch_controls->register_provider(mod_ctx, &desc, &s_touch_controls_provider);
    if (result != MOD_OK) {
        return mods::set_error(
            error, result, "failed to register Dawnlight touch-controls provider");
    }
    return MOD_OK;
}

}  // namespace dawnlight
