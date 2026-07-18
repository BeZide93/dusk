#include "config.hpp"

#include "mods/service.hpp"
#include "mods/svc/item_assignment.h"

IMPORT_SERVICE(ItemAssignmentService, svc_item_assignment);

namespace dawnlight {
namespace {

ItemAssignmentProviderHandle s_provider = 0;

int select_item_slot_count(ModContext*, void*) {
    return z_item_slot_enabled() ? 3 : 2;
}

}  // namespace

ModResult install_item_slot_hooks(ModError* error) {
    ItemAssignmentProviderDesc desc = ITEM_ASSIGNMENT_PROVIDER_DESC_INIT;
    desc.select_item_slot_count = select_item_slot_count;

    const ModResult result = svc_item_assignment->register_provider(mod_ctx, &desc, &s_provider);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight item-slot provider");
    }
    return MOD_OK;
}

}  // namespace dawnlight
