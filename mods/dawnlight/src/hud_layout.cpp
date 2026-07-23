#include "hud_layout.hpp"

#include "mods/service.hpp"
#include "mods/svc/config.h"
#include "mods/svc/hud_layout.h"
#include "mods/svc/log.h"
#include "mods/svc/ui.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(HudLayoutService, svc_hud_layout);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(UiService, svc_ui);

namespace dawnlight {
namespace {

enum HudElement : size_t {
    kElementA = 0,
    kElementB,
    kElementX,
    kElementY,
    kElementZ,
    kElementHearts,
    kElementRupees,
    kElementKeys,
    kElementOil,
    kElementOxygen,
    kElementDPad,
    kElementMinimap,
    kElementButtonBacking,
    kElementMidna,
    kElementCount,
};

enum HudButton : size_t {
    kButtonA = 0,
    kButtonB,
    kButtonX,
    kButtonY,
    kButtonZ,
    kButtonCount,
};

static_assert(static_cast<size_t>(kElementCount) == DUSK_MOD_HUD_ELEMENT_COUNT);
static_assert(static_cast<size_t>(kButtonCount) == DUSK_MOD_HUD_BUTTON_COUNT);

struct ElementVars {
    ConfigVarHandle x = 0;
    ConfigVarHandle y = 0;
    ConfigVarHandle scale = 0;
};

struct ButtonVars {
    ConfigVarHandle itemAnchor = 0;
    ConfigVarHandle textAnchor = 0;
    ConfigVarHandle itemScale = 0;
    ConfigVarHandle itemOffsetX = 0;
    ConfigVarHandle itemOffsetY = 0;
    ConfigVarHandle ammoOffsetX = 0;
    ConfigVarHandle ammoOffsetY = 0;
    ConfigVarHandle ammoScale = 0;
    ConfigVarHandle textScale = 0;
    ConfigVarHandle textOffsetX = 0;
    ConfigVarHandle textOffsetY = 0;
};

constexpr std::array<const char*, kElementCount> kElementLabels = {
    "A",
    "B",
    "X",
    "Y",
    "Z",
    "Hearts",
    "Rupees",
    "Keys",
    "Oil",
    "Oxygen",
    "D-Pad",
    "Minimap",
    "Button Backing",
    "Midna",
};

constexpr std::array<const char*, kElementCount> kElementConfigKeys = {
    "a",
    "b",
    "x",
    "y",
    "z",
    "hearts",
    "rupees",
    "keys",
    "oil",
    "oxygen",
    "dpad",
    "minimap",
    "button-backing",
    "midna",
};

constexpr std::array<const char*, kButtonCount> kButtonLabels = {
    "A",
    "B",
    "X",
    "Y",
    "Z",
};

constexpr std::array<const char*, kButtonCount> kButtonKeys = {
    "a",
    "b",
    "x",
    "y",
    "z",
};

constexpr std::array<const char*, 4> kItemAnchorOptions = {
    "Left",
    "Right",
    "Top",
    "Bottom",
};

constexpr std::array<const char*, 2> kTextAnchorOptions = {
    "Left",
    "Right",
};

constexpr std::array<const char*, 2> kMinimapSlideOptions = {
    "Left -> Right",
    "Right -> Left",
};

ConfigVarHandle s_background = 0;
ConfigVarHandle s_roundXYButtons = 0;
ConfigVarHandle s_selectedElement = 0;
ConfigVarHandle s_dpadFollowsMinimap = 0;
ConfigVarHandle s_minimapSlideDirection = 0;
std::array<ElementVars, kElementCount> s_elements{};
std::array<ButtonVars, kButtonCount> s_buttons{};
HudLayoutProviderHandle s_hudLayoutProvider = 0;
DuskModHudLayoutSnapshot s_layout{};
uint32_t s_nextRevision = 1;
bool s_dirty = true;
std::string s_dataPath;

int clamp_int(const int value, const int min, const int max) {
    return std::clamp(value, min, max);
}

float scale_ratio_from_percent(const int value) {
    return static_cast<float>(clamp_int(value, 1, 9999)) / 100.0f;
}

int percent_from_scale(const double value) {
    return clamp_int(static_cast<int>(std::lround(value * 100.0)), 1, 9999);
}

ModResult set_error(ModError* error, const ModResult code, const char* message) {
    if (error != nullptr) {
        error->code = code;
        std::snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return code;
}

ModResult register_bool(const char* name, const bool defaultValue, ConfigVarHandle& handle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_BOOL;
    desc.default_bool = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &handle);
}

ModResult register_int(const char* name, const int64_t defaultValue, ConfigVarHandle& handle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_INT;
    desc.default_int = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &handle);
}

bool get_bool(const ConfigVarHandle handle, const bool fallback) {
    bool value = fallback;
    if (handle != 0) {
        svc_config->get_bool(mod_ctx, handle, &value);
    }
    return value;
}

int get_int(const ConfigVarHandle handle, const int fallback) {
    int64_t value = fallback;
    if (handle != 0) {
        svc_config->get_int(mod_ctx, handle, &value);
    }
    return static_cast<int>(value);
}

void set_bool(const ConfigVarHandle handle, const bool value) {
    if (handle != 0) {
        svc_config->set_bool(mod_ctx, handle, value);
    }
}

void set_int(const ConfigVarHandle handle, const int value) {
    if (handle != 0) {
        svc_config->set_int(mod_ctx, handle, value);
    }
}

int default_item_anchor(const HudButton button) {
    return button == kButtonY ? 0 : 1;
}

int selected_element() {
    return clamp_int(get_int(s_selectedElement, 0), 0, static_cast<int>(kElementCount - 1));
}

bool is_button_element(const int element) {
    return element >= static_cast<int>(kElementA) && element <= static_cast<int>(kElementZ);
}

HudButton button_from_element(const int element) {
    return static_cast<HudButton>(clamp_int(element, 0, static_cast<int>(kButtonCount - 1)));
}

bool has_item_layout(const int element) {
    return element == static_cast<int>(kElementB) || element == static_cast<int>(kElementX) ||
           element == static_cast<int>(kElementY) || element == static_cast<int>(kElementZ);
}

bool has_ammo_layout(const int element) {
    return element == static_cast<int>(kElementX) || element == static_cast<int>(kElementY) ||
           element == static_cast<int>(kElementZ);
}

bool has_text_anchor(const int element) {
    return element == static_cast<int>(kElementA) || element == static_cast<int>(kElementB) ||
           element == static_cast<int>(kElementX) || element == static_cast<int>(kElementY);
}

bool has_text_layout(const int element) {
    return is_button_element(element);
}

bool is_minimap(const int element) {
    return element == static_cast<int>(kElementMinimap);
}

bool is_disabled_false(ModContext*, void*) {
    return false;
}

bool is_item_layout_disabled(ModContext*, void*) {
    return !has_item_layout(selected_element());
}

bool is_ammo_layout_disabled(ModContext*, void*) {
    return !has_ammo_layout(selected_element());
}

bool is_text_anchor_disabled(ModContext*, void*) {
    return !has_text_anchor(selected_element());
}

bool is_text_layout_disabled(ModContext*, void*) {
    return !has_text_layout(selected_element());
}

bool is_minimap_disabled(ModContext*, void*) {
    return !is_minimap(selected_element());
}

void mark_dirty(ModContext*, ConfigVarHandle, const ConfigVarValue*, const ConfigVarValue*, void*) {
    s_dirty = true;
}

void subscribe(ConfigVarHandle handle) {
    if (handle != 0) {
        svc_config->subscribe(mod_ctx, handle, mark_dirty, nullptr, nullptr);
    }
}

void subscribe_all() {
    subscribe(s_background);
    subscribe(s_roundXYButtons);
    subscribe(s_dpadFollowsMinimap);
    subscribe(s_minimapSlideDirection);
    for (const auto& element : s_elements) {
        subscribe(element.x);
        subscribe(element.y);
        subscribe(element.scale);
    }
    for (const auto& button : s_buttons) {
        subscribe(button.itemAnchor);
        subscribe(button.textAnchor);
        subscribe(button.itemScale);
        subscribe(button.itemOffsetX);
        subscribe(button.itemOffsetY);
        subscribe(button.ammoOffsetX);
        subscribe(button.ammoOffsetY);
        subscribe(button.ammoScale);
        subscribe(button.textScale);
        subscribe(button.textOffsetX);
        subscribe(button.textOffsetY);
    }
}

std::string config_name(std::string_view prefix, std::string_view key, std::string_view suffix) {
    std::string name;
    name.reserve(prefix.size() + key.size() + suffix.size() + 2);
    name.append(prefix);
    name.push_back('-');
    name.append(key);
    name.push_back('-');
    name.append(suffix);
    return name;
}

ModResult register_hud_config(ModError* error) {
    if (register_bool("hud-backing-texture", true, s_background) != MOD_OK ||
        register_bool("round-xy-buttons", false, s_roundXYButtons) != MOD_OK ||
        register_int("selected-element", 0, s_selectedElement) != MOD_OK ||
        register_bool("dpad-follows-minimap", true, s_dpadFollowsMinimap) != MOD_OK ||
        register_int("minimap-slide-direction", 0, s_minimapSlideDirection) != MOD_OK)
    {
        return set_error(error, MOD_ERROR, "failed to register HUD editing config variables");
    }

    for (size_t i = 0; i < kElementCount; i++) {
        auto& element = s_elements[i];
        if (register_int(config_name("element", kElementConfigKeys[i], "x").c_str(), 0, element.x) !=
                MOD_OK ||
            register_int(config_name("element", kElementConfigKeys[i], "y").c_str(), 0, element.y) !=
                MOD_OK ||
            register_int(
                config_name("element", kElementConfigKeys[i], "scale").c_str(), 100, element.scale) !=
                MOD_OK)
        {
            return set_error(error, MOD_ERROR, "failed to register HUD element config variables");
        }
    }

    for (size_t i = 0; i < kButtonCount; i++) {
        auto& button = s_buttons[i];
        const auto buttonId = static_cast<HudButton>(i);
        if (register_int(config_name("button", kButtonKeys[i], "item-anchor").c_str(),
                default_item_anchor(buttonId), button.itemAnchor) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "text-anchor").c_str(), 0,
                button.textAnchor) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "item-scale").c_str(), 100,
                button.itemScale) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "item-offset-x").c_str(), 0,
                button.itemOffsetX) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "item-offset-y").c_str(), 0,
                button.itemOffsetY) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "ammo-offset-x").c_str(), 0,
                button.ammoOffsetX) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "ammo-offset-y").c_str(), 0,
                button.ammoOffsetY) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "ammo-scale").c_str(), 100,
                button.ammoScale) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "text-scale").c_str(), 100,
                button.textScale) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "text-offset-x").c_str(), 0,
                button.textOffsetX) != MOD_OK ||
            register_int(config_name("button", kButtonKeys[i], "text-offset-y").c_str(), 0,
                button.textOffsetY) != MOD_OK)
        {
            return set_error(error, MOD_ERROR, "failed to register HUD button config variables");
        }
    }

    subscribe_all();
    s_dirty = true;
    return MOD_OK;
}

