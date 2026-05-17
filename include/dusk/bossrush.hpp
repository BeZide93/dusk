#pragma once

#include <types.h>

namespace dusk::bossrush {

void apply_new_save_preset();
void set_next_stage_for_current();
bool complete_ganondorf_sequence();
bool is_hub_stage();
bool is_hub_center_portal(u8 sceneListNo);
bool has_hub_midna_prompt();
const char* hub_midna_prompt_text();
bool begin_hub_midna_prompt();
bool finish_hub_midna_prompt(int choice);
bool resolve_hub_midna_prompt(int choice);
bool consume_hub_midna_prompt_resolution();
bool has_midna_hub_warp_prompt();
const char* midna_hub_warp_option_text();
bool begin_midna_hub_warp_prompt();
bool resolve_midna_hub_warp_prompt(int choice);
bool cancel_midna_hub_warp_prompt();
bool consume_midna_hub_warp_prompt_resolution();
bool consume_midna_hub_warp_request();
bool prepare_midna_hub_warp();
void warp_to_hub_now();
void update();

}  // namespace dusk::bossrush
