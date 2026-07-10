#include "item_slots.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/item_slots.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::item_slots");

struct Provider {
    uint64_t handle = 0;
    ItemSlotsProviderDesc desc = ITEM_SLOTS_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const ItemSlotsProviderDesc* desc, ItemSlotsProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(ItemSlotsProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, ItemSlotsProviderHandle handle) {
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

const Provider* latest_provider(LoadedMod*& owner) {
    owner = nullptr;
    const Provider* selection = nullptr;
    for (auto& mod : ModLoader::instance().active_mods()) {
        const auto it = s_providers.find(&mod);
        if (it == s_providers.end()) {
            continue;
        }
        for (const auto& provider : it->second) {
            if (provider.desc.enable_z_slot != nullptr) {
                owner = &mod;
                selection = &provider;
            }
        }
    }
    return selection;
}

bool call_enable_z_slot() {
    LoadedMod* owner = nullptr;
    const auto* provider = latest_provider(owner);
    if (owner == nullptr || provider == nullptr) {
        return false;
    }

    try {
        return provider->desc.enable_z_slot(owner->context.get(), provider->desc.user_data) != 0;
    } catch (const std::exception& e) {
        fail_mod(*owner, MOD_ERROR, std::string{"Exception in item-slots provider: "} + e.what());
    } catch (...) {
        fail_mod(*owner, MOD_ERROR, "Unknown exception in item-slots provider");
    }
    return false;
}

constexpr ItemSlotsService s_itemSlotsService{
    .header = SERVICE_HEADER(ItemSlotsService, ITEM_SLOTS_SERVICE_MAJOR, ITEM_SLOTS_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace item_slots {

bool z_slot_enabled() {
    return call_enable_z_slot();
}

}  // namespace item_slots

constinit const ServiceModule g_itemSlotsModule{
    .id = ITEM_SLOTS_SERVICE_ID,
    .majorVersion = ITEM_SLOTS_SERVICE_MAJOR,
    .minorVersion = ITEM_SLOTS_SERVICE_MINOR,
    .service = &s_itemSlotsService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