void reset_button(const HudButton button) {
    auto& vars = s_buttons[static_cast<size_t>(button)];
    set_int(vars.itemAnchor, default_item_anchor(button));
    set_int(vars.textAnchor, 0);
    set_int(vars.itemScale, 100);
    set_int(vars.itemOffsetX, 0);
    set_int(vars.itemOffsetY, 0);
    set_int(vars.ammoOffsetX, 0);
    set_int(vars.ammoOffsetY, 0);
    set_int(vars.ammoScale, 100);
    set_int(vars.textScale, 100);
    set_int(vars.textOffsetX, 0);
    set_int(vars.textOffsetY, 0);
}

void reset_element(const int elementIndex) {
    const auto element = static_cast<size_t>(
        clamp_int(elementIndex, 0, static_cast<int>(kElementCount - 1)));
    set_int(s_elements[element].x, 0);
    set_int(s_elements[element].y, 0);
    set_int(s_elements[element].scale, 100);
    if (is_button_element(static_cast<int>(element))) {
        reset_button(button_from_element(static_cast<int>(element)));
    }
    if (is_minimap(static_cast<int>(element))) {
        set_bool(s_dpadFollowsMinimap, true);
        set_int(s_minimapSlideDirection, 0);
    }
    s_dirty = true;
}

