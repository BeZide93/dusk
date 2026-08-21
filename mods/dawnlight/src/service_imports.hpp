#pragma once

#include "mods/svc/config.h"
#include "mods/svc/game.h"
#include "mods/svc/game_mode.h"
#include "mods/svc/hook.h"
#include "mods/svc/host.h"
#include "mods/svc/log.h"
#include "mods/svc/ui.h"

extern const ConfigService* svc_config;
extern const GameService* svc_game;
extern const GameModeService* svc_game_mode;
extern const HookService* svc_hook;
extern const HostService* svc_host;
extern const LogService* svc_log;
extern const UiService* svc_ui;
