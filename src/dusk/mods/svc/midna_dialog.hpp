#pragma once

namespace dusk::mods::svc::midna_dialog {

const char* prompt_text();
bool prompt_begin();
bool prompt_resolve(int choice);
bool prompt_consume_resolution();
const char* menu_option();
bool menu_begin();
bool menu_resolve(int choice);
bool menu_cancel();
bool menu_execute_action(void* player);

}  // namespace dusk::mods::svc::midna_dialog
