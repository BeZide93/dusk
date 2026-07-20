#include "touch_controls.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/touch_controls.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::touch_controls");

struct Provider {
    uint64_t handle = 0;
    TouchControlsProviderDesc desc = TOUCH_CONTROLS_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;
ui::TouchLayoutControlInfo s_controlScratch;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(ModContext* context, const TouchControlsProviderDesc* desc,
    TouchControlsProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(TouchControlsProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, TouchControlsProviderHandle handle) {
    auto* mod = mod_from_context(context);
    if (mod == nullptr || handle == 0) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto it = s_providers.find(mod);
    if (it == s_providers.end()) {
        return MOD_INVALID_ARGUMENT;
    }

    if (std::erase_if(it->second, [&](const auto& provider) {
            return provider.handle == handle;
        }) == 0)
    {
        Log.error("[{}] unregister_provider failed: unknown handle {}", mod->metadata.id, handle);
        return MOD_INVALID_ARGUMENT;
    }

    if (it->second.empty()) {
        s_providers.erase(it);
    }
    return MOD_OK;
}

template <class Fn>
struct ProviderSelection {
    LoadedMod* mod = nullptr;
    const Provider* provider = nullptr;
    Fn callback = nullptr;
};

template <class Fn>
ProviderSelection<Fn> latest_provider(Fn TouchControlsProviderDesc::*member) {
    ProviderSelection<Fn> selection;
    for (auto& mod : ModLoader::instance().active_mods()) {
        const auto it = s_providers.find(&mod);
        if (it == s_providers.end()) {
            continue;
        }
        for (const auto& provider : it->second) {
            if (auto callback = provider.desc.*member) {
                selection = {&mod, &provider, callback};
            }
        }
    }
    return selection;
}

template <class Return, class Fn, class... Args>
Return call(Fn TouchControlsProviderDesc::*member, Return fallback, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return fallback;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in touch-controls provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in touch-controls provider");
    }
    return fallback;
}

bool valid_control(int32_t control) {
    return control >= 0 && control < static_cast<int32_t>(ui::Control::COUNT);
}

bool valid_anchor(int32_t anchor) {
    return anchor >= static_cast<int32_t>(ui::ControlAnchor::None) &&
           anchor <= static_cast<int32_t>(ui::ControlAnchor::BottomRight);
}

bool convert_control(const TouchControlsControlDesc& in, ui::TouchLayoutControlInfo& out) {
    if (in.struct_size < sizeof(TouchControlsControlDesc) || in.layout_id == nullptr ||
        in.element_id == nullptr || !valid_anchor(in.anchor) ||
        (in.has_control != 0 && !valid_control(in.control)))
    {
        return false;
    }

    out = {
        .layoutId = in.layout_id,
        .elementId = in.element_id,
        .props =
            {
                .x = in.x,
                .y = in.y,
                .w = in.w,
                .h = in.h,
                .scale = in.scale,
                .anchor = static_cast<ui::ControlAnchor>(in.anchor),
            },
        .control = in.has_control != 0 ? static_cast<ui::Control>(in.control) : ui::Control::COUNT,
        .hasControl = in.has_control != 0,
    };
    return true;
}

constexpr TouchControlsService s_touchControlsService{
    .header =
        SERVICE_HEADER(TouchControlsService, TOUCH_CONTROLS_SERVICE_MAJOR, TOUCH_CONTROLS_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace touch_controls {

std::string_view extra_rml_fragment() noexcept {
    const char* fragment = call(&TouchControlsProviderDesc::extra_rml, static_cast<const char*>(nullptr));
    return fragment != nullptr ? std::string_view{fragment} : std::string_view{};
}

std::size_t extra_control_count() noexcept {
    return call(&TouchControlsProviderDesc::extra_control_count, static_cast<std::size_t>(0));
}

const ui::TouchLayoutControlInfo* extra_control_at(std::size_t index) noexcept {
    auto selection = latest_provider(&TouchControlsProviderDesc::extra_control_at);
    if (selection.mod == nullptr) {
        return nullptr;
    }

    TouchControlsControlDesc desc = TOUCH_CONTROLS_CONTROL_DESC_INIT;
    int handled = 0;
    try {
        handled = selection.callback(selection.mod->context.get(), index, &desc,
            selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in touch-controls provider: "} + e.what());
        return nullptr;
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in touch-controls provider");
        return nullptr;
    }

    if (handled == 0 || !convert_control(desc, s_controlScratch)) {
        return nullptr;
    }
    return &s_controlScratch;
}

bool display_override(ui::Control control, ui::TouchControlDisplayOverride* out) noexcept {
    if (out == nullptr) {
        return false;
    }

    TouchControlsDisplayOverride override = TOUCH_CONTROLS_DISPLAY_OVERRIDE_INIT;
    const int handled = call(&TouchControlsProviderDesc::display_override, 0,
        static_cast<int32_t>(control), &override);
    if (handled == 0 || override.struct_size < sizeof(TouchControlsDisplayOverride)) {
        return false;
    }

    *out = {
        .iconSource = override.icon_source,
        .iconRevision = override.icon_revision,
        .visible = override.visible != 0,
        .showIcon = override.show_icon != 0,
    };
    return true;
}

std::uint16_t pad_button(ui::Control control, std::uint16_t fallback) noexcept {
    return call(&TouchControlsProviderDesc::pad_button, fallback,
        static_cast<int32_t>(control), fallback);
}

void control_event(ui::Control control, bool pressed) noexcept {
    auto selection = latest_provider(&TouchControlsProviderDesc::control_event);
    if (selection.mod == nullptr) {
        return;
    }

    try {
        selection.callback(selection.mod->context.get(), static_cast<int32_t>(control),
            pressed ? 1 : 0, selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in touch-controls provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in touch-controls provider");
    }
}

}  // namespace touch_controls

constinit const ServiceModule g_touchControlsModule{
    .id = TOUCH_CONTROLS_SERVICE_ID,
    .majorVersion = TOUCH_CONTROLS_SERVICE_MAJOR,
    .minorVersion = TOUCH_CONTROLS_SERVICE_MINOR,
    .service = &s_touchControlsService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
