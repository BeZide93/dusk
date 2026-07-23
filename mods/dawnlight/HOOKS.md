# Dawnlight Hook-Only Status

This branch keeps Dawnlight behavior in `dawnlight_mod.dusk` and exposes only neutral host
extension services:

- `HookService` for direct game-function hooks.
- `ConfigService` for persistent Dawnlight options.
- `UiService` for the Dawnlight mod settings window.
- `LogService` and `GameService` for ordinary mod lifecycle support.
- `FileSelectService` for save creation flows.
- `StageFlowService` for Boss Rush transition handling.
- `MidnaDialogService` for Boss Rush confirmation prompts and hub warp.
- `TouchControlsService` for Z-item and D-Pad Down display/input overrides.
- `HudLayoutService` for Dawnlight-compatible HUD layout snapshots.

The host-side services are intended to remain generic and reusable; Dawnlight-specific behavior
stays in the package.

## Active In The .dusk

- File-select hooks for New Game Plus, Intro Skip, and Boss Rush creation.
- New Game Plus carryover, NG+ counter markers, HP scaling, and save compatibility repairs.
- Intro Skip setup and known softlock repairs.
- Boss Rush save setup, hub portal spawning, repeatable encounters, sequential runs, victory
  rewards, final-battle handoff, audio reset, Midna confirmation prompts, and Midna hub warp.
- Aim movement hooks for supported subject-aim items. These replace the small item subject
  functions directly from the mod and keep movement/C-stick adjustment active while vanilla
  subject aim is running.
- Minimal camera scope suppression hooks for the Aim Mode setting. These avoid the vanilla scope
  camera in supported states without copying the whole camera implementation.
- Manual Shielding through direct player hooks: R raises the shield while targeting, and R+B starts
  Shield Attack.
- R Jump and R+B jump attack fallback through direct player hooks.
- Hook-only Z item slot support: item-wheel assignment, select-item normalization, Z item use
  checks, and Midna relocation to D-Pad Down.
- Touch-control overrides for the Z item icon and D-Pad Down Midna button.
- HUD layout editing and JSON import/export through `HudLayoutService`.
- Bee-larva bottle and invalid item-combination integrity repairs.

## Intentionally Deferred

- Full Wii U controller style, labels, and physical/touch binding model. These are treated as the
  Wii U input exception for this hook-only branch.
- Full camera-profile replacement for Cinema/3rd Person aim. The hook-only package uses small
  scope-suppression hooks instead of replacing `dCamera_c::subjectCamera`.
- Bullet Time and Flurry Rush.

`res/README.md` documents the user-facing feature status for the packaged mod.
