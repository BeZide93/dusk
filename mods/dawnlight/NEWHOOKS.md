# Upstream Service Gaps

This branch intentionally does not patch the host. The notes below track which Dawnlight features
still need additional upstream services before they can be enabled in this package.

## Current Direct Hook Groups Used By The Mod

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

## Missing Services For Deferred Dawnlight Features

- File-select/game-mode services for New Game Plus, Intro Skip, and Boss Rush creation.
- Scene-flow and Midna-dialog services for Boss Rush hub portals, confirmations, and return warps.
- Touch-control services for D-Pad Down Midna and Z item touch HUD/input overrides.
- HUD layout or meter-pane services for `hud_layout_settings.json` import/export.
- Controller binding/label services for Wii U style controls.
- Camera extension points for full Cinema/3rd Person profile replacement without copying camera
  code.
- Player combat-time services for Bullet Time and Flurry Rush.
