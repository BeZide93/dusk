# Dawnlight Mod-Only Hook Status

This branch keeps the host on upstream Mod API architecture. Dawnlight behavior is implemented in
`dawnlight_mod.dusk` through upstream services plus a small set of generic provider services:

- `HookService` for direct game-function hooks.
- `ConfigService` for persistent Dawnlight options.
- `UiService` for the mod settings window and Boss Rush confirmation dialogs.
- `HudLayoutService` for HUD transforms and button style flags.
- `AimControlService` for item-aim camera mode, input routing, and subject-aim overrides.
- `ItemAssignmentService` for selecting the number of item-assignment slots.
- `ActionInputService` for generic player-action input overrides.
- `StageFlowService` for stage-flow actor behavior and sequence completion.

The host services avoid Dawnlight-specific policy names; Dawnlight decides how to use them inside
the mod package.

## Active In The .dusk

- File-select hooks for New Game Plus, Intro Skip, and Boss Rush creation.
- New Game Plus carryover, NG+ counter markers, HP scaling, and save compatibility repairs.
- Intro Skip setup and known softlock repairs.
- Boss Rush save setup, hub portal spawning, repeatable encounters, sequential runs, victory
  rewards, and audio reset.
- Boss Rush portal confirmation dialogs through `UiService`.
- Boss Rush D-Pad Down hub-warp dialog through `UiService`.
- Aim mode hooks for supported subject-aim items, including camera-facing aim start, cinema/third
  person camera selection, and left-stick movement with right-stick/gyro aim routing.
- Logical Z item-slot support for `SELECT_ITEM_DOWN`, including item resolution and basic
  combination item handling for already assigned third-slot items.
- Manual Shielding through `ActionInputService`.
- R Jump and R+B jump attack fallback through direct player hooks.
- Bee-larva bottle and invalid item-combination integrity repairs.
- HUD layout transforms through `HudLayoutService`, using the same `hud_layout_settings.json`
  schema as the standalone Dawnlight fork where possible.

## Still Deferred Without Host Support

- Full Wii U controller style, labels, and physical/touch binding model.
- Native touch controller layout/editor changes.
- Native visual HUD editor in the host settings UI.
- Native Midna dialog integration. Boss Rush uses mod-owned `UiService` dialogs instead.
- Full third-person/cinema parity for any future aim item that does not pass through
  `AimControlService`.
- Bullet Time and Flurry Rush. These need deeper player, projectile, combat-time, and time-scale
  integration than the current upstream services expose cleanly.

`res/README.md` documents the user-facing feature status for the packaged mod.
