#include "midna_dialog.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_midna.h"
#include "d/d_com_inf_game.h"
#include "d/d_msg_class.h"
#include "d/d_msg_object.h"
#include "d/d_msg_scrn_base.h"
#include "dusk/mods/loader/loader.hpp"
#include "mods/svc/midna_dialog.h"

#include <algorithm>
#include <cstring>
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

bool menu_execute_warp(void* player) {
    return call(&MidnaDialogProviderDesc::menu_execute_warp, 0, player) != 0;
}

bool has_custom_flow() {
    return prompt_text() != nullptr || menu_option() != nullptr;
}

bool custom_prompt_available() {
    return prompt_text() != nullptr;
}

bool custom_menu_option_available() {
    return menu_option() != nullptr;
}

void reset_midna_talk(daMidna_c* midna) {
    dComIfGp_getEvent()->reset(midna);
    midna->offStateFlg0(daMidna_c::FLG0_UNK_8000);
}

bool consume_pending_flow(daMidna_c* midna, daAlink_c* player) {
    if (midna == nullptr) {
        return false;
    }

    const bool promptResolved = prompt_consume_resolution();
    const bool warpRequested = menu_execute_warp(player);
    if (!promptResolved && !warpRequested) {
        return false;
    }

    reset_midna_talk(midna);
    return true;
}

bool begin_custom_flow() {
    if (prompt_begin()) {
        dMsgObject_setWord(prompt_text());
        dMsgObject_setSelectWordFlag(2);
        dMsgObject_setSelectWord(0, "Yes");
        dMsgObject_setSelectWord(1, "No");
        dMsgObject_setSelectWord(2, "");
        return true;
    }

    if (menu_begin()) {
        dMsgObject_setSelectWordFlag(3);
        dMsgObject_setSelectWord(0, "");
        dMsgObject_setSelectWord(1, "");
        dMsgObject_setSelectWord(2, "");
        return true;
    }

    return false;
}

bool finish_custom_flow(daMidna_c* midna, bool flowDone, int choice) {
    if (midna == nullptr) {
        return false;
    }

    if (flowDone && choice < 0 && has_custom_flow()) {
        choice = dMsgObject_getSelectCursorPos();
    }

    if (choice < 0 || (!menu_resolve(choice) && !prompt_resolve(choice))) {
        return false;
    }

    reset_midna_talk(midna);
    return true;
}

bool draw_custom_prompt(dMsgScrnBase_c* screen, jmessage_tReference*) {
    const char* customPrompt = prompt_text();
    if (screen == nullptr || customPrompt == nullptr) {
        return false;
    }

    char yes[] = "Yes";
    char no[] = "No";
    char empty[] = "";
    screen->setString(customPrompt, customPrompt);
    screen->setRubyString(empty);
    screen->setSelectString(empty, yes, no);
    return true;
}

bool apply_custom_menu_option(jmessage_tReference* reference) {
    const char* customOption = menu_option();
    if (reference == nullptr || customOption == nullptr) {
        return false;
    }

    char option0[200];
    char option1[200];
    std::strcpy(option0, reference->getSelTextPtr(1));
    std::strcpy(option1, reference->getSelTextPtr(2));
    std::strcpy(reference->getSelTextPtr(0), option0);
    std::strcpy(reference->getSelTextPtr(1), option1);
    SAFE_STRCPY(reference->getSelTextPtr(2), customOption);
    return true;
}

bool resolve_cancelled_selection() {
    return menu_cancel() || prompt_resolve(1);
}

bool resolve_cursor_selection(int choice) {
    return menu_resolve(choice) || prompt_resolve(choice);
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
