#include "config.hpp"

#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_meter2_info.h"
#include "dusk/ui/touch_control_hooks.hpp"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

IMPORT_SERVICE(HookService, svc_hook);

namespace dawnlight {
namespace {

constexpr int kZItemSlot = SELECT_ITEM_DOWN;
const std::array<dusk::ui::TouchLayoutControlInfo, 1> kExtraTouchControls = {{
    {
        .layoutId = "dpadDown",
        .elementId = "dpad-down",
        .props =
            {
                .x = 176.f,
                .y = 76.f,
                .w = 54.f,
                .h = 54.f,
                .scale = 1.f,
                .anchor = dusk::ui::ControlAnchor::BottomLeft,
            },
        .control = dusk::ui::Control::DPAD_DOWN,
        .hasControl = true,
    },
}};

std::array<char, 64> s_zItemSource{};
std::array<char, 64> s_midnaSource{};
uint64_t s_midnaRevision = 0;

DEFINE_HOOK_SYMBOL("dusk::ui::touch_layout_extra_control_count", std::size_t(),
    TouchExtraControlCountHook);
DEFINE_HOOK_SYMBOL("dusk::ui::touch_layout_extra_control_at",
    const dusk::ui::TouchLayoutControlInfo*(std::size_t), TouchExtraControlAtHook);
DEFINE_HOOK_SYMBOL("dusk::ui::touch_control_display_override",
    bool(dusk::ui::Control, dusk::ui::TouchControlDisplayOverride*), TouchDisplayOverrideHook);

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

HookAction before_extra_control_count(ModContext*, void*, void* retval, void*) {
    if (!z_item_slot_enabled()) {
        return HOOK_CONTINUE;
    }

    *static_cast<std::size_t*>(retval) = kExtraTouchControls.size();
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_extra_control_at(ModContext*, void* args, void* retval, void*) {
    if (!z_item_slot_enabled()) {
        return HOOK_CONTINUE;
    }

    const std::size_t index = mods::arg<std::size_t>(args, 0);
    *static_cast<const dusk::ui::TouchLayoutControlInfo**>(retval) =
        index < kExtraTouchControls.size() ? &kExtraTouchControls[index] : nullptr;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_display_override(ModContext*, void* args, void* retval, void*) {
    const auto control = mods::arg<dusk::ui::Control>(args, 0);
    auto* out = mods::arg<dusk::ui::TouchControlDisplayOverride*>(args, 1);
    if (!z_item_slot_enabled() || out == nullptr) {
        return HOOK_CONTINUE;
    }

    if (control == dusk::ui::Control::Z) {
        const bool itemMode = dComIfGp_getLinkPlayer() != nullptr && daPy_py_c::checkNowWolf() == 0;
        const u8 itemNo = dComIfGp_getSelectItem(kZItemSlot);
        const char* source = itemMode ? z_item_icon_source(itemNo) : "";
        *out = {
            .iconSource = source,
            .iconRevision = itemMode && valid_icon_item(itemNo) ? item_icon_revision(itemNo) : 0,
            .visible = true,
            .showIcon = source[0] != '\0',
        };
        *static_cast<bool*>(retval) = true;
        return HOOK_SKIP_ORIGINAL;
    }

    if (control == dusk::ui::Control::DPAD_DOWN) {
        const char* source = midna_meter_source();
        *out = {
            .iconSource = source,
            .iconRevision = s_midnaRevision,
            .visible = true,
            .showIcon = true,
        };
        *static_cast<bool*>(retval) = true;
        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
}

ModResult add_hook(ModResult result, ModError* error) {
    return result == MOD_OK ? MOD_OK :
        mods::set_error(error, result, "failed to install Dawnlight touch control hooks");
}

}  // namespace

ModResult install_touch_hooks(ModError* error) {
    ModResult result = mods::hook_add_pre<TouchExtraControlCountHook>(svc_hook, before_extra_control_count);
    if (result == MOD_OK) {
        result = mods::hook_add_pre<TouchExtraControlAtHook>(svc_hook, before_extra_control_at);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<TouchDisplayOverrideHook>(svc_hook, before_display_override);
    }
    return add_hook(result, error);
}

}  // namespace dawnlight
