#pragma once

#include "mods/svc/config.h"
#include "mods/svc/flow.h"
#include "mods/svc/game.h"
#include "mods/svc/hook.h"
#include "mods/svc/host.h"
#include "mods/svc/log.h"
#include "mods/svc/message.h"
#include "mods/svc/ui.h"

extern const ConfigService* svc_config;
extern const FlowService* svc_flow;
extern const GameService* svc_game;
extern const HookService* svc_hook;
extern const HostService* svc_host;
extern const LogService* svc_log;
extern const MessageService* svc_message;
extern const UiService* svc_ui;
