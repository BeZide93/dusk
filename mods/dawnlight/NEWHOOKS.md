# Host Hook Delta

This branch adds generic provider services where upstream's existing Mod API cannot yet express
Dawnlight behavior through ordinary hooks alone. The host exposes narrow extension points; all
Dawnlight-specific policy stays in `mods/dawnlight`.

## Added Generic Services

- `sdk/include/mods/svc/aim_control.h`
  Implemented by `src/dusk/mods/svc/aim_control.cpp`. Lets a mod choose a generic item-aim camera
  mode, input routing mode, and optional per-item subject-aim replacement.
- `sdk/include/mods/svc/item_assignment.h`
  Implemented by `src/dusk/mods/svc/item_assignment.cpp`. Lets a mod report how many select-item
  assignment slots should be available.
- `sdk/include/mods/svc/action_input.h`
  Implemented by `src/dusk/mods/svc/action_input.cpp`. Lets a mod override generic player action
  states such as guard.
- `sdk/include/mods/svc/stage_flow.h`
  Implemented by `src/dusk/mods/svc/stage_flow.cpp`. Lets a mod influence generic stage-flow
  transition actors and final-sequence completion.
- `sdk/include/mods/svc/hud_layout.h`
  Extended with generic element flags, parent mode, slide direction, and button style flags. The
  service still provides a single render-time HUD snapshot.

## Host Callsite Groups

- Item aim callsites in `src/d/actor/d_a_alink_*` and `src/d/d_camera.cpp` query
  `AimControlService`.
- Item wheel, item resolution, meter, and touch-control callsites query `ItemAssignmentService`.
- Player guard and shield-attack decisions in `src/d/actor/d_a_alink.cpp` and
  `src/d/actor/d_a_alink_guard.inc` query `ActionInputService`.
- Boss-warp actors and final battle completion query `StageFlowService`.
- HUD meter draw/layout code reads the generic `HudLayoutService` snapshot.

## Upstream API Gaps

These are the areas where the current upstream API is still too narrow for feature parity with the
standalone Dawnlight fork:

- A host settings/file-picker service for importing, exporting, and editing HUD layouts from the
  normal settings UI.
- Controller binding/label services for Wii U style L/ZL/R/ZR controls across Aurora and touch
  controls.
- Player combat-time services for Bullet Time and Flurry Rush.
- Native Midna dialog/menu extension points if the Boss Rush prompts should appear inside the
  vanilla Midna UI instead of mod-owned dialogs.