void reset_layout() {
    set_bool(s_background, true);
    set_bool(s_roundXYButtons, false);
    set_bool(s_dpadFollowsMinimap, true);
    set_int(s_minimapSlideDirection, 0);
    for (int i = 0; i < static_cast<int>(kElementCount); i++) {
        reset_element(i);
    }
    s_dirty = true;
}

const char* anchor_name(const int anchor, const std::array<const char*, 4>& names) {
    return names[static_cast<size_t>(clamp_int(anchor, 0, static_cast<int>(names.size() - 1)))];
}

const char* text_anchor_name(const int anchor) {
    return kTextAnchorOptions[static_cast<size_t>(clamp_int(anchor, 0, 1))];
}

int item_anchor_from_json(const nlohmann::json& object, const char* key, const int fallback) {
    const auto it = object.find(key);
    if (it == object.end()) {
        return fallback;
    }
    if (it->is_number_integer()) {
        return clamp_int(it->get<int>(), 0, 3);
    }
    if (!it->is_string()) {
        return fallback;
    }
    const std::string value = it->get<std::string>();
    for (size_t i = 0; i < kItemAnchorOptions.size(); i++) {
        if (value == kItemAnchorOptions[i]) {
            return static_cast<int>(i);
        }
    }
    return fallback;
}

int text_anchor_from_json(const nlohmann::json& object, const char* key, const int fallback) {
    const auto it = object.find(key);
    if (it == object.end()) {
        return fallback;
    }
    if (it->is_number_integer()) {
        return clamp_int(it->get<int>(), 0, 1);
    }
    if (!it->is_string()) {
        return fallback;
    }
    const std::string value = it->get<std::string>();
    for (size_t i = 0; i < kTextAnchorOptions.size(); i++) {
        if (value == kTextAnchorOptions[i]) {
            return static_cast<int>(i);
        }
    }
    return fallback;
}

int slide_direction_from_json(const nlohmann::json& object, const char* key, const int fallback) {
    const auto it = object.find(key);
    if (it == object.end()) {
        return fallback;
    }
    if (it->is_number_integer()) {
        return clamp_int(it->get<int>(), 0, 1);
    }
    if (!it->is_string()) {
        return fallback;
    }
    const std::string value = it->get<std::string>();
    return value == "Right -> Left" ? 1 : 0;
}

