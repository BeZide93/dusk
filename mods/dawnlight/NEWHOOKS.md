# Host Service Delta

This branch adds neutral host services that allow `dawnlight_mod.dusk` to keep Dawnlight-specific
behavior in the package instead of hardcoding it in game files.

## Host Changes

- `CMakeLists.txt` optionally includes `mods/dawnlight` when the local source package exists.
- `FileSelectService` exposes file-select lifecycle callbacks used for New Game Plus, Intro Skip,
  and Boss Rush save creation.
- `StageFlowService` exposes stage-transition callbacks used for Boss Rush portal handling.
- `MidnaDialogService` exposes Midna prompt/menu extension points used for Boss Rush confirmations
  and the hub warp option.
- `TouchControlsService` exposes touch-control display and input overrides used for the Z item slot
  and D-Pad Down Midna button.
- `HudLayoutService` exposes HUD layout snapshots used for Dawnlight-compatible
  `hud_layout_settings.json` editing.

## Direct Hook Groups Used By The Mod

- File select and scene start:
  `dFile_select_c::_move`, `dFile_select_c::dataSelectStart`,
  `dFile_select_c::menuSelectStart`, `dFile_select_c::nameInput2`,
  and `dScnName_c::changeGameScene`.
- Boss Rush flow:
  `daObjBossWarp_c::execute`; the main Boss Rush state machine runs from `mod_update()`.
- Aim helpers:
  `daAlink_c::procBowSubject`, `daAlink_c::procBoomerangSubject`,
  `daAlink_c::procHookshotSubject`, `daAlink_c::procIronBallSubject`,
  `daAlink_c::procCopyRodSubject`, `dCamera_c::nextMode`, and `dCamera_c::nextType`.
- Manual Shielding:
  `daAlink_c::swordSwingTrigger`, `daAlink_c::setShieldGuard`,
  `daAlink_c::checkItemAction`, and `daAlink_c::procGuardAttackInit`.
- R Jump:
  `daAlink_c::checkAutoJumpAction`, `daAlink_c::procAutoJump`, and
  `daAlink_c::commonProcInit`.
- Z item slot:
  `dComIfGp_getSelectItem`, `dComIfGp_setSelectItem`, `dMenu_Ring_c::setActiveCursor`,
  `daAlink_c::midnaTalkTrigger`, `daAlink_c::checkItemButtonChange`,
  `daAlink_c::checkItemChangeFromButton`, `daAlink_c::checkSetItemTrigger`, and
  `daAlink_c::execute`.
- Save and item repairs:
  `dSv_info_c::card_to_memory`, selected item-get handlers, and selected
  `dSv_player_item_c` item mutation functions.
- Enemy HP scaling:
  `fopAc_Create`.

## Remaining Upstream API Gaps

- Controller binding/label services for Wii U style controls.
- Camera extension points for full Cinema/3rd Person profile replacement without copying camera
  code.
- Player combat-time services for Bullet Time and Flurry Rush.
