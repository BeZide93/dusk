#include "action_input.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/action_input.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::action_input");

struct Provider {
    uint64_t handle = 0;
    ActionInputProviderDesc desc = ACTION_INPUT_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(ModContext* context, const ActionInputProviderDesc* desc,
    ActionInputProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(ActionInputProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, ActionInputProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn ActionInputProviderDesc::*member) {
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
int call_int(Fn ActionInputProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return DUSK_MOD_ACTION_INPUT_DEFAULT;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in action-input provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in action-input provider");
    }
    return 0;
}

constexpr ActionInputService s_actionInputService{
    .header =
        SERVICE_HEADER(ActionInputService, ACTION_INPUT_SERVICE_MAJOR, ACTION_INPUT_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace action_input {

int player_action_state(void* player, int action, bool defaultPressed) {
    const int defaultState =
        defaultPressed ? DUSK_MOD_ACTION_INPUT_PRESSED : DUSK_MOD_ACTION_INPUT_RELEASED;
    const int state =
        call_int(&ActionInputProviderDesc::player_action, player, action, defaultState);
    switch (state) {
    case DUSK_MOD_ACTION_INPUT_RELEASED:
    case DUSK_MOD_ACTION_INPUT_PRESSED:
        return state;
    default:
        return DUSK_MOD_ACTION_INPUT_DEFAULT;
    }
}

bool player_action_pressed(void* player, int action, bool defaultPressed) {
    const int state = player_action_state(player, action, defaultPressed);
    return state == DUSK_MOD_ACTION_INPUT_DEFAULT ? defaultPressed :
                                                    state == DUSK_MOD_ACTION_INPUT_PRESSED;
}

}  // namespace action_input

constinit const ServiceModule g_actionInputModule{
    .id = ACTION_INPUT_SERVICE_ID,
    .majorVersion = ACTION_INPUT_SERVICE_MAJOR,
    .minorVersion = ACTION_INPUT_SERVICE_MINOR,
    .service = &s_actionInputService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
