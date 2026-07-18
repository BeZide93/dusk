#include "hud_layout.hpp"

#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/hud_layout.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::hud_layout");

struct HudLayoutProvider {
    uint64_t handle = 0;
    HudLayoutProviderFn getLayout = nullptr;
    void* userData = nullptr;
};

std::unordered_map<const LoadedMod*, std::vector<HudLayoutProvider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(ModContext* context, const HudLayoutProviderDesc* desc,
    HudLayoutProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(HudLayoutProviderDesc) ||
        desc->get_layout == nullptr)
    {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({
        .handle = handle,
        .getLayout = desc->get_layout,
        .userData = desc->user_data,
    });
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, HudLayoutProviderHandle handle) {
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

const DuskModHudLayoutSnapshot* call_provider(
    LoadedMod& mod, const HudLayoutProvider& provider, const char* dataPath) {
    if (!mod.active || provider.getLayout == nullptr) {
        return nullptr;
    }

    try {
        const auto* layout = provider.getLayout(mod.context.get(), dataPath, provider.userData);
        if (layout != nullptr && layout->struct_size < sizeof(DuskModHudLayoutSnapshot)) {
            Log.error("[{}] HUD layout provider returned an undersized snapshot", mod.metadata.id);
            return nullptr;
        }
        return layout;
    } catch (const std::exception& e) {
        fail_mod(mod, MOD_ERROR, std::string{"Exception in HUD layout provider: "} + e.what());
    } catch (...) {
        fail_mod(mod, MOD_ERROR, "Unknown exception in HUD layout provider");
    }
    return nullptr;
}

constexpr HudLayoutService s_hudLayoutService{
    .header = SERVICE_HEADER(HudLayoutService, HUD_LAYOUT_SERVICE_MAJOR, HUD_LAYOUT_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

const DuskModHudLayoutSnapshot* hud_layout_snapshot(const char* dataPath) {
    LoadedMod* owner = nullptr;
    const HudLayoutProvider* provider = nullptr;

    for (auto& mod : ModLoader::instance().active_mods()) {
        const auto it = s_providers.find(&mod);
        if (it == s_providers.end() || it->second.empty()) {
            continue;
        }
        owner = &mod;
        provider = &it->second.back();
    }

    return owner != nullptr && provider != nullptr ? call_provider(*owner, *provider, dataPath) :
                                                     nullptr;
}

constinit const ServiceModule g_hudLayoutModule{
    .id = HUD_LAYOUT_SERVICE_ID,
    .majorVersion = HUD_LAYOUT_SERVICE_MAJOR,
    .minorVersion = HUD_LAYOUT_SERVICE_MINOR,
    .service = &s_hudLayoutService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
