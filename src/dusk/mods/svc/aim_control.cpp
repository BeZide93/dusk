#include "aim_control.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/aim_control.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::aim_control");

struct Provider {
    uint64_t handle = 0;
    AimControlProviderDesc desc = AIM_CONTROL_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const AimControlProviderDesc* desc, AimControlProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(AimControlProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, AimControlProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn AimControlProviderDesc::*member) {
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
int call_int(Fn AimControlProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return 0;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in aim-control provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in aim-control provider");
    }
    return 0;
}

DuskModAimRequest make_request(
    uint32_t pad, int itemKind, bool scopedAim, bool supportedItemAim) {
    return {
        .struct_size = sizeof(DuskModAimRequest),
        .pad = pad,
        .item_kind = itemKind,
        .scoped = static_cast<int32_t>(scopedAim),
        .supported = static_cast<int32_t>(supportedItemAim),
    };
}

constexpr AimControlService s_aimControlService{
    .header = SERVICE_HEADER(AimControlService, AIM_CONTROL_SERVICE_MAJOR, AIM_CONTROL_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace aim_control {

int camera_mode(uint32_t pad, int itemKind, bool scopedAim, bool supportedItemAim) {
    const auto request = make_request(pad, itemKind, scopedAim, supportedItemAim);
    const int mode = call_int(&AimControlProviderDesc::camera_mode, &request);
    switch (mode) {
    case DUSK_MOD_AIM_CAMERA_FIRST_PERSON:
    case DUSK_MOD_AIM_CAMERA_THIRD_PERSON:
    case DUSK_MOD_AIM_CAMERA_OVER_SHOULDER:
        return mode;
    default:
        return DUSK_MOD_AIM_CAMERA_DEFAULT;
    }
}

int input_routing(int itemKind) {
    const auto request = make_request(0, itemKind, false, true);
    const int routing = call_int(&AimControlProviderDesc::input_routing, &request);
    return routing == DUSK_MOD_AIM_INPUT_LEFT_MOVE_RIGHT_AIM ?
        DUSK_MOD_AIM_INPUT_LEFT_MOVE_RIGHT_AIM :
        DUSK_MOD_AIM_INPUT_DEFAULT;
}

bool subject_update(void* player, int itemKind) {
    return call_int(&AimControlProviderDesc::subject_update, player, itemKind) != 0;
}

}  // namespace aim_control

constinit const ServiceModule g_aimControlModule{
    .id = AIM_CONTROL_SERVICE_ID,
    .majorVersion = AIM_CONTROL_SERVICE_MAJOR,
    .minorVersion = AIM_CONTROL_SERVICE_MINOR,
    .service = &s_aimControlService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
