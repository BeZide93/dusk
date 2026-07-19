#include "item_assignment.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/item_assignment.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::item_assignment");

struct Provider {
    uint64_t handle = 0;
    ItemAssignmentProviderDesc desc = ITEM_ASSIGNMENT_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const ItemAssignmentProviderDesc* desc,
    ItemAssignmentProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(ItemAssignmentProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, ItemAssignmentProviderHandle handle) {
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
            if (provider.desc.select_item_slot_count != nullptr) {
                owner = &mod;
                selection = &provider;
            }
        }
    }
    return selection;
}

int call_select_item_slot_count() {
    LoadedMod* owner = nullptr;
    const auto* provider = latest_provider(owner);
    if (owner == nullptr || provider == nullptr) {
        return 2;
    }

    try {
        const int slotCount =
            provider->desc.select_item_slot_count(owner->context.get(), provider->desc.user_data);
        if (slotCount < 2) {
            return 2;
        }
        if (slotCount > 4) {
            return 4;
        }
        return slotCount;
    } catch (const std::exception& e) {
        fail_mod(*owner, MOD_ERROR,
            std::string{"Exception in item-assignment provider: "} + e.what());
    } catch (...) {
        fail_mod(*owner, MOD_ERROR, "Unknown exception in item-assignment provider");
    }
    return 2;
}

constexpr ItemAssignmentService s_itemAssignmentService{
    .header = SERVICE_HEADER(
        ItemAssignmentService, ITEM_ASSIGNMENT_SERVICE_MAJOR, ITEM_ASSIGNMENT_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace item_assignment {

int select_item_slot_count() {
    return call_select_item_slot_count();
}

bool select_item_slot_enabled(int slot) {
    return slot >= 0 && slot < select_item_slot_count();
}

}  // namespace item_assignment

constinit const ServiceModule g_itemAssignmentModule{
    .id = ITEM_ASSIGNMENT_SERVICE_ID,
    .majorVersion = ITEM_ASSIGNMENT_SERVICE_MAJOR,
    .minorVersion = ITEM_ASSIGNMENT_SERVICE_MINOR,
    .service = &s_itemAssignmentService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
