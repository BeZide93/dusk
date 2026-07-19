#include "midna_dialog.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/midna_dialog.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::midna_dialog");

struct Provider {
    uint64_t handle = 0;
    MidnaDialogProviderDesc desc = MIDNA_DIALOG_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(ModContext* context, const MidnaDialogProviderDesc* desc,
    MidnaDialogProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(MidnaDialogProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, MidnaDialogProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn MidnaDialogProviderDesc::*member) {
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
Return call(Fn MidnaDialogProviderDesc::*member, Return fallback, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return fallback;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in Midna-dialog provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in Midna-dialog provider");
    }
    return fallback;
}

constexpr MidnaDialogService s_midnaDialogService{
    .header =
        SERVICE_HEADER(MidnaDialogService, MIDNA_DIALOG_SERVICE_MAJOR, MIDNA_DIALOG_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace midna_dialog {

const char* prompt_text() {
    return call(&MidnaDialogProviderDesc::prompt_text, static_cast<const char*>(nullptr));
}

bool prompt_begin() {
    return call(&MidnaDialogProviderDesc::prompt_begin, 0) != 0;
}

bool prompt_resolve(int choice) {
    return call(&MidnaDialogProviderDesc::prompt_resolve, 0, choice) != 0;
}

bool prompt_consume_resolution() {
    return call(&MidnaDialogProviderDesc::prompt_consume_resolution, 0) != 0;
}

const char* menu_option() {
    return call(&MidnaDialogProviderDesc::menu_option, static_cast<const char*>(nullptr));
}

bool menu_begin() {
    return call(&MidnaDialogProviderDesc::menu_begin, 0) != 0;
}

bool menu_resolve(int choice) {
    return call(&MidnaDialogProviderDesc::menu_resolve, 0, choice) != 0;
}

bool menu_cancel() {
    return call(&MidnaDialogProviderDesc::menu_cancel, 0) != 0;
}

bool menu_execute_action(void* player) {
    return call(&MidnaDialogProviderDesc::menu_execute_action, 0, player) != 0;
}

}  // namespace midna_dialog

constinit const ServiceModule g_midnaDialogModule{
    .id = MIDNA_DIALOG_SERVICE_ID,
    .majorVersion = MIDNA_DIALOG_SERVICE_MAJOR,
    .minorVersion = MIDNA_DIALOG_SERVICE_MINOR,
    .service = &s_midnaDialogService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
