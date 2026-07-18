#include "hud_layout.hpp"

#include "mods/service.hpp"
#include "mods/svc/hud_layout.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

IMPORT_SERVICE(HudLayoutService, svc_hud_layout);

namespace dawnlight {
namespace {

HudLayoutProviderHandle s_provider = 0;
DuskModHudLayoutSnapshot s_layout{};
std::filesystem::path s_loadedPath;
std::filesystem::file_time_type s_loadedWriteTime{};
uint32_t s_revision = 1;
bool s_hasLayout = false;

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

void reset_layout() {
    s_layout = {};
    s_layout.struct_size = sizeof(DuskModHudLayoutSnapshot);
    s_layout.revision = s_revision++;

    for (auto& element : s_layout.elements) {
        element.scale = 1.0f;
    }
    for (auto& button : s_layout.buttons) {
        button.item_anchor = -1;
        button.text_anchor = -1;
        button.item_scale = 1.0f;
        button.ammo_scale = 1.0f;
        button.text_scale = 1.0f;
    }
}

float json_float(const nlohmann::json& object, const char* key, float fallback) {
    const auto it = object.find(key);
    return it != object.end() && it->is_number() ? it->get<float>() : fallback;
}

bool json_bool(const nlohmann::json& object, const char* key, bool fallback) {
    const auto it = object.find(key);
    return it != object.end() && it->is_boolean() ? it->get<bool>() : fallback;
}

int item_anchor(const nlohmann::json& object, const char* key, int fallback) {
    const auto it = object.find(key);
    if (it == object.end() || !it->is_string()) {
        return fallback;
    }

    const std::string value = it->get<std::string>();
    if (value == "Left") return 0;
    if (value == "Right") return 1;
    if (value == "Top") return 2;
    if (value == "Bottom") return 3;
    return fallback;
}

int text_anchor(const nlohmann::json& object, const char* key, int fallback) {
    const auto it = object.find(key);
    if (it == object.end() || !it->is_string()) {
        return fallback;
    }

    const std::string value = it->get<std::string>();
    if (value == "Left") return 0;
    if (value == "Right") return 1;
    return fallback;
}

void read_transform(const nlohmann::json& elements, const char* name, int index) {
    if (!elements.contains(name) || !elements.at(name).is_object()) {
        return;
    }

    const auto& object = elements.at(name);
    auto& transform = s_layout.elements[index];
    transform.offset_x = json_float(object, "x", transform.offset_x);
    transform.offset_y = json_float(object, "y", transform.offset_y);
    transform.scale = json_float(object, "scale", transform.scale);
}

void read_button(const nlohmann::json& elements, const char* name, int index) {
    if (!elements.contains(name) || !elements.at(name).is_object()) {
        return;
    }

    const auto& object = elements.at(name);
    auto& button = s_layout.buttons[index];
    button.item_anchor = item_anchor(object, "itemAnchor", button.item_anchor);
    button.text_anchor = text_anchor(object, "textAnchor", button.text_anchor);
    button.item_scale = json_float(object, "itemScale", button.item_scale);
    button.item_offset_x = json_float(object, "itemOffsetX", button.item_offset_x);
    button.item_offset_y = json_float(object, "itemOffsetY", button.item_offset_y);
    button.ammo_offset_x = json_float(object, "ammoOffsetX", button.ammo_offset_x);
    button.ammo_offset_y = json_float(object, "ammoOffsetY", button.ammo_offset_y);
    button.ammo_scale = json_float(object, "ammoScale", button.ammo_scale);
    button.text_scale = json_float(object, "textScale", button.text_scale);
    button.text_offset_x = json_float(object, "textOffsetX", button.text_offset_x);
    button.text_offset_y = json_float(object, "textOffsetY", button.text_offset_y);
}

bool load_layout(const std::filesystem::path& path) {
    reset_layout();

    std::ifstream stream(path);
    if (!stream.is_open()) {
        return false;
    }

    const auto json = nlohmann::json::parse(stream, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        return false;
    }

    if (!json_bool(json, "background", true)) {
        s_layout.elements[12].flags |= DUSK_MOD_HUD_ELEMENT_HIDDEN;
    }
    if (json_bool(json, "roundXYButtons", false)) {
        s_layout.buttons[2].style_flags |= DUSK_MOD_HUD_BUTTON_STYLE_ROUND;
        s_layout.buttons[3].style_flags |= DUSK_MOD_HUD_BUTTON_STYLE_ROUND;
    }

    const auto elementsIt = json.find("elements");
    if (elementsIt != json.end() && elementsIt->is_object()) {
        const auto& elements = *elementsIt;
        static constexpr const char* kButtonNames[] = {"A", "B", "X", "Y", "Z"};
        for (int i = 0; i < 5; i++) {
            read_transform(elements, kButtonNames[i], i);
            read_button(elements, kButtonNames[i], i);
        }

        read_transform(elements, "Hearts", 5);
        read_transform(elements, "Rupees", 6);
        read_transform(elements, "Keys", 7);
        read_transform(elements, "Oil", 8);
        read_transform(elements, "Oxygen", 9);
        read_transform(elements, "D-Pad", 10);
        read_transform(elements, "Minimap", 11);
        read_transform(elements, "Button Backing", 12);
        read_transform(elements, "Midna", 13);

        if (elements.contains("Minimap") && elements.at("Minimap").is_object()) {
            const auto& minimap = elements.at("Minimap");
            if (!json_bool(minimap, "dpadFollowsMinimap", true)) {
                s_layout.elements[10].parent_mode = DUSK_MOD_HUD_PARENT_INDEPENDENT;
            }
            const auto slideIt = minimap.find("slideDirection");
            if (slideIt != minimap.end() && slideIt->is_string() &&
                slideIt->get<std::string>() == "Right -> Left")
            {
                s_layout.elements[11].slide_direction = DUSK_MOD_HUD_SLIDE_RIGHT_TO_LEFT;
            }
        }
    }

    return true;
}

const DuskModHudLayoutSnapshot* get_layout(ModContext*, const char* data_path, void*) {
    const std::filesystem::path base = path_from_utf8(data_path);
    if (base.empty()) {
        return nullptr;
    }

    const auto path = base / "hud_layout_settings.json";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        s_hasLayout = false;
        return nullptr;
    }

    const auto writeTime = std::filesystem::last_write_time(path, ec);
    if (!s_hasLayout || path != s_loadedPath || writeTime != s_loadedWriteTime) {
        s_loadedPath = path;
        s_loadedWriteTime = writeTime;
        s_hasLayout = load_layout(path);
    }

    return s_hasLayout ? &s_layout : nullptr;
}

}  // namespace

ModResult register_hud_layout_provider(ModError* error) {
    HudLayoutProviderDesc desc = HUD_LAYOUT_PROVIDER_DESC_INIT;
    desc.get_layout = get_layout;

    const ModResult result = svc_hud_layout->register_provider(mod_ctx, &desc, &s_provider);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight HUD-layout provider");
    }
    return MOD_OK;
}

void update_hud_layout() {}

}  // namespace dawnlight
