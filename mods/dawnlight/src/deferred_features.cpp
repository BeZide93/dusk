// These ports are deliberately excluded from CMake until the required services exist.
#if 0

// Complete Wii U controller style:
// The portable Z item slot and R item-wheel assignment path are active. Additional shoulder-button
// bindings, labels, touch layout changes, and the native third-item HUD still require Aurora and
// meter extension points.
void install_wii_u_controller_style();

// Touch controller editor and host settings pages:
// The Mod UI exposes Dawnlight's portable ConfigService-backed options. A host settings-page
// service is still needed for integration with the normal app settings.
void install_touch_and_settings_ui();

// Visual HUD layout editor:
// Dawnlight's hud_layout_settings.json schema is documented for compatibility, but applying the
// native meter transforms still requires a public HUD layout or meter-pane service.
void install_hud_editor_ui();

// Bullet Time and Flurry Rush:
// R Jump is active as a portable fallback. Bullet Time and Flurry Rush require finer-grained
// player, combat, projectile, and time-scale extension points.
void install_action_gameplay();

// The original umbrella entry point is intentionally not compiled.
void install_custom_game_modes();

#endif
