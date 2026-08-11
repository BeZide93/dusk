# Dawnlight Mod

This package runs on a clean upstream Dusklight `main` host and uses the
upstream mod services that are available today.

## Active Features

- Z and D-Pad Down item slots with item-wheel assignment, HUD icons, ammo, oil,
  bottle contents, and combine prompts.
- Midna moved off Z when the Z item slot is enabled, including touch and prompt
  support.
- Aim Movement, Vanilla/3rd Person/Cinema aim modes, Cinema Zoom, touch/gyro aim
  helpers, and Bullet Time for bow aiming during a jump.
- Manual Shielding, R Jump, and R+B jump attacks when no vanilla R interaction is
  available.
- Intro Skip and Boss Rush new-save modes.
- Boss Rush hub, individual boss portals, Boss Rush run portal, Midna prompts,
  and return-to-hub support.
- HUD presets for GameCube, X-Box, Wii-U, and Dawnlight layouts.
- Custom HUD Layout Editor with import/export through `hud_layout_settings.json`
  in the `mods` folder.
- Save compatibility repairs, NG+ enemy HP scaling for existing Dawnlight saves,
  and item integrity fixes.
- Optional update checks against the Dawnlight GitHub releases.

## Installation

Copy `dawnlight_mod.dusk` into the `mods` directory inside the active Dusklight
data folder, then restart the game or reload mods from the mod manager. Android
uses the same manual installation flow; the package is not embedded in the APK.

## HUD Editing

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> HUD`.

The Custom HUD layout can move and scale supported HUD elements and can adjust
item, text, ammo, and button-backing offsets. `EXPORT HUD` writes
`hud_layout_settings.json` into the `mods` folder, and `IMPORT HUD` reads the
same file from that folder. The copy buttons can seed Custom from the GameCube,
X-Box, Wii-U, or Dawnlight presets.

## Deferred Features

New Game+ creation is not enabled in this upstream-main package yet because it
still needs a clean source-save selection flow from upstream services.
