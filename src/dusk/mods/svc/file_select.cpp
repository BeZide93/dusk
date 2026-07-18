#include "file_select.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/file_select.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::file_select");

struct Provider {
    uint64_t handle = 0;
    FileSelectProviderDesc desc = FILE_SELECT_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const FileSelectProviderDesc* desc, FileSelectProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(FileSelectProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, FileSelectProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn FileSelectProviderDesc::*member) {
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
int call_int(Fn FileSelectProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return 0;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR, std::string{"Exception in file-select provider: "} +
                                                e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in file-select provider");
    }
    return 0;
}

template <class Fn, class... Args>
void call_void(Fn FileSelectProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return;
    }

    try {
        selection.callback(selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR, std::string{"Exception in file-select provider: "} +
                                                e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in file-select provider");
    }
}

constexpr FileSelectService s_fileSelectService{
    .header = SERVICE_HEADER(FileSelectService, FILE_SELECT_SERVICE_MAJOR, FILE_SELECT_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace file_select {

bool update(void* fileSelect) {
    return call_int(&FileSelectProviderDesc::update, fileSelect) != 0;
}

bool open_new_slot(void* fileSelect) {
    return call_int(&FileSelectProviderDesc::open_new_slot, fileSelect) != 0;
}

bool start_existing_slot(void* fileSelect) {
    return call_int(&FileSelectProviderDesc::start_existing_slot, fileSelect) != 0;
}

void names_confirmed(void* fileSelect) {
    call_void(&FileSelectProviderDesc::names_confirmed, fileSelect);
}

void destroyed(void* fileSelect) {
    call_void(&FileSelectProviderDesc::destroyed, fileSelect);
}

bool start_stage(void* nameScene) {
    return call_int(&FileSelectProviderDesc::start_stage, nameScene) != 0;
}

}  // namespace file_select

constinit const ServiceModule g_fileSelectModule{
    .id = FILE_SELECT_SERVICE_ID,
    .majorVersion = FILE_SELECT_SERVICE_MAJOR,
    .minorVersion = FILE_SELECT_SERVICE_MINOR,
    .service = &s_fileSelectService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
