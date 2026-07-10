#include "boss_flow.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/boss_flow.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::boss_flow");

struct Provider {
    uint64_t handle = 0;
    BossFlowProviderDesc desc = BOSS_FLOW_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const BossFlowProviderDesc* desc, BossFlowProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(BossFlowProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, BossFlowProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn BossFlowProviderDesc::*member) {
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

template <class Fn, class... Args>
int call_int(Fn BossFlowProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return 0;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in boss-flow provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in boss-flow provider");
    }
    return 0;
}

constexpr BossFlowService s_bossFlowService{
    .header = SERVICE_HEADER(BossFlowService, BOSS_FLOW_SERVICE_MAJOR, BOSS_FLOW_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace boss_flow {

int boss_warp_execute(void* bossWarp) {
    return call_int(&BossFlowProviderDesc::boss_warp_execute, bossWarp);
}

bool final_battle_complete() {
    return call_int(&BossFlowProviderDesc::final_battle_complete) != 0;
}

}  // namespace boss_flow

constinit const ServiceModule g_bossFlowModule{
    .id = BOSS_FLOW_SERVICE_ID,
    .majorVersion = BOSS_FLOW_SERVICE_MAJOR,
    .minorVersion = BOSS_FLOW_SERVICE_MINOR,
    .service = &s_bossFlowService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
