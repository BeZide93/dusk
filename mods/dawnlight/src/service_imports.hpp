#pragma once

#include "mods/svc/config.h"
#include "mods/svc/game.h"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/save.h"
#include "mods/svc/ui.h"

extern const ConfigService* svc_config;
extern const GameService* svc_game;
extern const HookService* svc_hook;
extern const LogService* svc_log;
extern const SaveService* svc_save;
extern const UiService* svc_ui;
