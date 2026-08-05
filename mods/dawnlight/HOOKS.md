# Dawnlight Upstream-Main Package

This branch keeps the host at a clean `upstream/main` state. `dawnlight_mod.dusk` only uses
services that already exist upstream:

- `HookService` for game-function hooks.
- `ConfigService` for persistent Dawnlight options.
- `UiService` for the Dawnlight mod settings window.
- `LogService` and `GameService` for ordinary mod lifecycle support.

## Active In The .dusk

- Aim movement hooks for supported subject-aim items. These replace the small item subject
  functions directly from the mod and keep movement/C-stick adjustment active while vanilla
  subject aim is running.
- Minimal camera scope suppression hooks for the Aim Mode setting. These avoid the vanilla scope
  camera in supported states without copying the whole camera implementation.
- Manual Shielding through direct player hooks: R raises the shield while targeting, and R+B starts
  Shield Attack.
- R Jump and R+B jump attack fallback through direct player hooks.
- Hook-only Z item slot support where possible through existing hooks: item-wheel assignment,
  select-item normalization, Z item use checks, and Midna relocation to D-Pad Down.
- Bee-larva bottle and invalid item-combination integrity repairs.
- Existing Dawnlight save compatibility repairs and NG+ HP scaling for saves that already carry
  Dawnlight markers.

## Intentionally Deferred

- New Game Plus, Intro Skip, and Boss Rush creation/flow. These need file-select, Midna-dialog,
  and scene-flow extension points that are not in upstream/main yet.
- Touch-control overrides for the Z item icon and D-Pad Down Midna button. These need a touch
  controls service.
- HUD layout editing and JSON import/export. These need a HUD layout or meter-pane service.
- Full Wii U controller style, labels, and physical/touch binding model. These are treated as the
  Wii U input exception for this upstream-main package.
- Full camera-profile replacement for Cinema/3rd Person aim. The hook-only package uses small
  scope-suppression hooks instead of replacing `dCamera_c::subjectCamera`.
- Bullet Time and Flurry Rush.

`res/README.md` documents the user-facing feature status for the packaged mod.
