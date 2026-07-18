# Dawnlight Mod API Port

This package ports the Dawnlight features that can run on the current upstream Dusklight Mod API
without Dawnlight-specific host patches.

## Active Features

- New Game Plus creation, source-save selection, overwrite support, NG+ counters, progression-safe
  item carryover, and enemy HP scaling.
- Intro Skip creation, start-stage setup, story setup, and compatibility repairs.
- Boss Rush save creation, hub portals, repeatable encounters, sequential runs, victory hearts, and
  Boss Rush audio reset.
- Boss Rush portal confirmation dialogs and a D-Pad Down hub-warp dialog, both implemented through
  the upstream Mod UI service.
- Portable aim helpers for supported item-aim states. The mod can align Cinema aim starts toward
  the camera and keep movement/C-stick adjustments active while vanilla subject aim is running.
- Logical Z item-slot handling for an already assigned `SELECT_ITEM_DOWN` slot, including bow/bomb,
  hawkeye, and fishing-rod combination resolution.
- Manual Shielding with Target + R and Target + R + B shield attacks.
- R Jump and R+B jump attacks when no vanilla R interaction is available.
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
- `mod.dev_bezide_dawnlight.z-item-slot`
- `mod.dev_bezide_dawnlight.manual-shielding`
- `mod.dev_bezide_dawnlight.r-jump`

## Installation

Copy `dawnlight_mod.dusk` into the `mods` directory inside the active Dusklight data folder, then
restart the game or reload mods from the mod manager. Android uses the same manual installation
flow; the package is not embedded in the APK.

## Deferred Features

These standalone Dawnlight features are not fully portable yet on the current upstream API:

- Complete Wii U controller style, including native L/ZL/R/ZR labels and touch-controller layout.
- Native HUD layout editing and Dawnlight `hud_layout_settings.json` application.
- Native third item HUD rendering for the exact Z button, item icon, oil meter, and ammo layout.
- Visual HUD editor and import/export buttons in the normal app settings.
- Full third-person/cinema camera replacement for every item state.
- Bullet Time and Flurry Rush.
- Native Midna dialog/menu integration. Boss Rush currently uses mod-owned UI dialogs.

See `HOOKS.md` and `NEWHOOKS.md` in the source tree for the technical boundaries.
