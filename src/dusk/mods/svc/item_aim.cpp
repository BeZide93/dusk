#include "item_aim.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/item_aim.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::item_aim");

struct Provider {
    uint64_t handle = 0;
    ItemAimProviderDesc desc = ITEM_AIM_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const ItemAimProviderDesc* desc, ItemAimProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(ItemAimProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, ItemAimProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn ItemAimProviderDesc::*member) {
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
int call_int(Fn ItemAimProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return 0;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in item-aim provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in item-aim provider");
    }
    return 0;
}

constexpr ItemAimService s_itemAimService{
    .header = SERVICE_HEADER(ItemAimService, ITEM_AIM_SERVICE_MAJOR, ITEM_AIM_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace item_aim {

bool use_cinema_camera(uint32_t pad, bool scopedAim, bool supportedItemAim) {
    return call_int(&ItemAimProviderDesc::use_cinema_camera, pad, static_cast<int>(scopedAim),
               static_cast<int>(supportedItemAim)) != 0;
}

bool use_third_person(uint32_t pad, bool supportedItemAim) {
    return call_int(
               &ItemAimProviderDesc::use_third_person, pad, static_cast<int>(supportedItemAim)) !=
           0;
}

bool movement_enabled() {
    return call_int(&ItemAimProviderDesc::movement_enabled) != 0;
}

bool subject_update(void* player, int itemKind) {
    return call_int(&ItemAimProviderDesc::subject_update, player, itemKind) != 0;
}

}  // namespace item_aim

constinit const ServiceModule g_itemAimModule{
    .id = ITEM_AIM_SERVICE_ID,
    .majorVersion = ITEM_AIM_SERVICE_MAJOR,
    .minorVersion = ITEM_AIM_SERVICE_MINOR,
    .service = &s_itemAimService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
