# Dawnlight Upstream-Main Mod

This package runs on a clean upstream Dusklight `main` host. It only uses services that are already
available upstream, so several full Dawnlight fork features are intentionally deferred.

## Active Features

- Portable aim helpers for supported item-aim states. The mod can keep movement/C-stick
  adjustments active while vanilla subject aim is running.
- Manual Shielding with Target + R and Target + R + B shield attacks.
- R Jump and R+B jump attacks when no vanilla R interaction is available.
- Z item-slot assignment and use through existing hook points where possible. Touch-specific Z and
  Midna layout overrides are deferred until upstream exposes touch-control extension points.
- Existing Dawnlight save compatibility repairs and NG+ enemy HP scaling for saves that already
  carry Dawnlight markers.
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

These full Dawnlight fork features are not enabled in this upstream-main-only package yet:

- New Game Plus, Intro Skip, and Boss Rush creation/flow.
- Boss Rush hub portals, Midna confirmations, and hub warp.
- HUD layout editing and `hud_layout_settings.json` import/export.
- Touch-specific D-Pad Down Midna and Z item HUD/input overrides.
- Complete Wii U controller style, including native L/ZL/R/ZR labels and full physical controller
  binding UI changes.
- Wii U-specific input labels.
- Full third-person/cinema camera replacement for every item state.
- Bullet Time and Flurry Rush.

See `HOOKS.md` and `NEWHOOKS.md` in the source tree for the technical boundaries.
