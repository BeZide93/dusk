#include "stage_flow.hpp"
#include "registry.hpp"

#include "aurora/lib/logging.hpp"
#include "SSystem/SComponent/c_lib.h"
#include "JSystem/JGeometry.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "d/actor/d_a_b_gnd.h"
#include "d/actor/d_a_obj_bosswarp.h"
#include "d/d_com_inf_game.h"
#include "dusk/mods/loader/loader.hpp"
#include "f_op/f_op_camera_mng.h"
#include "m_Do/m_Do_audio.h"
#include "mods/svc/stage_flow.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace dusk::mods::svc {
namespace {

aurora::Module Log("dusk::mods::stage_flow");

struct Provider {
    uint64_t handle = 0;
    StageFlowProviderDesc desc = STAGE_FLOW_PROVIDER_DESC_INIT;
};

std::unordered_map<const LoadedMod*, std::vector<Provider>> s_providers;
uint64_t s_nextHandle = 1;

void remove_mod(LoadedMod& mod) {
    s_providers.erase(&mod);
}

ModResult register_provider(
    ModContext* context, const StageFlowProviderDesc* desc, StageFlowProviderHandle* outHandle) {
    if (outHandle != nullptr) {
        *outHandle = 0;
    }

    auto* mod = mod_from_context(context);
    if (mod == nullptr || desc == nullptr || desc->struct_size < sizeof(StageFlowProviderDesc)) {
        return MOD_INVALID_ARGUMENT;
    }

    const auto handle = s_nextHandle++;
    s_providers[mod].push_back({.handle = handle, .desc = *desc});
    if (outHandle != nullptr) {
        *outHandle = handle;
    }
    return MOD_OK;
}

ModResult unregister_provider(ModContext* context, StageFlowProviderHandle handle) {
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
ProviderSelection<Fn> latest_provider(Fn StageFlowProviderDesc::*member) {
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
int call_int(Fn StageFlowProviderDesc::*member, Args... args) {
    auto selection = latest_provider(member);
    if (selection.mod == nullptr) {
        return 0;
    }

    try {
        return selection.callback(
            selection.mod->context.get(), args..., selection.provider->desc.user_data);
    } catch (const std::exception& e) {
        fail_mod(*selection.mod, MOD_ERROR,
            std::string{"Exception in stage-flow provider: "} + e.what());
    } catch (...) {
        fail_mod(*selection.mod, MOD_ERROR, "Unknown exception in stage-flow provider");
    }
    return 0;
}

constexpr StageFlowService s_stageFlowService{
    .header = SERVICE_HEADER(StageFlowService, STAGE_FLOW_SERVICE_MAJOR, STAGE_FLOW_SERVICE_MINOR),
    .register_provider = register_provider,
    .unregister_provider = unregister_provider,
};

}  // namespace

namespace stage_flow {

void run_boss_warp_transition(void* actor, int mode) {
    auto& warp = *static_cast<daObjBossWarp_c*>(actor);
    warp.appear(0);
    if (mode == DUSK_MOD_STAGE_FLOW_TRANSITION_ACTIVE) {
        warp.mpBrkAnm->play();
        warp.mpBtkAnm[0]->play();
        warp.mpBtkAnm[1]->play();
    } else {
        warp.mpBrkAnm->setPlaySpeed(0.0f);
        warp.mpBtkAnm[0]->setPlaySpeed(0.0f);
        warp.mpBtkAnm[1]->setPlaySpeed(0.0f);
    }

    if (warp.mScalingUp) {
        cLib_chaseF(&warp.scale.y, 1.0f, 0.016f);
    }

    if (warp.mpParticle[3] != nullptr) {
        JGeometry::TVec3<f32> particleScale;
        JGeometry::setTVec3f(&warp.scale.x, &particleScale.x);
        warp.mpParticle[3]->setGlobalScale(particleScale);
    }

    if (mode == DUSK_MOD_STAGE_FLOW_TRANSITION_ACTIVE && warp.mpBrkAnm != nullptr &&
        warp.mpBrkAnm->getFrame() != 0.0f)
    {
        mDoAud_seStartLevel(Z2SE_OBJ_MDN_ESCAPE_HOLE, &warp.current.pos, 0, 0);
    }

    warp.setBaseMtx();
}

bool transition_actor_update(void* actor, int actorKind) {
    const int mode = call_int(&StageFlowProviderDesc::transition_actor_update, actor, actorKind);
    if (mode == DUSK_MOD_STAGE_FLOW_TRANSITION_DEFAULT) {
        return false;
    }

    if (actorKind == DUSK_MOD_STAGE_FLOW_TRANSITION_ACTOR_BOSS_WARP && actor != nullptr) {
        run_boss_warp_transition(actor, mode);
        return true;
    }

    return false;
}

bool sequence_complete(int sequenceKind) {
    return call_int(&StageFlowProviderDesc::sequence_complete, sequenceKind) != 0;
}

bool final_battle_sequence_complete(void* camera, void* boss) {
    if (!sequence_complete(DUSK_MOD_STAGE_FLOW_SEQUENCE_FINAL_BATTLE)) {
        return false;
    }

    auto* cameraProcess = static_cast<camera_process_class*>(camera);
    if (cameraProcess != nullptr) {
        cameraProcess->mCamera.Start();
        cameraProcess->mCamera.SetTrimSize(0);
    }

    dComIfGp_event_reset();

    auto* ganondorf = static_cast<b_gnd_class*>(boss);
    if (ganondorf != nullptr) {
        ganondorf->mDemoCamMode = 0;
        ganondorf->mDemoCamTimer = 0;
    }
    return true;
}

}  // namespace stage_flow

constinit const ServiceModule g_stageFlowModule{
    .id = STAGE_FLOW_SERVICE_ID,
    .majorVersion = STAGE_FLOW_SERVICE_MAJOR,
    .minorVersion = STAGE_FLOW_SERVICE_MINOR,
    .service = &s_stageFlowService,
    .modDetached = remove_mod,
};

}  // namespace dusk::mods::svc