int json_int(const nlohmann::json& object, const char* key, const int fallback, const int min,
    const int max) {
    const auto it = object.find(key);
    if (it == object.end() || !it->is_number()) {
        return fallback;
    }
    return clamp_int(static_cast<int>(std::lround(it->get<double>())), min, max);
}

bool json_bool(const nlohmann::json& object, const char* key, const bool fallback) {
    const auto it = object.find(key);
    return it != object.end() && it->is_boolean() ? it->get<bool>() : fallback;
}

void import_element_json(const nlohmann::json& elements, const int element) {
    const auto found = elements.find(kElementLabels[static_cast<size_t>(element)]);
    if (found == elements.end() || !found->is_object()) {
        return;
    }

    const auto& object = *found;
    auto& vars = s_elements[static_cast<size_t>(element)];
    set_int(vars.x, json_int(object, "x", get_int(vars.x, 0), -9999, 9999));
    set_int(vars.y, json_int(object, "y", get_int(vars.y, 0), -9999, 9999));
    const auto scaleIt = object.find("scale");
    if (scaleIt != object.end() && scaleIt->is_number()) {
        set_int(vars.scale, percent_from_scale(scaleIt->get<double>()));
    }

    if (is_button_element(element)) {
        auto& button = s_buttons[static_cast<size_t>(button_from_element(element))];
        set_int(button.itemAnchor,
            item_anchor_from_json(object, "itemAnchor", get_int(button.itemAnchor, 0)));
        set_int(button.textAnchor,
            text_anchor_from_json(object, "textAnchor", get_int(button.textAnchor, 0)));
        const auto itemScaleIt = object.find("itemScale");
        if (itemScaleIt != object.end() && itemScaleIt->is_number()) {
            set_int(button.itemScale, percent_from_scale(itemScaleIt->get<double>()));
        }
        set_int(button.itemOffsetX,
            json_int(object, "itemOffsetX", get_int(button.itemOffsetX, 0), -9999, 9999));
        set_int(button.itemOffsetY,
            json_int(object, "itemOffsetY", get_int(button.itemOffsetY, 0), -9999, 9999));
        set_int(button.ammoOffsetX,
            json_int(object, "ammoOffsetX", get_int(button.ammoOffsetX, 0), -9999, 9999));
        set_int(button.ammoOffsetY,
            json_int(object, "ammoOffsetY", get_int(button.ammoOffsetY, 0), -9999, 9999));
        const auto ammoScaleIt = object.find("ammoScale");
        if (ammoScaleIt != object.end() && ammoScaleIt->is_number()) {
            set_int(button.ammoScale, percent_from_scale(ammoScaleIt->get<double>()));
        }
        const auto textScaleIt = object.find("textScale");
        if (textScaleIt != object.end() && textScaleIt->is_number()) {
            set_int(button.textScale, percent_from_scale(textScaleIt->get<double>()));
        }
        set_int(button.textOffsetX,
            json_int(object, "textOffsetX", get_int(button.textOffsetX, 0), -9999, 9999));
        set_int(button.textOffsetY,
            json_int(object, "textOffsetY", get_int(button.textOffsetY, 0), -9999, 9999));
    }

    if (is_minimap(element)) {
        set_int(s_minimapSlideDirection,
            slide_direction_from_json(object, "slideDirection", get_int(s_minimapSlideDirection, 0)));
        set_bool(s_dpadFollowsMinimap,
            json_bool(object, "dpadFollowsMinimap", get_bool(s_dpadFollowsMinimap, true)));
    }
}

nlohmann::json element_to_json(const int element) {
    const auto& vars = s_elements[static_cast<size_t>(element)];
    nlohmann::json object = {
        {"x", get_int(vars.x, 0)},
        {"y", get_int(vars.y, 0)},
        {"scale", scale_ratio_from_percent(get_int(vars.scale, 100))},
    };

    if (is_minimap(element)) {
        object["slideDirection"] =
            kMinimapSlideOptions[static_cast<size_t>(clamp_int(
                get_int(s_minimapSlideDirection, 0), 0, 1))];
        object["dpadFollowsMinimap"] = get_bool(s_dpadFollowsMinimap, true);
    }

    if (is_button_element(element)) {
        const auto buttonId = button_from_element(element);
        const auto& button = s_buttons[static_cast<size_t>(buttonId)];
        if (has_item_layout(element)) {
            object["itemAnchor"] = anchor_name(get_int(button.itemAnchor, default_item_anchor(buttonId)),
                kItemAnchorOptions);
            object["itemScale"] = scale_ratio_from_percent(get_int(button.itemScale, 100));
            object["itemOffsetX"] = get_int(button.itemOffsetX, 0);
            object["itemOffsetY"] = get_int(button.itemOffsetY, 0);
        }
        if (has_ammo_layout(element)) {
            object["ammoOffsetX"] = get_int(button.ammoOffsetX, 0);
            object["ammoOffsetY"] = get_int(button.ammoOffsetY, 0);
            object["ammoScale"] = scale_ratio_from_percent(get_int(button.ammoScale, 100));
        }
        if (has_text_anchor(element)) {
            object["textAnchor"] = text_anchor_name(get_int(button.textAnchor, 0));
        }
        if (has_text_layout(element)) {
            object["textScale"] = scale_ratio_from_percent(get_int(button.textScale, 100));
        }
        if (has_text_anchor(element)) {
            object["textOffsetX"] = get_int(button.textOffsetX, 0);
            object["textOffsetY"] = get_int(button.textOffsetY, 0);
        }
    }

    return object;
}

