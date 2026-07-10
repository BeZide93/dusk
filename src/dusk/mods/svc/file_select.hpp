#pragma once

namespace dusk::mods::svc::file_select {

bool update(void* fileSelect);
bool open_new_slot(void* fileSelect);
bool start_existing_slot(void* fileSelect);
void names_confirmed(void* fileSelect);
void destroyed(void* fileSelect);
bool start_stage(void* nameScene);

}  // namespace dusk::mods::svc::file_select
