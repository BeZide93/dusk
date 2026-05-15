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
void update();

}  // namespace dusk::bossrush