nlohmann::json export_layout_json() {
    nlohmann::json elements = nlohmann::json::object();
    for (int i = 0; i < static_cast<int>(kElementCount); i++) {
        elements[kElementLabels[static_cast<size_t>(i)]] = element_to_json(i);
    }
    return {
        {"version", 10},
        {"background", get_bool(s_background, true)},
        {"roundXYButtons", get_bool(s_roundXYButtons, false)},
        {"elements", std::move(elements)},
    };
}

bool import_layout_json(const nlohmann::json& root) {
    if (!root.is_object()) {
        return false;
    }

    const nlohmann::json* hud = &root;
    const auto nested = root.find("hudButtons");
    if (nested != root.end()) {
        if (!nested->is_object()) {
            return false;
        }
        hud = &*nested;
    }

    if (!hud->contains("background") && !hud->contains("buttons") && !hud->contains("elements")) {
        return false;
    }

    set_bool(s_background, json_bool(*hud, "background", get_bool(s_background, true)));
    set_bool(s_roundXYButtons,
        json_bool(*hud, "roundXYButtons", get_bool(s_roundXYButtons, false)));

    const auto buttons = hud->find("buttons");
    if (buttons != hud->end() && buttons->is_object()) {
        for (int i = 0; i < static_cast<int>(kButtonCount); i++) {
            import_element_json(*buttons, i);
        }
    }

    const auto elements = hud->find("elements");
    if (elements != hud->end() && elements->is_object()) {
        for (int i = 0; i < static_cast<int>(kElementCount); i++) {
            import_element_json(*elements, i);
        }
    }
    s_dirty = true;
    return true;
}

std::filesystem::path path_from_utf8(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return {};
    }
#if defined(_WIN32)
    return std::filesystem::path{reinterpret_cast<const char8_t*>(text)};
#else
    return std::filesystem::path{text};
#endif
}

std::filesystem::path layout_file_path() {
    if (s_dataPath.empty()) {
        return {};
    }
    return path_from_utf8(s_dataPath.c_str()) / "hud_layout_settings.json";
}

std::string path_display(const std::filesystem::path& path) {
#if defined(_WIN32)
    const auto text = path.u8string();
    return reinterpret_cast<const char*>(text.c_str());
#else
    return path.string();
#endif
}

void show_dialog(const char* title, const std::string& body, const UiDialogVariant variant) {
    UiDialogAction action{
        .label = "OK",
        .on_pressed = nullptr,
        .user_data = nullptr,
        .keep_open = false,
    };
    UiDialogDesc desc = UI_DIALOG_DESC_INIT;
    desc.title = title;
    desc.body_rml = body.c_str();
    desc.variant = variant;
    desc.actions = &action;
    desc.action_count = 1;
    svc_ui->dialog_push(mod_ctx, &desc, nullptr);
}

void export_to_default_file(ModContext*, void*) {
    const auto path = layout_file_path();
    if (path.empty()) {
        show_dialog("HUD Layout Export Failed",
            "No active data folder is known yet. Open a save once, then export again.",
            UI_DIALOG_WARNING);
        return;
    }

    try {
        std::ofstream stream(path);
        if (!stream.is_open()) {
            show_dialog("HUD Layout Export Failed", "Could not open hud_layout_settings.json.",
                UI_DIALOG_WARNING);
            return;
        }
        stream << export_layout_json().dump(4) << '\n';
        show_dialog("HUD Layout Exported",
            "Wrote hud_layout_settings.json to:<br/>" + path_display(path), UI_DIALOG_NORMAL);
    } catch (const std::exception& e) {
        show_dialog("HUD Layout Export Failed", e.what(), UI_DIALOG_WARNING);
    }
}

void import_from_default_file(ModContext*, void*) {
    const auto path = layout_file_path();
    if (path.empty()) {
        show_dialog("HUD Layout Import Failed",
            "No active data folder is known yet. Open a save once, then import again.",
            UI_DIALOG_WARNING);
        return;
    }

    try {
        std::ifstream stream(path);
        if (!stream.is_open()) {
            show_dialog("HUD Layout Import Failed",
                "Could not open hud_layout_settings.json in the active data folder.",
                UI_DIALOG_WARNING);
            return;
        }
        const auto json = nlohmann::json::parse(stream, nullptr, false);
        if (json.is_discarded() || !import_layout_json(json)) {
            show_dialog("HUD Layout Import Failed",
                "The selected hud_layout_settings.json has no matching HUD layout data.",
                UI_DIALOG_WARNING);
            return;
        }
        show_dialog("HUD Layout Imported",
            "Loaded hud_layout_settings.json from:<br/>" + path_display(path), UI_DIALOG_NORMAL);
    } catch (const std::exception& e) {
        show_dialog("HUD Layout Import Failed", e.what(), UI_DIALOG_WARNING);
    }
}

