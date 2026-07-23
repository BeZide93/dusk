# Dawnlight Mod API Port

This package ports Dawnlight features onto the upstream Dusklight Mod API plus the neutral
gameplay services included in this branch.

## Active Features

- New Game Plus creation, source-save selection, overwrite support, NG+ counters, progression-safe
  item carryover, and enemy HP scaling.
- Intro Skip creation, start-stage setup, story setup, and compatibility repairs.
- Boss Rush save creation, hub portals, repeatable encounters, sequential runs, victory hearts,
  Boss Rush audio reset, Midna confirmation prompts, and Midna hub warp.
- Portable aim helpers for supported item-aim states. The mod can align Cinema aim starts toward
  the camera and keep movement/C-stick adjustments active while vanilla subject aim is running.
- Manual Shielding with Target + R and Target + R + B shield attacks.
- R Jump and R+B jump attacks when no vanilla R interaction is available.
- Z item-slot assignment and use through the existing Z input. Midna moves to D-Pad Down while the
  feature is enabled, including the touch-control D-Pad Down override.
- HUD layout editing through Dawnlight-compatible `hud_layout_settings.json`, including HUD
  backing, element transforms, item/text/ammo offsets, and JSON import/export from the active data
  folder.
- Existing NG+ save repair for the Ordon sword and shield quests.
- Bee-larva bottle and invalid item-combination integrity fixes.
- Dawnlight settings inside the Mod UI.

Configuration is stored in `config.json` using these keys:

- `mod.dev_bezide_dawnlight.hp-scale-percent`
- `mod.dev_bezide_dawnlight.ngplus-auto-hp-scaling`
- `mod.dev_bezide_dawnlight.save-compatibility`
- `mod.dev_bezide_dawnlight.item-integrity-fixes`
- `mod.dev_bezide_dawnlight.aim-mode` (`0` vanilla, `1` third person, `2` cinema)
- `mod.dev_bezide_dawnlight.aim-movement`
- `mod.dev_bezide_dawnlight.manual-shielding`
- `mod.dev_bezide_dawnlight.r-jump`
- `mod.dev_bezide_dawnlight.z-item-slot`

## Installation

Copy `dawnlight_mod.dusk` into the `mods` directory inside the active Dusklight data folder, then
restart the game or reload mods from the mod manager. Android uses the same manual installation
flow; the package is not embedded in the APK.

## Deferred Features

These standalone Dawnlight features are not fully portable yet on the current upstream API:

- Complete Wii U controller style, including native L/ZL/R/ZR labels and full physical controller
  binding UI changes.
- Wii U-specific input labels.
- Full third-person/cinema camera replacement for every item state.
- Bullet Time and Flurry Rush.

See `HOOKS.md` and `NEWHOOKS.md` in the source tree for the technical boundaries.
