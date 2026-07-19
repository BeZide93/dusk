#include "game_modes.hpp"
#include "bossrush.hpp"
#include "save_compat.hpp"

#include "global.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "d/d_com_inf_game.h"
#include "d/d_file_select.h"
#include "d/d_lib.h"
#include "d/d_meter2_info.h"
#include "d/d_s_name.h"
#include "d/d_save.h"
#include "d/d_stage.h"
#include "f_op/f_op_overlap_mng.h"
#include "f_op/f_op_msg_mng.h"
#include "m_Do/m_Do_audio.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_Reset.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <cstring>
#include <unordered_map>

IMPORT_SERVICE(HookService, svc_hook);

namespace dawnlight {
namespace {

DEFINE_HOOK(&dFile_select_c::_move, FileSelectMoveHook);
DEFINE_HOOK(&dFile_select_c::dataSelectStart, FileSelectDataSelectStartHook);
DEFINE_HOOK(&dFile_select_c::menuSelectStart, FileSelectMenuSelectStartHook);
DEFINE_HOOK(&dFile_select_c::nameInput2, FileSelectNameInput2Hook);
DEFINE_HOOK(&dScnName_c::changeGameScene, NameChangeGameSceneHook);

enum class Flow {
    None,
    NewGameTypeIn,
    NewGameTypeSelect,
    ExistingActionIn,
    ExistingActionSelect,
    StoryModeIn,
    StoryModeSelect,
    IntroSkipIn,
    IntroSkipSelect,
    CursorMove,
    CloseToStory,
    CloseToIntro,
    CloseToName,
    SourceOpen,
    SourceSelect,
    SourceMove,
    SourceReturn,
    SourceCancel,
    CancelToFiles,
    CancelToMenu,
};

enum class Prompt {
    NewGameType,
    ExistingAction,
    StoryMode,
    IntroSkip,
};

struct FileSelectState {
    Flow flow = Flow::None;
    Flow cursorReturn = Flow::None;
    u8 targetSlot = 0xff;
    u8 sourceSlot = 0xff;
    bool newGamePlus = false;
    bool introSkip = false;
    bool bossRush = false;
};

std::unordered_map<dFile_select_c*, FileSelectState> s_states;

FileSelectState& state(dFile_select_c* fileSelect) {
    return s_states[fileSelect];
}

bool valid_source(dFile_select_c* fileSelect, u8 slot) {
    return slot < 3 && !fileSelect->mIsNoData[slot] && fileSelect->mIsDataNew[slot] == 0;
}

u8 find_source(dFile_select_c* fileSelect) {
    for (u8 slot = 0; slot < 3; ++slot) {
        if (valid_source(fileSelect, slot)) {
            return slot;
        }
    }
    return 0xff;
}

void set_labels(dFile_select_c* fileSelect, const char* first, const char* second) {
    const char* labels[] = {first, second};
    for (int i = 0; i < 2; ++i) {
        auto* textBox = static_cast<J2DTextBox*>(fileSelect->mYnSelTxtPane[i]->getPanePtr());
        textBox->setString(labels[i]);
    }
}

void restore_yes_no_labels(dFile_select_c* fileSelect) {
    static constexpr u8 messageIds[] = {0x08, 0x07};
    char text[16];
    for (int i = 0; i < 2; ++i) {
        fopMsgM_messageGet(text, messageIds[i]);
        auto* textBox = static_cast<J2DTextBox*>(fileSelect->mYnSelTxtPane[i]->getPanePtr());
        textBox->setString(text);
    }
}

void set_header(dFile_select_c* fileSelect, const char* text) {
    const u8 displayIndex = fileSelect->mHeaderTxtDispIdx ^ 1;
    auto* textBox = static_cast<J2DTextBox*>(fileSelect->mHeaderTxtPane[displayIndex]->getPanePtr());
    textBox->setFont(fileSelect->fileSel.font[1]);
    textBox->setFontSize(27.0f, 27.0f);
    textBox->setLineSpace(20.0f);
    textBox->setCharSpace(0.0f);
    textBox->setString(512, text);
    fileSelect->mHeaderStringPtr[displayIndex] = textBox->getStringPtr();
    fileSelect->mHeaderTxtPane[fileSelect->mHeaderTxtDispIdx]->alphaAnimeStart(0);
    fileSelect->mHeaderTxtPane[displayIndex]->alphaAnimeStart(0);
    fileSelect->field_0x021d = 0;
}

void begin_prompt(dFile_select_c* fileSelect, Prompt prompt, Flow flow) {
    fileSelect->field_0x0268 = 0;
    fileSelect->field_0x0269 = 1;
    fileSelect->mSelIcon->setAlphaRate(0.0f);
    fileSelect->yesnoMenuMoveAnmInitSet(0x473, 0x47d);
    fileSelect->modoruTxtDispAnmInit(1);

    switch (prompt) {
    case Prompt::NewGameType:
        set_header(fileSelect, "Select new game type");
        set_labels(fileSelect, "New Game", "New Game+");
        break;
    case Prompt::ExistingAction:
        set_header(fileSelect, "Select file action");
        set_labels(fileSelect, "Start", "New Game+");
        fileSelect->menuMoveAnmInitSet(0x329, 799);
        break;
    case Prompt::StoryMode:
        set_header(fileSelect, "Select story mode");
        set_labels(fileSelect, "New Game", "Bossrush");
        break;
    case Prompt::IntroSkip:
        set_header(fileSelect, "Skip intro?");
        restore_yes_no_labels(fileSelect);
        break;
    }
    state(fileSelect).flow = flow;
}

void start_name_input(dFile_select_c* fileSelect) {
    FileSelectState& current = state(fileSelect);
#if PLATFORM_GCN
    dComIfGs_setNewFile(128);
#endif
    dComIfGs_setDataNum(current.targetSlot);
    if (current.newGamePlus && current.sourceSlot == current.targetSlot) {
        dComIfGs_getSaveData()->init();
    }

    fileSelect->mSelectNum = current.targetSlot;
    fileSelect->mSelIcon->setAlphaRate(0.0f);
    mDoAud_seStart(Z2SE_SY_NEW_FILE, nullptr, 0, 0);
    fileSelect->headerTxtSet(0x385, 1, 0);
    fileSelect->fileRecScaleAnmInitSet2(1.0f, 0.0f);
    fileSelect->nameMoveAnmInitSet(3359, 3369);
    fileSelect->mSelFileMoyoPane[current.targetSlot]->setAlpha(0);
    fileSelect->mSelFileGoldPane[current.targetSlot]->setAlpha(0);
    fileSelect->mSelFileGold2Pane[current.targetSlot]->setAlpha(0);

    char name[32];
    dMeter2Info_getString(0x382, name, nullptr);
    dComIfGs_setPlayerName(name);
    fileSelect->mpName->setNextNameStr(dComIfGs_getPlayerName());
    fileSelect->mpName->initial();
    fileSelect->modoruTxtChange(1);
    game_modes::clear_markers(dComIfGs_getSaveData());
    fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_SELECT_DATA_NAME_MOVE;
    current.flow = Flow::None;
}

void move_prompt_cursor(dFile_select_c* fileSelect, bool selectSecond, Flow returnFlow) {
    const u8 next = selectSecond ? 1 : 0;
    if (fileSelect->field_0x0268 == next) {
        return;
    }
    mDoAud_seStart(Z2SE_SY_MENU_CURSOR_COMMON, nullptr, 0, 0);
    fileSelect->field_0x0269 = fileSelect->field_0x0268;
    fileSelect->field_0x0268 = next;
    fileSelect->yesnoSelectAnmSet();
    FileSelectState& current = state(fileSelect);
    current.cursorReturn = returnFlow;
    current.flow = Flow::CursorMove;
}

bool update_prompt_input(dFile_select_c* fileSelect, Flow ownFlow) {
    fileSelect->stick->checkTrigger();
    if (fileSelect->stick->checkRightTrigger()) {
        move_prompt_cursor(fileSelect, false, ownFlow);
        return true;
    }
    if (fileSelect->stick->checkLeftTrigger()) {
        move_prompt_cursor(fileSelect, true, ownFlow);
        return true;
    }
    return false;
}

void close_prompt(dFile_select_c* fileSelect, Flow next) {
    fileSelect->mSelIcon->setAlphaRate(0.0f);
    fileSelect->yesnoMenuMoveAnmInitSet(0x47d, 0x473);
    state(fileSelect).flow = next;
}

void cancel_to_files(dFile_select_c* fileSelect) {
    mDoAud_seStart(Z2SE_SY_CURSOR_CANCEL, nullptr, 0, 0);
    fileSelect->mSelIcon->setAlphaRate(0.0f);
    fileSelect->yesnoMenuMoveAnmInitSet(0x47d, 0x473);
    fileSelect->modoruTxtDispAnmInit(0);
    fileSelect->headerTxtSet(0x43, 1, 0);
    state(fileSelect).flow = Flow::CancelToFiles;
}

void update_file_select(dFile_select_c* fileSelect, FileSelectState& current) {
    switch (current.flow) {
    case Flow::NewGameTypeIn:
    case Flow::ExistingActionIn:
    case Flow::StoryModeIn:
    case Flow::IntroSkipIn: {
        const bool ready = fileSelect->headerTxtChangeAnm() && fileSelect->yesnoMenuMoveAnm() &&
                           fileSelect->modoruTxtDispAnm();
        if (ready) {
            fileSelect->yesnoCursorShow();
            current.flow = current.flow == Flow::NewGameTypeIn ? Flow::NewGameTypeSelect :
                           current.flow == Flow::ExistingActionIn ? Flow::ExistingActionSelect :
                           current.flow == Flow::StoryModeIn ? Flow::StoryModeSelect :
                                                               Flow::IntroSkipSelect;
        }
        break;
    }
    case Flow::CursorMove:
        if (fileSelect->yesnoSelectMoveAnm() &&
            fileSelect->yesnoWakuAlpahAnm(fileSelect->field_0x0269))
        {
            fileSelect->yesnoCursorShow();
            current.flow = current.cursorReturn;
        }
        break;
    case Flow::NewGameTypeSelect:
        if (update_prompt_input(fileSelect, current.flow)) {
            break;
        }
        if (mDoCPd_c::getTrigB(PAD_1)) {
            cancel_to_files(fileSelect);
        } else if (mDoCPd_c::getTrigA(PAD_1) || mDoCPd_c::getTrigStart(PAD_1)) {
            mDoAud_seStart(Z2SE_SY_CURSOR_OK, nullptr, 0, 0);
            current.newGamePlus = fileSelect->field_0x0268 != 0;
            close_prompt(fileSelect, current.newGamePlus ? Flow::SourceOpen : Flow::CloseToStory);
        }
        break;
    case Flow::ExistingActionSelect:
        if (update_prompt_input(fileSelect, current.flow)) {
            break;
        }
        if (mDoCPd_c::getTrigB(PAD_1)) {
            mDoAud_seStart(Z2SE_SY_CURSOR_CANCEL, nullptr, 0, 0);
            fileSelect->mSelIcon->setAlphaRate(0.0f);
            fileSelect->yesnoMenuMoveAnmInitSet(0x47d, 0x473);
            fileSelect->menuMoveAnmInitSet(799, 0x329);
            fileSelect->modoruTxtDispAnmInit(0);
            current.flow = Flow::CancelToMenu;
        } else if (mDoCPd_c::getTrigA(PAD_1) || mDoCPd_c::getTrigStart(PAD_1)) {
            mDoAud_seStart(Z2SE_SY_CURSOR_OK, nullptr, 0, 0);
            if (fileSelect->field_0x0268 == 0) {
                dComIfGs_setCardToMemory(reinterpret_cast<u8*>(fileSelect->mSaveData), current.targetSlot);
                dComIfGs_setDataNum(current.targetSlot);
                fileSelect->mIsSelectEnd = true;
                fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_NEXT_MODE_WAIT;
                current.flow = Flow::None;
            } else {
                current.newGamePlus = true;
                current.sourceSlot = current.targetSlot;
                close_prompt(fileSelect, Flow::CloseToIntro);
            }
        }
        break;
    case Flow::StoryModeSelect:
        if (update_prompt_input(fileSelect, current.flow)) {
            break;
        }
        if (mDoCPd_c::getTrigB(PAD_1)) {
            cancel_to_files(fileSelect);
        } else if (mDoCPd_c::getTrigA(PAD_1) || mDoCPd_c::getTrigStart(PAD_1)) {
            mDoAud_seStart(Z2SE_SY_CURSOR_OK, nullptr, 0, 0);
            current.bossRush = fileSelect->field_0x0268 != 0;
            close_prompt(fileSelect, current.bossRush ? Flow::CloseToName : Flow::CloseToIntro);
        }
        break;
    case Flow::IntroSkipSelect:
        if (update_prompt_input(fileSelect, current.flow)) {
            break;
        }
        if (mDoCPd_c::getTrigB(PAD_1)) {
            cancel_to_files(fileSelect);
        } else if (mDoCPd_c::getTrigA(PAD_1) || mDoCPd_c::getTrigStart(PAD_1)) {
            mDoAud_seStart(Z2SE_SY_CURSOR_OK, nullptr, 0, 0);
            current.introSkip = fileSelect->field_0x0268 != 0;
            close_prompt(fileSelect, Flow::CloseToName);
        }
        break;
    case Flow::CloseToStory:
        if (fileSelect->yesnoMenuMoveAnm()) {
            begin_prompt(fileSelect, Prompt::StoryMode, Flow::StoryModeIn);
        }
        break;
    case Flow::CloseToIntro:
        if (fileSelect->yesnoMenuMoveAnm()) {
            begin_prompt(fileSelect, Prompt::IntroSkip, Flow::IntroSkipIn);
        }
        break;
    case Flow::CloseToName:
        if (fileSelect->yesnoMenuMoveAnm()) {
            start_name_input(fileSelect);
        }
        break;
    case Flow::SourceOpen:
        if (fileSelect->yesnoMenuMoveAnm()) {
            const u8 source = find_source(fileSelect);
            if (source == 0xff) {
                current.newGamePlus = false;
                begin_prompt(fileSelect, Prompt::StoryMode, Flow::StoryModeIn);
                break;
            }
            set_header(fileSelect, "Select source file");
            fileSelect->mLastSelectNum = fileSelect->mSelectNum;
            fileSelect->mSelectNum = source;
            fileSelect->dataSelectAnmSet();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_DATA_SELECT_MOVE_ANIME;
            current.flow = Flow::SourceMove;
        }
        break;
    case Flow::SourceMove:
        fileSelect->dataSelectMoveAnime();
        if (fileSelect->mDataSelProc == dFile_select_c::DATASELPROC_DATA_SELECT) {
            current.flow = Flow::SourceSelect;
        }
        break;
    case Flow::SourceSelect:
        fileSelect->stick->checkTrigger();
        if (mDoCPd_c::getTrigB(PAD_1)) {
            mDoAud_seStart(Z2SE_SY_CURSOR_CANCEL, nullptr, 0, 0);
            fileSelect->mLastSelectNum = fileSelect->mSelectNum;
            fileSelect->mSelectNum = current.targetSlot;
            fileSelect->dataSelectAnmSet();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_DATA_SELECT_MOVE_ANIME;
            current.newGamePlus = false;
            current.flow = Flow::SourceCancel;
        } else if (mDoCPd_c::getTrigA(PAD_1) || mDoCPd_c::getTrigStart(PAD_1)) {
            if (!valid_source(fileSelect, fileSelect->mSelectNum)) {
                mDoAud_seStart(Z2SE_SY_FILE_ERROR, nullptr, 0, 0);
                break;
            }
            mDoAud_seStart(Z2SE_SY_CURSOR_OK, nullptr, 0, 0);
            current.sourceSlot = fileSelect->mSelectNum;
            fileSelect->mLastSelectNum = fileSelect->mSelectNum;
            fileSelect->mSelectNum = current.targetSlot;
            fileSelect->dataSelectAnmSet();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_DATA_SELECT_MOVE_ANIME;
            current.flow = Flow::SourceReturn;
        } else if (fileSelect->stick->checkUpTrigger() && fileSelect->mSelectNum != 0) {
            fileSelect->mLastSelectNum = fileSelect->mSelectNum--;
            fileSelect->dataSelectAnmSet();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_DATA_SELECT_MOVE_ANIME;
            current.flow = Flow::SourceMove;
        } else if (fileSelect->stick->checkDownTrigger() && fileSelect->mSelectNum != 2) {
            fileSelect->mLastSelectNum = fileSelect->mSelectNum++;
            fileSelect->dataSelectAnmSet();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_DATA_SELECT_MOVE_ANIME;
            current.flow = Flow::SourceMove;
        }
        break;
    case Flow::SourceReturn:
    case Flow::SourceCancel:
        fileSelect->dataSelectMoveAnime();
        if (fileSelect->mDataSelProc == dFile_select_c::DATASELPROC_DATA_SELECT) {
            if (current.flow == Flow::SourceReturn) {
                begin_prompt(fileSelect, Prompt::IntroSkip, Flow::IntroSkipIn);
            } else {
                current.flow = Flow::None;
                fileSelect->selFileCursorShow();
            }
        }
        break;
    case Flow::CancelToFiles:
        if (fileSelect->headerTxtChangeAnm() && fileSelect->yesnoMenuMoveAnm() &&
            fileSelect->modoruTxtDispAnm())
        {
            restore_yes_no_labels(fileSelect);
            fileSelect->selFileCursorShow();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_DATA_SELECT;
            current = {};
        }
        break;
    case Flow::CancelToMenu:
        if (fileSelect->yesnoMenuMoveAnm() && fileSelect->menuMoveAnm() &&
            fileSelect->modoruTxtDispAnm())
        {
            restore_yes_no_labels(fileSelect);
            fileSelect->menuCursorShow();
            fileSelect->mDataSelProc = dFile_select_c::DATASELPROC_MENU_SELECT;
            current = {};
        }
        break;
    case Flow::None:
        break;
    }
}

bool file_update(dFile_select_c* fileSelect) {
    auto it = s_states.find(fileSelect);
    const bool active = it != s_states.end() && it->second.flow != Flow::None;
    if (active) {
        update_file_select(fileSelect, it->second);
        fileSelect->dataCopyEffAnm();
    }
    return active;
}

bool file_open(dFile_select_c* fileSelect) {
    FileSelectState& current = state(fileSelect);
    current = {};
    current.targetSlot = fileSelect->mSelectNum;
    dComIfGs_setDataNum(current.targetSlot);
    if (find_source(fileSelect) != 0xff) {
        begin_prompt(fileSelect, Prompt::NewGameType, Flow::NewGameTypeIn);
    } else {
        begin_prompt(fileSelect, Prompt::StoryMode, Flow::StoryModeIn);
    }
    return 1;
}

bool existing_start(dFile_select_c* fileSelect) {
    FileSelectState& current = state(fileSelect);
    current = {};
    current.targetSlot = fileSelect->mSelectNum;
    current.sourceSlot = current.targetSlot;
    dComIfGs_setDataNum(current.targetSlot);
    begin_prompt(fileSelect, Prompt::ExistingAction, Flow::ExistingActionIn);
    return 1;
}

void names_confirmed(dFile_select_c* fileSelect) {
    FileSelectState& current = state(fileSelect);
    if (current.newGamePlus) {
        game_modes::apply_new_game_plus(fileSelect, current.sourceSlot, current.targetSlot);
    }
    if (current.introSkip) {
        game_modes::apply_intro_skip(dComIfGs_getSaveData());
    }
    if (current.bossRush) {
        game_modes::apply_boss_rush(dComIfGs_getSaveData());
    }
    s_states.erase(fileSelect);
}

bool start_stage() {
    if (bossrush::is_active()) {
        bossrush::set_next_stage_for_current();
        return true;
    }

    if (is_intro_skipped(dComIfGs_getSaveData())) {
        if (!dComIfGs_isEventBit(dSv_event_flag_c::F_0226)) {
            dComIfGs_offSaveSwitch(dStage_SaveTbl_FARON, 12);
        }
        dComIfGs_onSaveSwitch(dStage_SaveTbl_FARON, 20);
        dSv_player_return_place_c& returnPlace =
            dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace();
        dComIfGp_setNextStage(returnPlace.getName(), returnPlace.getPlayerStatus(),
            returnPlace.getRoomNo(), -1, 0.0f, 0, 1, 0, 0, 0, 0);
        return true;
    }

    return false;
}

HookAction before_file_select_move(ModContext*, void* args, void*, void*) {
    auto* fileSelect = mods::arg<dFile_select_c*>(args, 0);
    return file_update(fileSelect) ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}

HookAction before_data_select_start(ModContext*, void* args, void*, void*) {
    auto* fileSelect = mods::arg<dFile_select_c*>(args, 0);
    if (fileSelect->mIsDataNew[fileSelect->mSelectNum] == 0 || !file_open(fileSelect)) {
        return HOOK_CONTINUE;
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_menu_select_start(ModContext*, void* args, void*, void*) {
    auto* fileSelect = mods::arg<dFile_select_c*>(args, 0);
    if (fileSelect->mSelectMenuNum != 1 || !existing_start(fileSelect)) {
        return HOOK_CONTINUE;
    }
    return HOOK_SKIP_ORIGINAL;
}

void after_name_input2(ModContext*, void* args, void*, void*) {
    auto* fileSelect = mods::arg<dFile_select_c*>(args, 0);
    auto it = s_states.find(fileSelect);
    if (it == s_states.end()) {
        return;
    }
    const FileSelectState& current = it->second;
    if (current.flow != Flow::None || (!current.newGamePlus && !current.introSkip && !current.bossRush)) {
        return;
    }
    if (fileSelect->mDataSelProc == dFile_select_c::DATASELPROC_NEXT_MODE_WAIT) {
        names_confirmed(fileSelect);
    }
}

void after_name_scene_change_game_scene(ModContext*, void*, void*, void*) {
    if (mDoRst::isReset() || fopOvlpM_IsPeek()) {
        return;
    }

    start_stage();
}

}  // namespace

ModResult install_file_select_hooks(ModError* error) {
    ModResult result = mods::hook_add_pre<FileSelectMoveHook>(svc_hook, before_file_select_move);
    if (result == MOD_OK) {
        result = mods::hook_add_pre<FileSelectDataSelectStartHook>(svc_hook, before_data_select_start);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<FileSelectMenuSelectStartHook>(svc_hook, before_menu_select_start);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<FileSelectNameInput2Hook>(svc_hook, after_name_input2);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<NameChangeGameSceneHook>(
            svc_hook, after_name_scene_change_game_scene);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight file-select hooks");
    }
    return MOD_OK;
}

}  // namespace dawnlight