void rebuild_layout() {
    s_layout = {};
    s_layout.struct_size = sizeof(DuskModHudLayoutSnapshot);
    s_layout.revision = s_nextRevision++;

    for (size_t i = 0; i < kElementCount; i++) {
        auto& transform = s_layout.elements[i];
        transform.offset_x = static_cast<float>(get_int(s_elements[i].x, 0));
        transform.offset_y = static_cast<float>(get_int(s_elements[i].y, 0));
        transform.scale = scale_ratio_from_percent(get_int(s_elements[i].scale, 100));
    }

    if (!get_bool(s_background, true)) {
        s_layout.elements[kElementButtonBacking].flags |= DUSK_MOD_HUD_ELEMENT_HIDDEN;
    }
    if (!get_bool(s_dpadFollowsMinimap, true)) {
        s_layout.elements[kElementDPad].parent_mode = DUSK_MOD_HUD_PARENT_INDEPENDENT;
    }
    if (get_int(s_minimapSlideDirection, 0) == 1) {
        s_layout.elements[kElementMinimap].slide_direction = DUSK_MOD_HUD_SLIDE_RIGHT_TO_LEFT;
    } else {
        s_layout.elements[kElementMinimap].slide_direction = DUSK_MOD_HUD_SLIDE_LEFT_TO_RIGHT;
    }

    for (size_t i = 0; i < kButtonCount; i++) {
        const auto buttonId = static_cast<HudButton>(i);
        const auto& vars = s_buttons[i];
        auto& button = s_layout.buttons[i];
        button.item_anchor = clamp_int(get_int(vars.itemAnchor, default_item_anchor(buttonId)), 0, 3);
        button.text_anchor = clamp_int(get_int(vars.textAnchor, 0), 0, 1);
        button.item_scale = scale_ratio_from_percent(get_int(vars.itemScale, 100));
        button.item_offset_x = static_cast<float>(get_int(vars.itemOffsetX, 0));
        button.item_offset_y = static_cast<float>(get_int(vars.itemOffsetY, 0));
        button.ammo_offset_x = static_cast<float>(get_int(vars.ammoOffsetX, 0));
        button.ammo_offset_y = static_cast<float>(get_int(vars.ammoOffsetY, 0));
        button.ammo_scale = scale_ratio_from_percent(get_int(vars.ammoScale, 100));
        button.text_scale = scale_ratio_from_percent(get_int(vars.textScale, 100));
        button.text_offset_x = static_cast<float>(get_int(vars.textOffsetX, 0));
        button.text_offset_y = static_cast<float>(get_int(vars.textOffsetY, 0));
    }

    if (get_bool(s_roundXYButtons, false)) {
        s_layout.buttons[kButtonX].style_flags |= DUSK_MOD_HUD_BUTTON_STYLE_ROUND;
        s_layout.buttons[kButtonY].style_flags |= DUSK_MOD_HUD_BUTTON_STYLE_ROUND;
    }
    s_dirty = false;
}

const DuskModHudLayoutSnapshot* get_layout(ModContext*, const char* dataPath, void*) {
    if (dataPath != nullptr && dataPath[0] != '\0' && s_dataPath != dataPath) {
        s_dataPath = dataPath;
    }
    if (s_dirty) {
        rebuild_layout();
    }
    return &s_layout;
}

ModResult add_section(UiElementHandle pane, const char* title) {
    return svc_ui->pane_add_section(mod_ctx, pane, title);
}

ModResult add_text(UiElementHandle pane, const char* text) {
    return svc_ui->pane_add_text(mod_ctx, pane, text, nullptr);
}

ModResult add_button(UiElementHandle pane, const char* label, UiPressedFn onPressed) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_BUTTON;
    desc.label = label;
    desc.on_pressed = onPressed;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}

ModResult add_toggle(UiElementHandle pane, const char* label, ConfigVarHandle var,
    const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_TOGGLE;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}

ModResult add_select(UiElementHandle pane, const char* label, ConfigVarHandle var,
    const char* const* options, const size_t optionCount,
    UiPredicateFn disabled = is_disabled_false, const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_SELECT;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CONFIG_VAR;
    desc.config_var = var;
    desc.options = options;
    desc.option_count = optionCount;
    desc.is_disabled = disabled;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}

ConfigVarHandle selected_element_x() {
    return s_elements[static_cast<size_t>(selected_element())].x;
}

ConfigVarHandle selected_element_y() {
    return s_elements[static_cast<size_t>(selected_element())].y;
}

ConfigVarHandle selected_element_scale() {
    return s_elements[static_cast<size_t>(selected_element())].scale;
}

