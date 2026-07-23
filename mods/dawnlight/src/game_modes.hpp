#pragma once

#include <types.h>

class dFile_select_c;
class dSv_save_c;

namespace dawnlight::game_modes {

void clear_markers(dSv_save_c* save);
void apply_new_game_plus(dFile_select_c* fileSelect, u8 sourceSlot, u8 targetSlot);
void apply_intro_skip(dSv_save_c* save);
void apply_boss_rush(dSv_save_c* save);

}  // namespace dawnlight::game_modes
