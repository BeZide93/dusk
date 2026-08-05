#include "config.hpp"
#include "service_imports.hpp"

#include "mods/service.hpp"
#include "mods/svc/ui.h"

#include <array>

namespace dawnlight {
namespace {

UiWindowHandle s_settingsWindow = 0;
UiMenuTabHandle s_menuTab = 0;

constexpr const char* kAimModeOptions[] = {
    "Vanilla",
    "3rd Person",
    "Cinema",
};

ModResult add_section(ModContext* ctx, UiElementHandle pane, const char* title) {
    return svc_ui->pane_add_section(ctx, pane, title);
}

ModResult add_text(ModContext* ctx, UiElementHandle pane, const char* text) {
    return svc_ui->pane_add_text(ctx, pane, text, nullptr);
}

ModResult add_button(ModContext* ctx, UiElementHandle pane, const char* label,
    UiPressedFn onPressed) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_BUTTON;
    desc.label = label;
    desc.on_pressed = onPressed;
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

ModResult add_toggle(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_TOGGLE;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

ModResult add_number(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, int min, int max, int step, const char* suffix,
    const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_NUMBER;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.min = min;
    desc.max = max;
    desc.step = step;
    desc.suffix = suffix;
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

ModResult add_select(ModContext* ctx, UiElementHandle pane, const char* label,
    ConfigVarHandle var, const char* const* options, size_t optionCount,
    const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_SELECT;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.options = options;
    desc.option_count = optionCount;
    return svc_ui->pane_add_control(ctx, pane, &desc, nullptr);
}

ModResult build_aiming_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Aiming") != MOD_OK) return MOD_ERROR;
    if (add_select(ctx, left, "Aim Mode", aim_mode_config_var(), kAimModeOptions,
            std::size(kAimModeOptions),
            "Vanilla keeps the original aiming flow. 3rd Person keeps Link visible while aiming. "
            "Cinema uses Dawnlight's close over-the-shoulder camera.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Aim Movement", aim_movement_config_var(),
            "Allows movement while aiming supported items. In Vanilla aim this keeps movement on "
            "the left stick and aiming on the C-stick/touch aim.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_text(ctx, left,
            "The host-clean .dusk can adjust supported vanilla subject-aim states while they are "
            "active. Full Dawnlight camera-profile replacement still needs upstream camera and "
            "item-aim extension points.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_controls_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Controls") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Manual Shielding", manual_shielding_config_var(),
            "Moves shielding to Target + ZR and Shield Attack to Target + ZR + B. With Switch "
            "lock-on, ZR alone still shields while a target remains locked.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "R Jump", r_jump_config_var(),
            "Uses R as a fallback jump button when no R interaction or targeting action is active. "
            "Press R+B during the jump to start a jump attack.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Z Item Slot", z_item_slot_config_var(),
            "Enables the third selectable item slot and moves Midna from Z to D-Pad Down.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_text(ctx, left,
            "Wii U controller style remains outside this hook-only package. The Z item slot uses "
            "the existing Z input and can later be remapped to a dedicated Wii U layout button "
            "once upstream exposes input-layout extension points.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_gameplay_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Enemy Scaling") != MOD_OK) return MOD_ERROR;
    if (add_number(ctx, left, "HP Scaling", health_scale_config_var(), 1, 9999, 10, "%",
            "Scales enemy health when enemies spawn. New Game Plus can raise the effective value "
            "above this setting when automatic scaling is enabled.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "NG+ Auto HP Scaling", automatic_health_scale_config_var(),
            "Applies Dawnlight's NG+ health floor based on the NG+ counter.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(ctx, left, "Compatibility") != MOD_OK) return MOD_ERROR;
    if (add_toggle(ctx, left, "Save Compatibility Repairs", save_compatibility_config_var(),
            "Repairs known Dawnlight save-state issues while loading or progressing saves.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(ctx, left, "Item Integrity Fixes", item_integrity_config_var(),
            "Keeps bottle contents and item combinations from turning into invalid items.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_deferred_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(ctx, left, "Waiting For Services") != MOD_OK) return MOD_ERROR;
    if (add_text(ctx, left,
            "New Game+, Intro Skip, and Boss Rush are not enabled in this upstream-main package "
            "because upstream does not yet expose file-select and scene-flow extension points.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_text(ctx, left,
            "HUD layout import/export is not enabled here because upstream does not yet expose a "
            "HUD layout or meter-pane service compatible with hud_layout_settings.json.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_text(ctx, left,
            "Touch-specific Z/Midna layout changes and full Wii U controller style are deferred "
            "until upstream exposes input-layout and touch-control extension points.")
        != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

void settings_closed(ModContext*, UiWindowHandle, void*) {
    s_settingsWindow = 0;
}

void open_settings(ModContext* ctx, void*) {
    if (s_settingsWindow != 0) {
        return;
    }

    std::array<UiTabDesc, 4> tabs{};
    for (auto& tab : tabs) {
        tab = UI_TAB_DESC_INIT;
    }
    tabs[0].title = "Aiming";
    tabs[0].build = build_aiming_tab;
    tabs[1].title = "Controls";
    tabs[1].build = build_controls_tab;
    tabs[2].title = "Gameplay";
    tabs[2].build = build_gameplay_tab;
    tabs[3].title = "Deferred";
    tabs[3].build = build_deferred_tab;

    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs.data();
    desc.tab_count = tabs.size();
    desc.on_closed = settings_closed;
    svc_ui->window_push(ctx, &desc, &s_settingsWindow);
}

ModResult build_mod_panel(ModContext* ctx, UiElementHandle panel, void*, ModError*) {
    if (add_section(ctx, panel, "Dawnlight Settings") != MOD_OK) return MOD_ERROR;
    if (add_button(ctx, panel, "Open Dawnlight Settings", open_settings) != MOD_OK) {
        return MOD_ERROR;
    }
    return MOD_OK;
}

}  // namespace

ModResult register_ui(ModError* error) {
    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build_mod_panel;
    ModResult result = svc_ui->register_mods_panel(mod_ctx, &panel);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight mod panel");
    }

    UiMenuTabDesc tab = UI_MENU_TAB_DESC_INIT;
    tab.label = "Dawnlight";
    tab.on_selected = open_settings;
    result = svc_ui->register_menu_tab(mod_ctx, &tab, &s_menuTab);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight menu tab");
    }
    return MOD_OK;
}

}  // namespace dawnlight