ButtonVars& selected_button_vars() {
    return s_buttons[static_cast<size_t>(button_from_element(selected_element()))];
}

void get_selected_number(ModContext*, void* userData, UiControlValue* outValue) {
    const auto selector = reinterpret_cast<uintptr_t>(userData);
    ConfigVarHandle handle = 0;
    switch (selector) {
    case 0:
        handle = selected_element_x();
        break;
    case 1:
        handle = selected_element_y();
        break;
    case 2:
        handle = selected_element_scale();
        break;
    case 3:
        handle = selected_button_vars().itemAnchor;
        break;
    case 4:
        handle = selected_button_vars().itemScale;
        break;
    case 5:
        handle = selected_button_vars().itemOffsetX;
        break;
    case 6:
        handle = selected_button_vars().itemOffsetY;
        break;
    case 7:
        handle = selected_button_vars().ammoOffsetX;
        break;
    case 8:
        handle = selected_button_vars().ammoOffsetY;
        break;
    case 9:
        handle = selected_button_vars().ammoScale;
        break;
    case 10:
        handle = selected_button_vars().textAnchor;
        break;
    case 11:
        handle = selected_button_vars().textScale;
        break;
    case 12:
        handle = selected_button_vars().textOffsetX;
        break;
    case 13:
        handle = selected_button_vars().textOffsetY;
        break;
    default:
        break;
    }
    *outValue = UI_CONTROL_VALUE_INIT;
    outValue->int_value = get_int(handle, 0);
}

void set_selected_number(ModContext*, void* userData, const UiControlValue* value) {
    const auto selector = reinterpret_cast<uintptr_t>(userData);
    ConfigVarHandle handle = 0;
    switch (selector) {
    case 0:
        handle = selected_element_x();
        break;
    case 1:
        handle = selected_element_y();
        break;
    case 2:
        handle = selected_element_scale();
        break;
    case 3:
        handle = selected_button_vars().itemAnchor;
        break;
    case 4:
        handle = selected_button_vars().itemScale;
        break;
    case 5:
        handle = selected_button_vars().itemOffsetX;
        break;
    case 6:
        handle = selected_button_vars().itemOffsetY;
        break;
    case 7:
        handle = selected_button_vars().ammoOffsetX;
        break;
    case 8:
        handle = selected_button_vars().ammoOffsetY;
        break;
    case 9:
        handle = selected_button_vars().ammoScale;
        break;
    case 10:
        handle = selected_button_vars().textAnchor;
        break;
    case 11:
        handle = selected_button_vars().textScale;
        break;
    case 12:
        handle = selected_button_vars().textOffsetX;
        break;
    case 13:
        handle = selected_button_vars().textOffsetY;
        break;
    default:
        break;
    }
    if (handle != 0 && value != nullptr) {
        set_int(handle, static_cast<int>(value->int_value));
    }
}

ModResult add_selected_number(UiElementHandle pane, const char* label, const uintptr_t selector,
    const int min, const int max, const int step, const char* suffix,
    UiPredicateFn disabled = is_disabled_false, const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_NUMBER;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CALLBACKS;
    desc.get = get_selected_number;
    desc.set = set_selected_number;
    desc.user_data = reinterpret_cast<void*>(selector);
    desc.min = min;
    desc.max = max;
    desc.step = step;
    desc.suffix = suffix;
    desc.is_disabled = disabled;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}

ModResult add_selected_select(UiElementHandle pane, const char* label, const uintptr_t selector,
    const char* const* options, const size_t optionCount,
    UiPredicateFn disabled = is_disabled_false, const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.kind = UI_CONTROL_SELECT;
    desc.label = label;
    desc.help_rml = help;
    desc.binding = UI_BINDING_CALLBACKS;
    desc.get = get_selected_number;
    desc.set = set_selected_number;
    desc.user_data = reinterpret_cast<void*>(selector);
    desc.options = options;
    desc.option_count = optionCount;
    desc.is_disabled = disabled;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}

void reset_selected_element(ModContext*, void*) {
    reset_element(selected_element());
}

void reset_all_layout(ModContext*, void*) {
    reset_layout();
}

