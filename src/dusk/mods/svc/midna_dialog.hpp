#pragma once

class daAlink_c;
class daMidna_c;
class dMsgScrnBase_c;
struct jmessage_tReference;

namespace dusk::mods::svc::midna_dialog {

const char* prompt_text();
bool prompt_begin();
bool prompt_resolve(int choice);
bool prompt_consume_resolution();
const char* menu_option();
bool menu_begin();
bool menu_resolve(int choice);
bool menu_cancel();
bool menu_execute_warp(void* player);
bool has_custom_flow();
bool consume_pending_flow(daMidna_c* midna, daAlink_c* player);
bool begin_custom_flow();
bool finish_custom_flow(daMidna_c* midna, bool flowDone, int choice);
bool draw_custom_prompt(dMsgScrnBase_c* screen, jmessage_tReference* reference);
bool custom_prompt_available();
bool custom_menu_option_available();
bool apply_custom_menu_option(jmessage_tReference* reference);
bool resolve_cancelled_selection();
bool resolve_cursor_selection(int choice);

}  // namespace dusk::mods::svc::midna_dialog
