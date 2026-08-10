# Dawnlight

Dawnlight is a Dusklight mod package that adds optional gameplay, controller,
aiming, boss, and HUD features for Twilight Princess.

> [!IMPORTANT]
> Dawnlight does not include or provide copyrighted game assets. You need a
> legal Dusklight installation and your own dumped copy of Twilight Princess.

## Features

- Z item slot: bind a third item to Z, move Midna to the D-Pad prompt, and use
  the Z slot from the item wheel.
- Improved item HUD support for Z, including item icons, ammo, oil, and combine
  prompts.
- Aim Movement and Aim Mode settings with Vanilla, 3rd Person, and Cinema
  options.
- Touch and gyro aiming support for the modded aiming modes.
- Manual Shielding and R Jump quality-of-life options.
- Bullet Time while aiming with the bow during a jump.
- Intro Skip and Boss Rush new-save modes. New Game+ is still deferred until
  upstream exposes the remaining mod services needed for a clean implementation.
- Boss Rush hub, individual boss portals, Boss Rush run portal, Midna prompts,
  and return-to-hub support.
- Hardcoded HUD presets for GameCube, Wii-U, and Dawnlight layouts.
- Custom HUD editing for supported HUD elements, item/text/ammo offsets, round
  X/Y buttons, and HUD import/export/reset.

## Installation

1. Download `dawnlight_mod.dusk` from the Releases page.
2. Move the file into your Dusklight mods directory:

| OS | Path |
| --- | --- |
| Windows | `%APPDATA%\TwilitRealm\Dusklight\mods` |
| Linux | `~/.local/share/TwilitRealm/Dusklight/mods` |
| macOS | `~/Library/Application Support/TwilitRealm/Dusklight/mods` |
| Android | `<active Dusklight data folder>/mods` |

3. Enable Dawnlight in the in-game Mod Manager menu.

On Android, the active data folder is the folder currently selected by
Dusklight. If you changed it with `Change Data Folder`, create or use the
`mods` folder inside that selected location.

## HUD Editing

Open `Mod Manager -> Dawnlight -> Open Dawnlight Settings -> HUD`.

The HUD layout setting has four modes:

- GameCube: vanilla-style HUD placement and backing.
- Wii-U: Wii-U inspired HUD placement with hidden button backing.
- Dawnlight: Dawnlight's compact custom layout with hidden button backing.
- Custom: editable layout, initialized from the Wii-U preset.

The Custom layout can move and scale supported HUD elements and can adjust item,
text, and ammo offsets on the HUD buttons. `EXPORT HUD` writes
`hud_layout_settings.json` into the `mods` folder, `IMPORT HUD` reads the same
file from that folder, and `RESET HUD` restores the Custom layout to the Wii-U
preset.

The `hud_layout_settings.json` format is compatible with the Dawnlight fork's
HUD layout export where the same fields are available.

## Building

The `Dawnlight Mod` GitHub Actions workflow builds `dawnlight_mod.dusk` for all
supported native platforms and combines them into one cross-platform mod bundle
when a `v*` tag is pushed.

For local development, build Dusklight once for the target platform so the SDK
headers and dependencies are available, then configure `mods/dawnlight` with
`DUSK_DIR` pointing at this repository.

## Credits

Dawnlight is maintained by BeZide93 and builds on the Dusklight mod API.

Special thanks to the [Dusklight](https://github.com/TwilitRealm/dusklight)
project, the TP decompilation team, the GC/Wii decompilation community, the
Aurora developers, the TP speedrunning community, and all contributors.