ModResult build_layout_tab_impl(
    ModContext*, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(left, "HUD Layout") != MOD_OK) return MOD_ERROR;
    if (add_select(left, "HUD Element", s_selectedElement, kElementLabels.data(),
            kElementLabels.size(), is_disabled_false,
            "Selects which HUD element the position, scale, item, ammo, and text controls edit.") !=
        MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(left, "HUD Backing Texture", s_background,
            "Shows or hides the decorative backing behind the A/B/X/Y HUD buttons.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_toggle(left, "Round X/Y Buttons", s_roundXYButtons,
            "Draws X and Y with the round button style exposed by the host HUD service.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_selected_number(left, "Position X", 0, -9999, 9999, 1, " px") != MOD_OK ||
        add_selected_number(left, "Position Y", 1, -9999, 9999, 1, " px") != MOD_OK ||
        add_selected_number(left, "HUD Element Scale", 2, 1, 9999, 1, "%") != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(left, "Item") != MOD_OK) return MOD_ERROR;
    if (add_selected_select(left, "HUD Item Anchor", 3, kItemAnchorOptions.data(),
            kItemAnchorOptions.size(), is_item_layout_disabled,
            "Places the item icon relative to the selected HUD button.") != MOD_OK ||
        add_selected_number(left, "HUD Item Scale", 4, 1, 9999, 1, "%", is_item_layout_disabled) !=
            MOD_OK ||
        add_selected_number(left, "HUD Item Offset X", 5, -9999, 9999, 1, " px",
            is_item_layout_disabled) != MOD_OK ||
        add_selected_number(left, "HUD Item Offset Y", 6, -9999, 9999, 1, " px",
            is_item_layout_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(left, "Ammo") != MOD_OK) return MOD_ERROR;
    if (add_selected_number(left, "Ammo Offset X", 7, -9999, 9999, 1, " px",
            is_ammo_layout_disabled) != MOD_OK ||
        add_selected_number(left, "Ammo Offset Y", 8, -9999, 9999, 1, " px",
            is_ammo_layout_disabled) != MOD_OK ||
        add_selected_number(left, "Ammo Scale", 9, 1, 9999, 1, "%", is_ammo_layout_disabled) !=
            MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(left, "Text") != MOD_OK) return MOD_ERROR;
    if (add_selected_select(left, "HUD Text Anchor", 10, kTextAnchorOptions.data(),
            kTextAnchorOptions.size(), is_text_anchor_disabled,
            "Places action text on the left or right side of the selected HUD button.") != MOD_OK ||
        add_selected_number(left, "HUD Text Scale", 11, 1, 9999, 1, "%", is_text_layout_disabled) !=
            MOD_OK ||
        add_selected_number(left, "Text Offset X", 12, -9999, 9999, 1, " px",
            is_text_anchor_disabled) != MOD_OK ||
        add_selected_number(left, "Text Offset Y", 13, -9999, 9999, 1, " px",
            is_text_anchor_disabled) != MOD_OK)
    {
        return MOD_ERROR;
    }

    if (add_section(left, "Minimap") != MOD_OK) return MOD_ERROR;
    if (add_toggle(left, "D-Pad Follows Minimap", s_dpadFollowsMinimap,
            "When enabled, the D-Pad keeps the host minimap slide relationship.") != MOD_OK ||
        add_select(left, "Minimap Slide Direction", s_minimapSlideDirection,
            kMinimapSlideOptions.data(), kMinimapSlideOptions.size(), is_minimap_disabled) !=
            MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult build_files_tab_impl(
    ModContext*, UiWindowHandle, UiElementHandle left, UiElementHandle, void*, ModError*) {
    if (add_section(left, "JSON Import/Export") != MOD_OK) return MOD_ERROR;
    if (add_text(left,
            "Import and export use hud_layout_settings.json in the active data folder. The format "
            "is compatible with Dawnlight's exported HUD layout JSON.") != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_button(left, "Import HUD Layout JSON", import_from_default_file) != MOD_OK ||
        add_button(left, "Export HUD Layout JSON", export_to_default_file) != MOD_OK)
    {
        return MOD_ERROR;
    }
    if (add_button(left, "Reset Selected HUD Element", reset_selected_element) != MOD_OK ||
        add_button(left, "Reset HUD Layout", reset_all_layout) != MOD_OK)
    {
        return MOD_ERROR;
    }
    return MOD_OK;
}

ModResult register_hud_layout_provider(ModError* error) {
    HudLayoutProviderDesc desc = HUD_LAYOUT_PROVIDER_DESC_INIT;
    desc.get_layout = get_layout;
    const ModResult result = svc_hud_layout->register_provider(mod_ctx, &desc, &s_hudLayoutProvider);
    if (result != MOD_OK) {
        return set_error(error, result, "failed to register HUD layout provider");
    }
    return MOD_OK;
}

void update_layout() {
    if (s_dirty) {
        rebuild_layout();
    }
}

}  // namespace

ModResult register_hud_layout(ModError* error) {
    if (const ModResult result = register_hud_config(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = register_hud_layout_provider(error); result != MOD_OK) {
        return result;
    }

    svc_log->info(mod_ctx, "Dawnlight HUD editing initialized");
    return MOD_OK;
}

ModResult build_hud_layout_settings_tab(
    ModContext* ctx, UiWindowHandle window, UiElementHandle left, UiElementHandle right,
    void* userData, ModError* error) {
    return build_layout_tab_impl(ctx, window, left, right, userData, error);
}

ModResult build_hud_layout_files_tab(
    ModContext* ctx, UiWindowHandle window, UiElementHandle left, UiElementHandle right,
    void* userData, ModError* error) {
    return build_files_tab_impl(ctx, window, left, right, userData, error);
}

void update_hud_layout() {
    update_layout();
}

}  // namespace dawnlight
