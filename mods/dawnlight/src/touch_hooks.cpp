#include "config.hpp"

#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_meter2_info.h"
#include "mods/service.hpp"
#include "mods/svc/touch_controls.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

IMPORT_SERVICE(TouchControlsService, svc_touch_controls);

namespace dawnlight {
namespace {

constexpr int kZItemSlot = SELECT_ITEM_DOWN;
TouchControlsProviderHandle s_touch_controls_provider = 0;

constexpr const char* kExtraTouchControlsRml = R"RML(
    <button id="dpad-down" class="control trigger button-z"><img id="dpad-down-icon" class="midna-icon" /><span>Down</span></button>
)RML";

const std::array<TouchControlsControlDesc, 1> kExtraTouchControls = {{
    {
        .struct_size = sizeof(TouchControlsControlDesc),
        .layout_id = "dpadDown",
        .element_id = "dpad-down",
        .x = 176.f,
        .y = 76.f,
        .w = 54.f,
        .h = 54.f,
        .scale = 1.f,
        .anchor = DUSK_MOD_TOUCH_ANCHOR_BOTTOM_LEFT,
        .control = DUSK_MOD_TOUCH_CONTROL_DPAD_DOWN,
        .has_control = 1,
    },
}};

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

const char* extra_rml(ModContext*, void*) {
    return kExtraTouchControlsRml;
}

std::size_t extra_control_count(ModContext*, void*) {
    return z_item_slot_enabled() ? kExtraTouchControls.size() : 0;
}

int extra_control_at(ModContext*, std::size_t index, TouchControlsControlDesc* out, void*) {
    if (!z_item_slot_enabled() || out == nullptr || index >= kExtraTouchControls.size()) {
        return 0;
    }

    *out = kExtraTouchControls[index];
    return 1;
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

}  // namespace

ModResult install_touch_hooks(ModError* error) {
    TouchControlsProviderDesc desc = TOUCH_CONTROLS_PROVIDER_DESC_INIT;
    desc.extra_rml = extra_rml;
    desc.extra_control_count = extra_control_count;
    desc.extra_control_at = extra_control_at;
    desc.display_override = display_override;

    const ModResult result =
        svc_touch_controls->register_provider(mod_ctx, &desc, &s_touch_controls_provider);
    if (result != MOD_OK) {
        return mods::set_error(
            error, result, "failed to register Dawnlight touch-controls provider");
    }
    return MOD_OK;
}

}  // namespace dawnlight
