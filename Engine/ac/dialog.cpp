//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stack>
#include <stdio.h>
#include "ac/dialog.h"
#include "ac/common.h"
#include "ac/character.h"
#include "ac/characterinfo.h"
#include "ac/dialogtopic.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_dialog.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_room.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/keycode.h"
#include "ac/overlay.h"
#include "ac/mouse.h"
#include "ac/parser.h"
#include "ac/sys_events.h"
#include "ac/string.h"
#include "ac/spritecache.h"
#include "ac/system.h"
#include "ac/dynobj/scriptdialogoptionsrendering.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/dynobj/cc_gui.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "script/cc_instance.h"
#include "main/game_run.h"
#include "platform/base/agsplatformdriver.h"
#include "script/script.h"
#include "gfx/ddb.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;
using namespace AGS::Engine;
class DialogExec;
class DialogOptions;

extern GameSetupStruct game;
extern EnterNewRoomState in_new_room;
extern CharacterInfo*playerchar;
extern SpriteCache spriteset;
extern AGSPlatformDriver *platform;
extern int cur_mode,cur_cursor;
extern IGraphicsDriver *gfxDriver;
extern std::vector<ScriptGUI> scrGui;
extern CCGUI ccDynamicGUI;

std::vector<DialogTopic> dialog;
ScriptDialogOptionsRendering ccDialogOptionsRendering;
ScriptDrawingSurface* dialogOptionsRenderingSurface;
std::unique_ptr<DialogExec> dialogExec; // current running dialog state
std::unique_ptr<DialogOptions> dialogOpts; // current running dialog options

int said_speech_line; // used while in dialog to track whether screen needs updating

// Old dialog support
std::vector<std::vector<uint8_t>> old_dialog_scripts;
std::vector<String> old_speech_lines;

int said_text = 0;
int longestline = 0;




void Dialog_Start(ScriptDialog *sd) {
  RunDialog(sd->id);
}

#define CHOSE_TEXTPARSER -3053
#define SAYCHOSEN_USEFLAG 1
#define SAYCHOSEN_YES 2
#define SAYCHOSEN_NO  3 

int Dialog_DisplayOptions(ScriptDialog *sd, int sayChosenOption)
{
  if ((sayChosenOption < 1) || (sayChosenOption > 3))
    quit("!Dialog.DisplayOptions: invalid parameter passed");

  int chose = show_dialog_options(sd->id, (game.options[OPT_RUNGAMEDLGOPTS] != 0));

  if (chose > 0)
  {
    run_dialog_option(sd->id, chose, sayChosenOption, false /* don't run script */);
  }

  if (chose != CHOSE_TEXTPARSER)
  {
    chose++;
  }
  return chose;
}

void Dialog_SetOptionState(ScriptDialog *sd, int option, int newState) {
  SetDialogOption(sd->id, option, newState);
}

int Dialog_GetOptionState(ScriptDialog *sd, int option) {
  return GetDialogOption(sd->id, option);
}

int Dialog_HasOptionBeenChosen(ScriptDialog *sd, int option)
{
  if ((option < 1) || (option > dialog[sd->id].numoptions))
    quit("!Dialog.HasOptionBeenChosen: Invalid option number specified");
  option--;  // option id is 1-based in script, and 0 is entry point

  if (dialog[sd->id].optionflags[option] & DFLG_HASBEENCHOSEN)
    return 1;
  return 0;
}

void Dialog_SetHasOptionBeenChosen(ScriptDialog *sd, int option, bool chosen)
{
    if (option < 1 || option > dialog[sd->id].numoptions)
    {
        quit("!Dialog.HasOptionBeenChosen: Invalid option number specified");
    }
    option--; // option id is 1-based in script, and 0 is entry point
    if (chosen)
    {
        dialog[sd->id].optionflags[option] |= DFLG_HASBEENCHOSEN;
    }
    else
    {
        dialog[sd->id].optionflags[option] &= ~DFLG_HASBEENCHOSEN;
    }
}

int Dialog_GetOptionCount(ScriptDialog *sd)
{
  return dialog[sd->id].numoptions;
}

int Dialog_GetOptionsBulletGraphic()
{
    return game.dialog_bullet;
}

void Dialog_SetOptionsBulletGraphic(int sprite)
{
    game.dialog_bullet = sprite;
}

int Dialog_GetOptionsFont()
{
    return play.dialog_options_font;
}

void Dialog_SetOptionsFont(int fontnum)
{
    fontnum = ValidateFontNumber("Dialog.SetOptionsFont", fontnum);
    play.dialog_options_font = fontnum;
}

int Dialog_GetOptionsNumbering()
{
    return game.options[OPT_DIALOGNUMBERED];
}

void Dialog_SetOptionsNumbering(int style)
{
    game.options[OPT_DIALOGNUMBERED] = style;
}

int Dialog_GetOptionsHighlightColor()
{
    return play.dialog_options_highlight_color;
}

void Dialog_SetOptionsHighlightColor(int color)
{
    play.dialog_options_highlight_color = color;
}

int Dialog_GetOptionsReadColor()
{
    return play.read_dialog_option_colour;
}

void Dialog_SetOptionsReadColor(int color)
{
    play.read_dialog_option_colour = color;
}

int Dialog_GetOptionsTextAlignment()
{
    return play.dialog_options_textalign;
}

void Dialog_SetOptionsTextAlignment(int align)
{
    play.dialog_options_textalign = (HorAlignment)align;
}

int Dialog_GetOptionsGap()
{
    return game.options[OPT_DIALOGGAP];
}

void Dialog_SetOptionsGap(int gap)
{
    game.options[OPT_DIALOGGAP] = gap;
}

ScriptGUI *Dialog_GetOptionsGUI()
{
    // NOTE: historically 0 meant "no gui", so gui id 0 cannot be used here
    if (game.options[OPT_DIALOGIFACE] <= 0 || game.options[OPT_DIALOGIFACE] >= game.numgui)
        return nullptr;
    return &scrGui[game.options[OPT_DIALOGIFACE]];
}

void Dialog_SetOptionsGUI(ScriptGUI *scgui)
{
    if (scgui->id == 0)
    {
        debug_script_warn("Dialog.SetOptionsGUI: cannot assign GUI with ID 0 to dialog options");
        return;
    }

    game.options[OPT_DIALOGIFACE] = scgui->id;
}

int Dialog_GetOptionsGUIX()
{
    return play.dialog_options_gui_x;
}

void Dialog_SetOptionsGUIX(int x)
{
    play.dialog_options_gui_x = x;
}

int Dialog_GetOptionsGUIY()
{
    return play.dialog_options_gui_y;
}

void Dialog_SetOptionsGUIY(int y)
{
    play.dialog_options_gui_y = y;
}

int Dialog_GetOptionsPaddingX()
{
    return play.dialog_options_pad_x;
}

void Dialog_SetOptionsPaddingX(int x)
{
    play.dialog_options_pad_x = x;
}

int Dialog_GetOptionsPaddingY()
{
    return play.dialog_options_pad_y;
}

void Dialog_SetOptionsPaddingY(int y)
{
    play.dialog_options_pad_y = y;
}

int Dialog_GetMaxOptionsGUIWidth()
{
    return play.max_dialogoption_width;
}

void Dialog_SetMaxOptionsGUIWidth(int width)
{
    play.max_dialogoption_width = width;
}

int Dialog_GetMinOptionsGUIWidth()
{
    return play.min_dialogoption_width;
}

void Dialog_SetMinOptionsGUIWidth(int width)
{
    play.min_dialogoption_width = width;
}

int Dialog_GetShowTextParser(ScriptDialog *sd)
{
  return (dialog[sd->id].topicFlags & DTFLG_SHOWPARSER) ? 1 : 0;
}

const char* Dialog_GetOptionText(ScriptDialog *sd, int option)
{
  if ((option < 1) || (option > dialog[sd->id].numoptions))
    quit("!Dialog.GetOptionText: Invalid option number specified");

  option--; // option id is 1-based in script, and 0 is entry point

  return CreateNewScriptString(get_translation(dialog[sd->id].optionnames[option]));
}

int Dialog_GetID(ScriptDialog *sd) {
  return sd->id;
}

const char *Dialog_GetScriptName(ScriptDialog *sd)
{
    return CreateNewScriptString(game.dialogScriptNames[sd->id]);
}

//=============================================================================
// dialog manager stuff

#define RUN_DIALOG_STAY          -1
#define RUN_DIALOG_STOP_DIALOG   -2
#define RUN_DIALOG_GOTO_PREVIOUS -4

static int run_dialog_request(int parmtr)
{
    play.stop_dialog_at_end = DIALOG_RUNNING;
    RuntimeScriptValue params[]{ parmtr };
    RunScriptFunction(gameinst.get(), "dialog_request", 1, params);

    if (play.stop_dialog_at_end == DIALOG_STOP)
    {
        play.stop_dialog_at_end = DIALOG_NONE;
        return -2;
    }
    if (play.stop_dialog_at_end >= DIALOG_NEWTOPIC)
    {
        int tval = play.stop_dialog_at_end - DIALOG_NEWTOPIC;
        play.stop_dialog_at_end = DIALOG_NONE;
        return tval;
    }
    if (play.stop_dialog_at_end >= DIALOG_NEWROOM)
    {
        int roomnum = play.stop_dialog_at_end - DIALOG_NEWROOM;
        play.stop_dialog_at_end = DIALOG_NONE;
        NewRoom(roomnum);
        return -2;
    }
    play.stop_dialog_at_end = DIALOG_NONE;
    return -1;
}

void get_dialog_script_parameters(unsigned char* &script, unsigned short* param1, unsigned short* param2)
{
  script++;
  *param1 = *script;
  script++;
  *param1 += *script * 256;
  script++;
  
  if (param2)
  {
    *param2 = *script;
    script++;
    *param2 += *script * 256;
    script++;
  }
}

int run_dialog_script(int dialogID, int offse, int optionIndex)
{
  said_speech_line = 0;
  int result = RUN_DIALOG_STAY;

  if (dialogScriptsInst)
  {
    char func_name[100];
    snprintf(func_name, sizeof(func_name), "_run_dialog%d", dialogID);
    RuntimeScriptValue params[]{ optionIndex };
    RunScriptFunction(dialogScriptsInst.get(), func_name, 1, params);
    result = dialogScriptsInst->GetReturnValue();
  }
  else
  {
    // old dialog format
    if (offse == -1)
      return result;	
	
    unsigned char* script = old_dialog_scripts[dialogID].data() + offse;

    unsigned short param1 = 0;
    unsigned short param2 = 0;
    bool script_running = true;

    while (script_running)
    {
      switch (*script)
      {
        case DCMD_SAY:
          get_dialog_script_parameters(script, &param1, &param2);
          
          if (param1 == DCHAR_PLAYER)
            param1 = game.playercharacter;

          if (param1 == DCHAR_NARRATOR)
            Display(get_translation(old_speech_lines[param2].GetCStr()));
          else
            DisplaySpeech(get_translation(old_speech_lines[param2].GetCStr()), param1);

          said_speech_line = 1;
          break;

        case DCMD_OPTOFF:
          get_dialog_script_parameters(script, &param1, nullptr);
          SetDialogOption(dialogID, param1 + 1, 0, true);
          break;

        case DCMD_OPTON:
          get_dialog_script_parameters(script, &param1, nullptr);
          SetDialogOption(dialogID, param1 + 1, DFLG_ON, true);
          break;

        case DCMD_RETURN:
          script_running = false;
          break;

        case DCMD_STOPDIALOG:
          result = RUN_DIALOG_STOP_DIALOG;
          script_running = false;
          break;

        case DCMD_OPTOFFFOREVER:
          get_dialog_script_parameters(script, &param1, nullptr);
          SetDialogOption(dialogID, param1 + 1, DFLG_OFFPERM, true);
          break;

        case DCMD_RUNTEXTSCRIPT:
          get_dialog_script_parameters(script, &param1, nullptr);
          result = run_dialog_request(param1);
          script_running = (result == RUN_DIALOG_STAY);
          break;

        case DCMD_GOTODIALOG:
          get_dialog_script_parameters(script, &param1, nullptr);
          result = param1;
          script_running = false;
          break;

        case DCMD_PLAYSOUND:
          get_dialog_script_parameters(script, &param1, nullptr);
          play_sound(param1);
          break;

        case DCMD_ADDINV:
          get_dialog_script_parameters(script, &param1, nullptr);
          add_inventory(param1);
          break;

        case DCMD_SETSPCHVIEW:
          get_dialog_script_parameters(script, &param1, &param2);
          SetCharacterSpeechView(param1, param2);
          break;

        case DCMD_NEWROOM:
          get_dialog_script_parameters(script, &param1, nullptr);
          NewRoom(param1);
          if (in_new_room == kEnterRoom_None)
              in_new_room = kEnterRoom_Normal; // set only in case NewRoom was scheduled
          result = RUN_DIALOG_STOP_DIALOG;
          script_running = false;
          break;

        case DCMD_SETGLOBALINT:
          get_dialog_script_parameters(script, &param1, &param2);
          SetGlobalInt(param1, param2);
          break;

        case DCMD_GIVESCORE:
          get_dialog_script_parameters(script, &param1, nullptr);
          GiveScore(param1);
          break;

        case DCMD_GOTOPREVIOUS:
          result = RUN_DIALOG_GOTO_PREVIOUS;
          script_running = false;
          break;

        case DCMD_LOSEINV:
          get_dialog_script_parameters(script, &param1, nullptr);
          lose_inventory(param1);
          break;

        case DCMD_ENDSCRIPT:
          result = RUN_DIALOG_STOP_DIALOG;
          script_running = false;
          break;
      }
    }
  }

  if (in_new_room > 0)
    return RUN_DIALOG_STOP_DIALOG;

  if (said_speech_line > 0) {
    // the line below fixes the problem with the close-up face remaining on the
    // screen after they finish talking; however, it makes the dialog options
    // area flicker when going between topics.
    // FIXME: find a better solution rather than disable/enable iface?
    // this causes all gui texture reupdate twice, and other things...
    DisableInterface();
    UpdateGameOnce(); // redraw the screen to make sure it looks right
    EnableInterface();
    // if we're not about to abort the dialog, switch back to arrow
    if (result != RUN_DIALOG_STOP_DIALOG)
      set_mouse_cursor(CURS_ARROW);
  }

  return result;
}

// TODO: make this a member of DialogOptions class
// TODO: don't use global variables inside this function
// TODO: gather parameters into struct(s)
// TODO: limit by area height too
static int write_dialog_options(Bitmap *ds, bool ds_has_alpha, int at_x, int at_y, int areawid,
    int bullet_wid, int bullet_spr, int bullet_sprwid,
    int usingfont, int linespacing, int selected_color,
    const DialogTopic *dtop, int numdisp, int mouseison, const int *disporder, short *dispyp)
{
    // Left-to-right text direction flag
    const bool ltr_position = (game.options[OPT_RIGHTLEFTWRITE] == 0)
        || (loaded_game_file_version < kGameVersion_363);

    // Configure positioning settings
    const HorAlignment text_align = play.dialog_options_textalign;
    const std::pair<int, int> wrap_range = std::make_pair(at_x + bullet_wid, at_x + areawid - 1);
    // Extra offset for 2nd, 3rd etc lines of the same option, *relative* to the first line
    // FIXME: make this value dynamic, based on sizes etc?
    const int min_multiline_off = 9;
    int first_line_off = 0, multiline_off = 0;
    switch (text_align)
    {
    case kHAlignRight:
        if (ltr_position)
        {
            // don't offset at all when right-aligned (does not look good)
            first_line_off = 0;
            multiline_off = 0;
        }
        else
        {
            // when RTL is right aligned: apply reverse offset;
            // next lines are extra offset by either bullet_wid or min offset
            first_line_off = -bullet_wid;
            multiline_off = -bullet_wid - std::max(0, min_multiline_off - bullet_wid);
        }
        break;
    case kHAlignCenter:
        // when centering we negate wrapping range offset by half, so truly centering;
        // next lines are still offset by either bullet_wid or min offset
        first_line_off = -bullet_wid / 2;
        multiline_off = -bullet_wid / 2 + std::max(0, min_multiline_off - bullet_wid);
        break;
    default:
        if (ltr_position)
        {
            // when left aligned, next lines are offset by either bullet_wid or min offset
            first_line_off = 0;
            multiline_off = std::max(0, min_multiline_off - bullet_wid);
        }
        else
        {
            // left align for RTL is like right align for LTR, except we need to inverse bullet_wid
            first_line_off = -bullet_wid;
            multiline_off = -bullet_wid;
        }
        break;
    }

    int curyp = at_y; // next line's Y position
    for (int ww = 0; ww < numdisp; ++ww)
    {
        color_t text_color = 0;
        if ((dtop->optionflags[disporder[ww]] & DFLG_HASBEENCHOSEN) &&
            (play.read_dialog_option_colour >= 0))
        {
            // 'read' colour
            text_color = ds->GetCompatibleColor(play.read_dialog_option_colour);
        }
        else
        {
            // 'unread' colour
            text_color = ds->GetCompatibleColor(playerchar->talkcolor);
        }

        if (mouseison == ww)
        {
            // If the normal colour is the same as highlight col, then fallback to another
            // FIXME: don't use hardcoded color 13 as a fallback; maybe don't do this fallback at all?
            if (text_color == ds->GetCompatibleColor(selected_color))
                text_color = ds->GetCompatibleColor(13);
            else
                text_color = ds->GetCompatibleColor(selected_color);
        }

        const char *draw_text = skip_voiceover_token(get_translation(dtop->optionnames[disporder[ww]]));
        // TODO: make the line-splitting container also save line widths!
        break_up_text_into_lines(draw_text, Lines, wrap_range.second - wrap_range.first + 1, usingfont);
        const int first_line_wid = get_text_width_outlined(Lines[0].GetCStr(), usingfont);
        const int first_line_at = AlignInHRange(wrap_range.first, wrap_range.second, 0, first_line_wid, (FrameAlignment)text_align)
            + first_line_off;

        dispyp[ww] = curyp;
        if (bullet_spr > 0)
        {
            if (ltr_position)
                draw_gui_sprite_v330(ds, bullet_spr, first_line_at - bullet_wid, curyp, ds_has_alpha);
            else
                draw_gui_sprite_v330(ds, bullet_spr, first_line_at + first_line_wid + (bullet_wid - bullet_sprwid), curyp, ds_has_alpha);
        }
        if (game.options[OPT_DIALOGNUMBERED] == kDlgOptNumbering)
        {
            String number = String::FromFormat("%d. ", ww + 1);
            if (ltr_position)
            {
                wouttext_outline(ds, first_line_at - bullet_wid + bullet_sprwid, curyp, usingfont, text_color, number.GetCStr());
            }
            else
            {
                number.ReverseUTF8();
                wouttext_outline(ds, first_line_at + first_line_wid, curyp, usingfont, text_color, number.GetCStr());
            }
        }

        for (size_t cc = 0; cc < Lines.Count(); ++cc)
        {
            const int line_wid = (cc == 0) ? first_line_wid : get_text_width_outlined(Lines[cc].GetCStr(), usingfont);
            const int line_at = AlignInHRange(wrap_range.first, wrap_range.second, 0, line_wid, (FrameAlignment)text_align)
                + ((cc == 0) ? first_line_off : multiline_off);
            wouttext_outline(ds, line_at, curyp, usingfont, text_color, Lines[cc].GetCStr());
            curyp += linespacing;
        }

        if (ww < numdisp - 1)
            curyp += data_to_game_coord(game.options[OPT_DIALOGGAP]);
    }
    return curyp;
}

void draw_gui_for_dialog_options(Bitmap *ds, GUIMain *guib, int dlgxp, int dlgyp) {
  if (guib->GetBgColor() != 0) {
    color_t draw_color = ds->GetCompatibleColor(guib->GetBgColor());
    ds->FillRect(Rect(dlgxp, dlgyp, dlgxp + guib->GetWidth(), dlgyp + guib->GetHeight()), draw_color);
  }
  if (guib->GetBgImage() > 0)
      GfxUtil::DrawSpriteWithTransparency(ds, spriteset[guib->GetBgImage()], dlgxp, dlgyp);
}

bool get_custom_dialog_options_dimensions(int dlgnum)
{
  ccDialogOptionsRendering.Reset();
  ccDialogOptionsRendering.dialogID = dlgnum;

  getDialogOptionsDimensionsFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
  run_function_on_non_blocking_thread(&getDialogOptionsDimensionsFunc);

  if ((ccDialogOptionsRendering.width > 0) &&
      (ccDialogOptionsRendering.height > 0))
  {
    return true;
  }
  return false;
}

#define DLG_OPTION_PARSER 99

// Dialog options state
class DialogOptions : public GameState
{
public:
    DialogOptions(DialogTopic *dtop, int dlgnum, bool runGameLoopsInBackground);
    ~DialogOptions();

    // Shows and run the loop until it's over
    void Show();
    // Request dialog options to stop;
    // Note that the stopping is scheduled and is performed as soon as
    // dialog options state receives control.
    void Stop();

    // Begin the state, initialize and prepare any resources
    void Begin() override;
    // End the state, release all resources
    void End() override;
    // Draw the state
    void Draw() override;
    // Update the state during a game tick
    bool Run() override;

    DialogTopic *GetDialog() const { return dtop; }
    int GetChosenOption() const { return chose; }

private:
    void CalcOptionsHeight(int padding);
    // Process all the buffered input events; returns if handled
    bool RunControls();
    // Process single key event; returns if handled
    bool RunKey(const KeyInput &ki);
    // Process single mouse event; returns if handled
    bool RunMouse(eAGSMouseButton mbut, int mx, int my);
    // Process mouse wheel scroll
    bool RunMouseWheel(int mwheelz);


    DialogTopic *const dtop;
    const int dlgnum;
    const bool runGameLoopsInBackground;

    // dialog options rectangle on screen
    Rect position;
    // initial dialog options position; used to restore pos in case of text window offsets
    Point init_position;
    // inner position of the options texts, relative to the gui
    Point inner_position;
    int usingfont;
    int lineheight;
    int linespacing;
    int curswas;
    int bullet_wid; // full width of bullet sprite + numbering
    int bullet_picwid; // bullet sprite width
    int number_wid; // width of number component
    int needheight; // height enough to accomodate dialog options texts
    // Backwards compatibility parameters (nasty stuff);
    // remove these whenever you don't care about keeping precise alignment in old games
    int line_x_off = 0; // extra X offset for option lines (normal GUI or def surf)
    int normal_area_width_mod = 0; // inc/dec for the options drawable area (normal GUI or def surf)
    int tw_area_width_mod = 0; // inc/dec for the options drawable area (TextWindow case)
    int fixed_padding = 0; // a fixed padding value added to the setup property (normal GUI or def surf)
    int linewrap_padding = 0; // padding sum used only for linewrapping

    std::unique_ptr<GUITextBox> parserInput;
    IDriverDependantBitmap *ddb = nullptr;
    std::unique_ptr<Bitmap> optionsBitmap;

    // List of displayed options and their precalculated states;
    // NOTE: this is only used in standard options render, not custom render
    // display order of options
    int disporder[MAXTOPICOPTIONS];
    // display Y coordinate of options
    short dispyp[MAXTOPICOPTIONS];
    // number of displayed options
    int numdisp;
    // last chosen option
    int chose;
    bool doStop = false;

    int parserActivated;

    int curyp; // current (latest) draw position of a option text
    bool needRedraw;
    bool wantRefresh; // FIXME: merge with needRedraw? or better names
    bool is_textwindow;
    bool is_normalgui;
    bool usingCustomRendering;
    bool newCustomRender; // using newer (post-3.5.0 render API)
    // width of a region within the gui where options are arranged;
    // includes internal gui padding (from both sides)
    int areawid;
    int forecol;

    int mouseison;
};

void DialogOptions::CalcOptionsHeight(int padding)
{
    // According to the options drawing logic:
    // * the distance between LINES in a multiline option is font linespacing;
    // * the distance between OPTIONS is font linespacing + OPT_DIALOGGAP;
    // * IF there's a parser input, we also add another linespacing + OPT_DIALOGGAP
    //   after the last option;
    // * IF there's no parser, then we add *font's graphical height* + OPT_DIALOGGAP,
    //   to ensure that the text is not going to be cut off visually.
    needheight = 0;
    int total_lines = 0;
    // TODO: cache breaking text into lines, don't repeat the process in Draw
    for (int i = 0; i < numdisp; ++i)
    {
        const char *draw_text = skip_voiceover_token(get_translation(dtop->optionnames[disporder[i]]));
        break_up_text_into_lines(draw_text, Lines, areawid - (2 * padding + 2 + bullet_wid), usingfont);
        total_lines += Lines.Count();
    }
    needheight = linespacing * (total_lines - 1) + data_to_game_coord(game.options[OPT_DIALOGGAP]) * (numdisp - 1);
    if (parserInput)
    {
        needheight += linespacing + data_to_game_coord(game.options[OPT_DIALOGGAP]);
        needheight += parserInput->GetHeight() + data_to_game_coord(game.options[OPT_DIALOGGAP]);
    }
    else
    {
        needheight += get_text_lines_height(usingfont, 1) + data_to_game_coord(game.options[OPT_DIALOGGAP]);
    }
}

DialogOptions::DialogOptions(DialogTopic *dtop_, int dlgnum_, bool runGameLoopsInBackground_)
    : dtop(dtop_)
    , dlgnum(dlgnum_)
    , runGameLoopsInBackground(runGameLoopsInBackground_)
{
}

DialogOptions::~DialogOptions()
{
    if (ddb != nullptr)
        gfxDriver->DestroyDDB(ddb);
    optionsBitmap.reset();
    parserInput.reset();
}

void DialogOptions::Show()
{
    Begin();
    Draw();
    while (Run());
    End();
}

void DialogOptions::Stop()
{
    doStop = true;
}

void DialogOptions::Begin()
{
    doStop = false;
    chose = -1;
    // First of all, decide which options should be displayed this turn
    numdisp = 0;
    for (int i = 0; i < dtop->numoptions; ++i)
    {
        if ((dtop->optionflags[i] & DFLG_ON)==0)
            continue; // option is off

        if (strlen(dtop->optionnames[i]) == 0)
            continue; // do not add an empty option name into the display list

        // Add this option into the display list
        disporder[numdisp++] = i;
    }

    if (loaded_game_file_version >= kGameVersion_363)
        usingfont = play.dialog_options_font;
    else
        usingfont = FONT_NORMAL;
    
    lineheight = get_font_height_outlined(usingfont);
    linespacing = get_font_linespacing(usingfont);
    curswas=cur_cursor;
    bullet_wid = 0;
    bullet_picwid = 0;
    number_wid = 0;
    ddb = nullptr;
    optionsBitmap = nullptr;
    parserInput = nullptr;
    said_text = 0;

    // Historically dialog options alignment calculation had pretty
    // inconsistent rules. We keep some of these strictly for backwards
    // compatible mode. Should remove these variables if we drop compatibility.
    if (loaded_game_file_version < kGameVersion_363)
    {
        line_x_off = 1; // extra X offset for option lines
        normal_area_width_mod = -5; // extra reduction for the options text (normal gui)
        tw_area_width_mod = +4; // extra increment for the options text (textwindow)
        fixed_padding = TEXTWINDOW_PADDING_DEFAULT;
    }

    if (game.dialog_bullet > 0)
    {
        bullet_picwid = game.SpriteInfos[game.dialog_bullet].Width + 3;
    }

    // numbered options, leave space for the numbers
    bullet_wid = bullet_picwid;
    if (game.options[OPT_DIALOGNUMBERED] == kDlgOptNumbering)
    {
        number_wid = get_text_width_outlined("9. ", usingfont);
        bullet_wid += number_wid;
    }

    play.in_conversation++;
    set_mouse_cursor(CURS_ARROW);

    parserActivated = 0;
    if ((dtop->topicFlags & DTFLG_SHOWPARSER) && (play.disable_dialog_parser == 0)) {
        parserInput.reset(new GUITextBox());
        parserInput->SetHeight(lineheight + get_fixed_pixel_size(4));
        parserInput->SetShowBorder(true);
        parserInput->SetFont(usingfont);
    }

    is_normalgui = false;
    is_textwindow = false;
    position = {};
    init_position = {};
    inner_position = {};
    forecol = play.dialog_options_highlight_color;

    mouseison = -1;
    usingCustomRendering = false;

    if (get_custom_dialog_options_dimensions(dlgnum))
    {
        // Custom dialog options rendering
        usingCustomRendering = true;
        position = RectWH(
            data_to_game_coord(ccDialogOptionsRendering.x),
            data_to_game_coord(ccDialogOptionsRendering.y),
            data_to_game_coord(ccDialogOptionsRendering.width),
            data_to_game_coord(ccDialogOptionsRendering.height));
    }
    else if (game.options[OPT_DIALOGIFACE] > 0)
    {
        // Use GUI or TextWindow GUI
        GUIMain*guib=&guis[game.options[OPT_DIALOGIFACE]];
        if (guib->IsTextWindow())
        {
            // Text-window, so do the QFG4-style speech options
            is_textwindow = true;
            forecol = guib->GetFgColor();

            // TODO: we do not do CalcOptionsHeight() here in case of a textwindow,
            // because it requires adjustments done by draw_text_window (like padding).
            // refactoring draw_text_window() may solve this issue.
        }
        else
        {
            // Normal GUI
            is_normalgui = true;
            position = guib->GetRect();

            areawid = guib->GetWidth() + normal_area_width_mod;
            linewrap_padding = play.dialog_options_pad_x + fixed_padding;
            CalcOptionsHeight(linewrap_padding);

            if (game.options[OPT_DIALOGUPWARDS])
            {
                // They want the options upwards from the bottom
                // FIXME: this setting is lying: it does not reverse the order, only aligns opts to the bottom of GUI
                position.MoveToY((guib->GetY() + guib->GetHeight()) - needheight);
            }
        }
    }
    else
    {
        // Default plain surface
        const Rect &ui_view = play.GetUIViewport();
        areawid = ui_view.GetWidth() + normal_area_width_mod;
        linewrap_padding = play.dialog_options_pad_x + fixed_padding;
        CalcOptionsHeight(linewrap_padding);

        position = RectWH(
            0,
            ui_view.GetHeight() - needheight,
            ui_view.GetWidth(),
            needheight);
    }

    newCustomRender = usingCustomRendering && game.options[OPT_DIALOGOPTIONSAPI] >= 0;
    init_position = position.GetLT();
    needRedraw = false;
    wantRefresh = false;
    mouseison=-10;

    // Disable rest of the game interface while the dialog options are displayed;
    // note that this also disables "overhotspot" labels update
    DisableInterfaceEx(false /* don't change cursor */);

    // Must update drawable states, in case they have not been initialized yet,
    // because we call Draw() once prior to Run().
    // TODO: need to revise the update logic within DialogOptions state,
    // maybe this will be not necessary to do after some adjustments.
    UpdateCursorAndDrawables();
}

void DialogOptions::Draw()
{
    wantRefresh = true;

    if (usingCustomRendering)
    {
        recycle_bitmap(optionsBitmap, game.GetColorDepth(),
            std::max(1, data_to_game_coord(ccDialogOptionsRendering.width)),
            std::max(1, data_to_game_coord(ccDialogOptionsRendering.height)));
    }
    else
    {
        recycle_bitmap(optionsBitmap, game.GetColorDepth(),
            std::max(1, position.GetWidth()),
            std::max(1, position.GetHeight()));
    }

    optionsBitmap->ClearTransparent();
    position.MoveTo(init_position);
    std::fill(dispyp, dispyp + MAXTOPICOPTIONS, 0);

    const Rect &ui_view = play.GetUIViewport();

    bool options_surface_has_alpha = false;

    if (usingCustomRendering)
    {
      // Custom dialog options rendering
      ccDialogOptionsRendering.surfaceToRenderTo = dialogOptionsRenderingSurface;
      ccDialogOptionsRendering.surfaceAccessed = false;
      dialogOptionsRenderingSurface->linkedBitmapOnly = optionsBitmap.get();
      dialogOptionsRenderingSurface->hasAlphaChannel = ccDialogOptionsRendering.hasAlphaChannel;
      options_surface_has_alpha = dialogOptionsRenderingSurface->hasAlphaChannel != 0;

      renderDialogOptionsFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
      run_function_on_non_blocking_thread(&renderDialogOptionsFunc);

      if (!ccDialogOptionsRendering.surfaceAccessed)
          debug_script_warn("dialog_options_get_dimensions was implemented, but no dialog_options_render function drew anything to the surface");

      if (parserInput)
      {
        parserInput->SetX(data_to_game_coord(ccDialogOptionsRendering.parserTextboxX));
        curyp = data_to_game_coord(ccDialogOptionsRendering.parserTextboxY);
        areawid = data_to_game_coord(ccDialogOptionsRendering.parserTextboxWidth);
        if (areawid == 0)
          areawid = optionsBitmap->GetWidth();
      }
      ccDialogOptionsRendering.needRepaint = false;
    }
    else if (is_textwindow)
    {
      // Text window behind the options
      areawid = data_to_game_coord(play.max_dialogoption_width);
      int biggest = 0;
      const int padding = guis[game.options[OPT_DIALOGIFACE]].GetPadding();
      for (int i = 0; i < numdisp; ++i) {
        const char *draw_text = skip_voiceover_token(get_translation(dtop->optionnames[disporder[i]]));
        break_up_text_into_lines(draw_text, Lines, areawid-((2*padding+2)+bullet_wid), usingfont);
        if (longestline > biggest)
          biggest = longestline;
      }
      if (biggest < areawid - ((2*padding + 2 + tw_area_width_mod)+bullet_wid))
        areawid = biggest + ((2*padding + 2 + tw_area_width_mod)+bullet_wid);

      areawid = std::max(areawid, data_to_game_coord(play.min_dialogoption_width));

      CalcOptionsHeight(padding);

      const int savedwid = areawid;
      int txoffs=0,tyoffs=0,yspos = ui_view.GetHeight()/2-(2*padding+needheight)/2;
      int xspos = ui_view.GetWidth()/2 - areawid/2;
      // shift window to the right if QG4-style full-screen pic
      if ((game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4) && (said_text > 0))
        xspos = (ui_view.GetWidth() - areawid) - get_fixed_pixel_size(10);

      // needs to draw the right text window, not the default
      std::unique_ptr<Bitmap> text_window_ds =
        draw_text_window(&txoffs,&tyoffs,&xspos,&yspos,&areawid,nullptr,needheight, game.options[OPT_DIALOGIFACE], DisplayVars());
      options_surface_has_alpha = guis[game.options[OPT_DIALOGIFACE]].HasAlphaChannel();
      // since draw_text_window incrases the width, restore the inner placement
      areawid = savedwid;

      // Ignore the dialog_options_pad_x/y offsets when using a text window
      // because it has its own padding property
      position = RectWH(xspos, yspos, text_window_ds->GetWidth(), text_window_ds->GetHeight());
      inner_position = Point(txoffs, tyoffs);
      optionsBitmap = std::move(text_window_ds);

      // NOTE: presumably, txoffs and tyoffs are already offset by padding,
      // although it's not entirely reliable, because these calculations are done inside draw_text_window.
      const int opts_areawid = areawid - (2 * padding + 2);
      curyp = write_dialog_options(optionsBitmap.get(), options_surface_has_alpha, inner_position.X, inner_position.Y, opts_areawid,
                                   bullet_wid, game.dialog_bullet, bullet_picwid,
                                   usingfont, linespacing, forecol,
                                   dtop, numdisp, mouseison, disporder, dispyp);
      if (parserInput)
        parserInput->SetX(inner_position.X);
    }
    else
    {
      // Normal GUI or default surface
      Bitmap *ds = optionsBitmap.get();
      if (wantRefresh)
      {
        // redraw the background so that anti-alias fonts don't re-alias themselves
        if (game.options[OPT_DIALOGIFACE] == 0)
        {
          // Default surface
          color_t draw_color = ds->GetCompatibleColor(16);
          ds->FillRect(RectWH(position.GetSize()), draw_color);
        }
        else
        {
          // Normal GUI
          GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
          if (!guib->IsTextWindow())
            draw_gui_for_dialog_options(ds, guib, 0, 0);
        }
      }

      if (game.options[OPT_DIALOGIFACE] > 0) 
      {
        // the whole GUI area should be marked dirty in order
        // to ensure it gets drawn
        GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
        options_surface_has_alpha = guib->HasAlphaChannel();
      }
      else
      {
        options_surface_has_alpha = false;
      }

      inner_position = Point(play.dialog_options_pad_x + line_x_off, play.dialog_options_pad_y);
      const int opts_areawid = areawid - (2 * linewrap_padding + 2);
      curyp = inner_position.Y;
      curyp = write_dialog_options(ds, options_surface_has_alpha, inner_position.X, inner_position.Y, opts_areawid,
                                   bullet_wid, game.dialog_bullet, bullet_picwid,
                                   usingfont, linespacing, forecol,
                                   dtop, numdisp, mouseison, disporder, dispyp);

      if (parserInput)
        parserInput->SetX(inner_position.X);
    }

    if (parserInput)
    {
      // Set up the text box, if present
      parserInput->SetY(curyp + data_to_game_coord(game.options[OPT_DIALOGGAP]));
      parserInput->SetWidth(areawid - get_fixed_pixel_size(10));
      parserInput->SetTextColor(playerchar->talkcolor);
      if (mouseison == DLG_OPTION_PARSER)
        parserInput->SetTextColor(forecol);

      // Left-to-right text direction flag
      const bool ltr_position = (game.options[OPT_RIGHTLEFTWRITE] == 0)
          || (loaded_game_file_version < kGameVersion_363);

      parserInput->SetWidth(parserInput->GetWidth() - bullet_wid);
      if (ltr_position)
        parserInput->SetX(parserInput->GetX() + bullet_wid);

      const int parserx = parserInput->GetX();
      const int parsery = parserInput->GetY();
      Bitmap *ds = optionsBitmap.get();
      if (game.dialog_bullet)
      {
          if (ltr_position)
            draw_gui_sprite_v330(ds, game.dialog_bullet, parserx - bullet_wid, parsery, options_surface_has_alpha);
          else
            draw_gui_sprite_v330(ds, game.dialog_bullet, parserx + parserInput->GetWidth() + (bullet_wid - bullet_picwid + 1), parsery, options_surface_has_alpha);
      }

      parserInput->Draw(ds, parserx, parsery);
      parserInput->SetActivated(false);
    }

    wantRefresh = false;

    // Apply custom GUI position (except when it's fully custom options rendering)
    if (!usingCustomRendering)
    {
        if (play.dialog_options_gui_x >= 0)
            position.MoveToX(play.dialog_options_gui_x);
        if (play.dialog_options_gui_y >= 0)
            position.MoveToY(play.dialog_options_gui_y);
    }

    ddb = recycle_ddb_bitmap(ddb, optionsBitmap.get(), options_surface_has_alpha, false);

    // FIXME: this operation, combined with the UpdateGameOnce/render calls
    // in Run() actually causes the scene to draw TWICE per game frame!
    // ...somehow this fact was overlooked during earlier refactors.
    // But fixing this might require caution, as DialogOptions own
    // update is mixed with the game update, noteably input events handling.
    if (runGameLoopsInBackground)
    {
        render_graphics(ddb, position.Left, position.Top);
    }
}

bool DialogOptions::Run()
{
    // Run() can be called in a loop, so keep events going.
    sys_evt_process_pending();

    // Optionally run full game update, otherwise only minimal auto & overlay update
    if (runGameLoopsInBackground)
    {
        UpdateGameOnce(false, ddb, position.Left, position.Top);
    }
    else
    {
        update_audio_system_on_game_loop();
        UpdateCursorAndDrawables();
        render_graphics(ddb, position.Left, position.Top);
    }

    // Stop the dialog options if wsa requested from script
    if (doStop)
        return false;

    needRedraw = false;

    // If there are no displayed options, and we are using standard options render,
    // then bail out as emergency, because otherwise player will be stuck forever.
    if ((numdisp == 0) && !usingCustomRendering)
    {
        debug_script_warn("WARNING: No dialog options to display, abort dialog");
        return false;
    }

    // For >= 3.4.0 custom options rendering: run "dialog_options_repexec"
    if (newCustomRender)
    {
        runDialogOptionRepExecFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&runDialogOptionRepExecFunc);

        // Stop the dialog options if requested from script
        if (doStop)
            return false;
    }

    // Handle mouse over options
    int mousewason = mouseison;
    mouseison = -1;
    if (newCustomRender)
    {
        // New custom rendering: do not automatically detect option under mouse
    }
    else if (usingCustomRendering)
    {
        // Old custom rendering
        if (position.IsInside(mousex, mousey))
        {
            // Run "dialog_options_get_active"
            getDialogOptionUnderCursorFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            run_function_on_non_blocking_thread(&getDialogOptionUnderCursorFunc);

            if (!getDialogOptionUnderCursorFunc.AtLeastOneImplementationExists)
                quit("!The script function dialog_options_get_active is not implemented. It must be present to use a custom dialogue system.");

            // Stop the dialog options if requested from script
            if (doStop)
                return false;

            mouseison = ccDialogOptionsRendering.activeOptionID;
        }
        else
        {
            ccDialogOptionsRendering.activeOptionID = -1;
        }
    }
    else if (Rect(position.Left + inner_position.X,
                  position.Top  + inner_position.Y,
                  position.Left + inner_position.X + areawid,
                  position.Top  + curyp).IsInside(mousex, mousey))
    {
        // Default rendering: detect option under mouse
        const int rel_mousey = mousey - position.Top;
        mouseison = numdisp-1;
        for (int i = 0; i < numdisp; ++i)
        {
            if (rel_mousey < dispyp[i]) { mouseison=i-1; break; }
        }
        if ((mouseison<0) | (mouseison>=numdisp)) mouseison=-1;
    }

    // Handle mouse over parser
    if (parserInput)
    {
        const int rel_mousey = mousey - position.Top;
        if ((rel_mousey > parserInput->GetY()) &&
            (rel_mousey < parserInput->GetY() + parserInput->GetHeight()))
            mouseison = DLG_OPTION_PARSER;
    }

    // Handle player's input
    RunControls();

    // Stop the dialog options if requested from script
    if (doStop)
        return false;

    // Post user input, processing changes
    if (newCustomRender)
    {
        // New-style custom rendering: check its explicit flag;
        // could be set by setting ActiveOptionID, or calling Update()
        needRedraw |= ccDialogOptionsRendering.needRepaint;
    }
    else
    {
        // Default rendering and old-style custom rendering:
        // test if an active option has changed
        needRedraw |= (mousewason != mouseison);
    }

    // Handle new parser's state
    if (parserInput && parserInput->IsActivated())
    {
        parserActivated = 1;
    }

    // Get if any option has been chosen
    if (chose >= 0)
    { // already have one set by default behavior
    }
    else if (parserActivated)
    {
        // They have selected a custom parser-based option
        if (!parserInput->GetText().IsEmpty() != 0)
        {
            chose = DLG_OPTION_PARSER;
        }
        else
        {
            parserActivated = 0;
            parserInput->SetActivated(false);
        }
    }
    else if (newCustomRender)
    {
        // New custom rendering: see if RunActiveOption was called
        if (ccDialogOptionsRendering.chosenOptionID >= 0)
        {
            chose = ccDialogOptionsRendering.chosenOptionID;
            ccDialogOptionsRendering.chosenOptionID = -1;
        }
    }

    // Finally, if the option has been chosen, then break the options loop
    if (chose >= 0)
        return false;

    // Redraw if needed
    if (needRedraw)
        Draw();

    // Go for another options loop round
    update_polled_stuff();
    if (!runGameLoopsInBackground && (play.fast_forward == 0))
    { // NOTE: if runGameLoopsInBackground then it's called inside UpdateGameOnce
        WaitForNextFrame();
    }
    return true; // continue running loop
}

bool DialogOptions::RunControls()
{
    bool state_handled = false;
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        if (type == kInputKeyboard)
        {
            KeyInput ki;
            if (!run_service_key_controls(ki) || state_handled)
                continue; // handled by engine layer, or resolved
            if (!play.IsIgnoringInput() && RunKey(ki))
            {
                state_handled = true; // handled
            }
        }
        else if (type == kInputMouse)
        {
            eAGSMouseButton mbut;
            Point mpos;
            if (!run_service_mb_controls(mbut, &mpos) || state_handled)
                continue; // handled by engine layer, or resolved
            if (!play.IsIgnoringInput() && RunMouse(mbut, mpos.X, mpos.Y))
            {
                state_handled = true; // handled
            }
        }
        else
        {
            ags_drop_next_inputevent();
        }
    }
    // Finally handle mouse wheel
    const int wheel = ags_check_mouse_wheel(); // poll always, otherwise it accumulates
    if (!state_handled)
        state_handled = RunMouseWheel(wheel);
    return state_handled;
}

bool DialogOptions::RunKey(const KeyInput &ki)
{
    const bool old_keyhandle = game.options[OPT_KEYHANDLEAPI] == 0;

    const eAGSKeyCode agskey = ki.Key;
    if (parserInput)
    {
        wantRefresh = true;
        // type into the parser 
        // TODO: find out what are these key commands, and are these documented?
        if ((agskey == eAGSKeyCodeF3) || ((agskey == eAGSKeyCodeSpace) && (parserInput->GetText().GetLength() == 0)))
        {
            // write previous contents into textbox (F3 or Space when box is empty)
            size_t last_len = ustrlen(play.lastParserEntry);
            size_t cur_len = ustrlen(parserInput->GetText().GetCStr());
            // [ikm] CHECKME: tbh I don't quite get the logic here (it was like this in original code);
            // but what we do is copying only the last part of the previous string
            if (cur_len < last_len)
            {
                const char *entry = play.lastParserEntry;
                // TODO: utility function for advancing N utf-8 chars
                for (size_t i = 0; i < cur_len; ++i) ugetxc(&entry);
                parserInput->SetText(String::FromFormat("%s%s", parserInput->GetText().GetCStr(), entry));
            }
            needRedraw = true;
            return true; // handled
        }
        else if ((ki.UChar > 0) || (agskey == eAGSKeyCodeReturn) || (agskey == eAGSKeyCodeBackspace))
        {
            bool handled = parserInput->OnKeyPress(ki);
            needRedraw = handled;
            return handled;
        }
    }
    else if (newCustomRender)
    {
        if (old_keyhandle || (ki.UChar == 0))
        { // "dialog_options_key_press"
            runDialogOptionKeyPressHandlerFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionKeyPressHandlerFunc.Params[1].SetInt32(AGSKeyToScriptKey(ki.Key));
            runDialogOptionKeyPressHandlerFunc.Params[2].SetInt32(ki.Mod);
            run_function_on_non_blocking_thread(&runDialogOptionKeyPressHandlerFunc);
        }
        if (!old_keyhandle && (ki.UChar > 0))
        { // "dialog_options_text_input"
            runDialogOptionTextInputHandlerFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionTextInputHandlerFunc.Params[1].SetInt32(ki.UChar);
            run_function_on_non_blocking_thread(&runDialogOptionKeyPressHandlerFunc);
        }
        return (old_keyhandle || (ki.UChar == 0)) || (!old_keyhandle && (ki.UChar > 0));
    }
    // Allow selection of options by keyboard shortcuts
    else if (game.options[OPT_DIALOGNUMBERED] >= kDlgOptKeysOnly &&
        agskey >= '1' && agskey <= '9')
    {
        int numkey = agskey - '1';
        if (numkey < numdisp)
        {
            chose = disporder[numkey];
            return true; // handled
        }
    }
    return false; // not handled
}

bool DialogOptions::RunMouse(eAGSMouseButton mbut, int mx, int my)
{
    if (mbut > kMouseNone)
    {
        if (mouseison < 0 && !newCustomRender)
        {
            if (usingCustomRendering)
            {
                runDialogOptionMouseClickHandlerFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
                runDialogOptionMouseClickHandlerFunc.Params[1].SetInt32(mbut);
                run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);
                needRedraw = runDialogOptionMouseClickHandlerFunc.AtLeastOneImplementationExists;
            }
        }
        else if (mouseison == DLG_OPTION_PARSER)
        {
            // they clicked the text box
            parserActivated = 1;
        }
        else if (newCustomRender)
        {
            runDialogOptionMouseClickHandlerFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionMouseClickHandlerFunc.Params[1].SetInt32(mbut);
            runDialogOptionMouseClickHandlerFunc.Params[2].SetInt32(mx);
            runDialogOptionMouseClickHandlerFunc.Params[3].SetInt32(my);
            run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);
        }
        else if (usingCustomRendering)
        {
            chose = mouseison;
        }
        else
        {
            chose = disporder[mouseison];
        }
        return true; // always treat handled, any mouse button does the same
    }
    return false; // not handled
}

bool DialogOptions::RunMouseWheel(int mwheelz)
{
    if ((mwheelz != 0) && usingCustomRendering)
    {
        runDialogOptionMouseClickHandlerFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
        runDialogOptionMouseClickHandlerFunc.Params[1].SetInt32((mwheelz < 0) ? 9 : 8);
        run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);
        needRedraw = !newCustomRender && runDialogOptionMouseClickHandlerFunc.AtLeastOneImplementationExists;
        return true; // handled
    }
    return false; // not handled
}

void DialogOptions::End()
{
    // Close custom dialog options
    if (usingCustomRendering)
    {
        runDialogOptionCloseFunc.Params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&runDialogOptionCloseFunc);
    }

  invalidate_screen();

  if (parserActivated) 
  {
    assert(parserInput);
    snprintf(play.lastParserEntry, MAX_MAXSTRLEN, "%s", parserInput->GetText().GetCStr());
    ParseText (parserInput->GetText().GetCStr());
    chose = CHOSE_TEXTPARSER;
  }

  if (ddb != nullptr)
    gfxDriver->DestroyDDB(ddb);
  ddb = nullptr;
  optionsBitmap.reset();
  parserInput.reset();

  set_mouse_cursor(curswas);
  // In case it's the QFG4 style dialog, remove the black screen
  play.in_conversation--;
  remove_screen_overlay(OVER_COMPLETE);

  // Re-enabled the game GUI
  EnableInterfaceEx(false /* don't change cursor */);
}

int run_dialog_entry(int dlgnum)
{
    DialogTopic *dialog_topic = &dialog[dlgnum];
    // Run global event kScriptEvent_DialogRun for the startup entry (index 0)
    run_on_event(kScriptEvent_DialogRun, dlgnum, 0);
    return run_dialog_script(dlgnum, dialog_topic->startupentrypoint, 0);
}

int run_dialog_option(int dlgnum, int dialog_choice, int sayChosenOption, bool run_script)
{
    assert(dialog_choice >= 0 && dialog_choice < MAXTOPICOPTIONS);
    DialogTopic *dialog_topic = &dialog[dlgnum];
    int &option_flags = dialog_topic->optionflags[dialog_choice];
    const char *option_name = dialog_topic->optionnames[dialog_choice];

    // Run global event kScriptEvent_DialogRun for the new option
    run_on_event(kScriptEvent_DialogRun, dlgnum, dialog_choice + 1);

    option_flags |= DFLG_HASBEENCHOSEN;
    bool sayTheOption = false;
    if (sayChosenOption == SAYCHOSEN_YES)
    {
        sayTheOption = true;
    }
    else if (sayChosenOption == SAYCHOSEN_USEFLAG)
    {
        sayTheOption = ((option_flags & DFLG_NOREPEAT) == 0);
    }

    // Optionally "say" the option's text
    if (sayTheOption)
        DisplaySpeech(get_translation(option_name), game.playercharacter);

    // Run the option script
    if (run_script)
        return run_dialog_script(dlgnum, dialog_topic->entrypoints[dialog_choice], dialog_choice + 1);

    return 0; // no script, bail out
}

int show_dialog_options(int dlgnum, bool runGameLoopsInBackground) 
{
  if ((dlgnum < 0) || (dlgnum >= game.numdialog))
  {
    quit("!RunDialog: invalid dialog number specified");
  }

  can_run_delayed_command();

  DialogTopic *dtop = &dialog[dlgnum];

  // First test if there are enough valid options to run DialogOptions
  int opt_count = 0;
  int last_opt = -1;
  for (int i = 0; i < dtop->numoptions; ++i)
  {
    if ((dtop->optionflags[i] & DFLG_ON) != 0)
    {
      last_opt = i;
      opt_count++;
    }
  }

  if (opt_count < 1)
  {
    debug_script_warn("Dialog: all options have been turned off, stopping dialog.");
    return -1;
  }
  // Don't display the options if there is only one and the parser is not enabled.
  const bool has_parser = (dtop->topicFlags & DTFLG_SHOWPARSER) && (play.disable_dialog_parser == 0);
  if (!has_parser && (opt_count == 1) && !play.show_single_dialog_option)
  {
    return last_opt; // only one choice, so select it
  }

  // Run the global DialogOptionsOpen event
  run_on_event(kScriptEvent_DialogOptionsOpen, dlgnum);

  dialogOpts.reset(new DialogOptions(dtop, dlgnum, runGameLoopsInBackground));
  dialogOpts->Show();

  // Run the global DialogOptionsClose event
  run_on_event(kScriptEvent_DialogOptionsClose, dlgnum, dialogOpts->GetChosenOption());

  const int chosen = dialogOpts->GetChosenOption();
  dialogOpts = {};
  return chosen;
}

// Dialog execution state
// TODO: reform into GameState implementation, similar to DialogOptions!
class DialogExec
{
public:
    DialogExec(int start_dlgnum)
        : _dlgNum(start_dlgnum) {}

    // Tells if the dialog is either processing or ended on the start entry
    // TODO: possibly a hack, investigate if it's possible to do without this
    bool IsFirstEntry() const { return _isFirstEntry; }
    int  GetDlgNum() const { return _dlgNum; }
    int  GetExecutedOption() const { return _executedOption; }
    bool AreOptionsDisplayed() const { return _areOptionsDisplayed; }

    // FIXME: this is a hack, see also the comment below
    ScriptPosition &GetSavedDialogRequestScPos() { return _savedDialogRequestScriptPos; }

    // Runs Dialog state
    void Run();
    // Request the Dialog state to stop.
    // Note that the stopping is scheduled and is performed as soon as
    // dialog state receives control.
    void Stop();

private:
    int HandleDialogResult(int res);

    int _dlgNum = -1;
    int _dlgWas = -1;
    // CHECKME: this may be unnecessary, investigate later
    bool _isFirstEntry = true;
    // Dialog topics history, used by "goto-previous" command
    std::stack<int> _topicHist;
    int _executedOption = -1; // option which is currently run (or -1)
    bool _areOptionsDisplayed = false; // if dialog options are displayed on screen
    bool _doStop = false;

    // A position in script saved by certain API function calls in "dialog_request" callback;
    // used purely for error reporting when the script has 2+ calls to gamestate-changing
    // functions such as StartDialog or ChangeRoom.
    // FIXME: this is horrible, review this and make consistent with error reporting
    // for regular calls in normal script.
    ScriptPosition _savedDialogRequestScriptPos;
};

int DialogExec::HandleDialogResult(int res)
{
    // Stop the dialog if requested
    if (_doStop)
        return RUN_DIALOG_STOP_DIALOG;

    // Handle goto-previous, see if there's any previous dialog in history
    if (res == RUN_DIALOG_GOTO_PREVIOUS)
    {
        if (_topicHist.size() == 0)
            return RUN_DIALOG_STOP_DIALOG;
        res = _topicHist.top();
        _topicHist.pop();
    }
    // Continue to the next dialog
    if (res >= 0)
    {
        // save the old topic number in the history, and switch to the new one
        _topicHist.push(_dlgNum);
        _dlgNum = res;
        return _dlgNum;
    }
    return res;
}

void DialogExec::Run()
{
    _doStop = false;

    while (_dlgNum >= 0)
    {
        if (_dlgNum < 0 || _dlgNum >= game.numdialog)
            quitprintf("!RunDialog: invalid dialog number specified: %d", _dlgNum);

        // current dialog object
        DialogTopic *dtop = &dialog[_dlgNum];
        int res = 0; // dialog execution result
        // If a new dialog topic: run dialog entry point
        if (_dlgNum != _dlgWas)
        {
            _executedOption = 0;
            res = run_dialog_entry(_dlgNum);
            _dlgWas = _dlgNum;
            _executedOption = -1;

            // Handle the dialog entry's result
            res = HandleDialogResult(res);
            if (res == RUN_DIALOG_STOP_DIALOG)
                return; // stop the dialog
            _isFirstEntry = false;
            if (res != RUN_DIALOG_STAY)
                continue; // skip to the next dialog
        }

        // Show current dialog's options
        _areOptionsDisplayed = true;
        int chose = show_dialog_options(_dlgNum, (game.options[OPT_RUNGAMEDLGOPTS] != 0));
        _areOptionsDisplayed = false;

        // Stop the dialog if requested from script
        if (_doStop)
            return;

        if (chose == CHOSE_TEXTPARSER)
        {
            said_speech_line = 0;
            res = run_dialog_request(_dlgNum);
            if (said_speech_line > 0)
            {
                // fix the problem with the close-up face remaining on screen
                // FIXME: find a better solution rather than disable/enable iface?
                // this causes all gui texture reupdate twice, and other things...
                DisableInterface();
                UpdateGameOnce(); // redraw the screen to make sure it looks right
                EnableInterface();
                set_mouse_cursor(CURS_ARROW);
            }
        }
        else if (chose >= 0)
        {
            _executedOption = chose + 1; // option id is 1-based in script, and 0 is entry point
            // chose some option - handle it and run its script
            res = run_dialog_option(_dlgNum, chose, SAYCHOSEN_USEFLAG, true /* run script */);
            _executedOption = -1;
        }
        else
        {
            return; // no option chosen? - stop the dialog
        }

        // Handle the dialog option's result
        res = HandleDialogResult(res);
        if (res == RUN_DIALOG_STOP_DIALOG)
            return; // stop the dialog
        // continue to the next dialog or show same dialog's options again
    }
}

void DialogExec::Stop()
{
    _doStop = true;
}

void do_conversation(int dlgnum)
{
    assert(dialogExec == nullptr);
    if (dialogExec)
    {
        Debug::Printf(kDbgMsg_Error, "ERROR: tried to start a new dialog state while a dialog state is running.");
        return;
    }

    EndSkippingUntilCharStops();

    // AGS 2.x always makes the mouse cursor visible when displaying a dialog.
    if (loaded_game_file_version <= kGameVersion_272)
        play.mouse_cursor_hidden = 0;

    // Run the global DialogStart event
    run_on_event(kScriptEvent_DialogStart, dlgnum);

    dialogExec.reset(new DialogExec(dlgnum));
    dialogExec->Run();
    // CHECKME: find out if this is safe to do always, regardless of number of iterations
    if (dialogExec->IsFirstEntry())
    {
        // bail out from first startup script
        remove_screen_overlay(OVER_COMPLETE);
        play.in_conversation--;
    }

    // Run the global DialogStop event; NOTE: _dlgNum may be different in the end
    run_on_event(kScriptEvent_DialogStop, dialogExec->GetDlgNum());
    dialogExec = {};

    set_default_cursor();
}

bool is_in_dialog()
{
    return dialogExec != nullptr;
}

bool is_in_dialogoptions()
{
    return dialogOpts != nullptr;
}

bool is_dialog_executing_script()
{
    return dialogExec && dialogScriptsInst && dialogExec->GetExecutedOption() >= 0;
}

// TODO: this is ugly, but I could not come to a better solution at the time...
void set_dialog_result_goto(int dlgnum)
{
    assert(is_dialog_executing_script());
    if (is_dialog_executing_script())
        dialogScriptsInst->SetReturnValue(dlgnum);
}

void set_dialog_result_stop()
{
    assert(is_dialog_executing_script());
    if (is_dialog_executing_script())
        dialogScriptsInst->SetReturnValue(RUN_DIALOG_STOP_DIALOG);
}

bool handle_state_change_in_dialog_request(const char *apiname, int dlgreq_retval)
{
    // Test if we are inside a dialog state AND dialog_request callback
    if ((dialogExec == nullptr) || (play.stop_dialog_at_end == DIALOG_NONE))
    {
        return false; // not handled, process command as normal
    }

    // Test if dialog result was not set yet
    if (play.stop_dialog_at_end == DIALOG_RUNNING)
    {
        play.stop_dialog_at_end = dlgreq_retval;
        get_script_position(dialogExec->GetSavedDialogRequestScPos());
    }
    else
    {
        debug_script_warn("!%s: more than one NewRoom/RunDialog/StopDialog requests within a dialog '%s' (%d), following one(s) will be ignored\n\tfirst was made in \"%s\", line %d",
            apiname, game.dialogScriptNames[dialogExec->GetDlgNum()].GetCStr(), dialogExec->GetDlgNum(),
            dialogExec->GetSavedDialogRequestScPos().Section.GetCStr(), dialogExec->GetSavedDialogRequestScPos().Line);
    }
    return true; // handled, state change will be taken care of by a dialog script
}

void schedule_dialog_stop()
{
    // NOTE: dialog options may be displayed with Dialog.DisplayOptions() too
    assert(dialogExec || dialogOpts);
    if (dialogExec)
        dialogExec->Stop();
    if (dialogOpts)
        dialogOpts->Stop();
}

void shutdown_dialog_state()
{
    dialogExec = {};
    dialogOpts = {};
}

// end dialog manager


//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "ac/dynobj/cc_dialog.h"
#include "ac/dynobj/scriptstring.h"

extern CCDialog     ccDynamicDialog;

ScriptDialog *Dialog_GetByName(const char *name)
{
    return static_cast<ScriptDialog*>(ccGetScriptObjectAddress(name, ccDynamicDialog.GetType()));
}

void Dialog_Stop()
{
    StopDialog();
}

ScriptDialog *Dialog_GetCurrentDialog()
{
    return dialogExec ? &scrDialog[dialogExec->GetDlgNum()] : nullptr;
}

int Dialog_GetExecutedOption()
{
    return dialogExec ? dialogExec->GetExecutedOption() : -1;
}

bool Dialog_GetAreOptionsDisplayed()
{
    return is_in_dialogoptions();
}

RuntimeScriptValue Sc_Dialog_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptDialog, ccDynamicDialog, Dialog_GetByName, const char);
}

RuntimeScriptValue Sc_Dialog_Stop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Dialog_Stop);
}

RuntimeScriptValue Sc_Dialog_GetCurrentDialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(ScriptDialog, ccDynamicDialog, Dialog_GetCurrentDialog);
}

RuntimeScriptValue Sc_Dialog_GetExecutedOption(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetExecutedOption);
}

RuntimeScriptValue Sc_Dialog_GetAreOptionsDisplayed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Dialog_GetAreOptionsDisplayed);
}

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetID);
}

RuntimeScriptValue Sc_Dialog_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptDialog, const char, myScriptStringImpl, Dialog_GetScriptName);
}

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetOptionCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetOptionCount);
}

RuntimeScriptValue Sc_Dialog_GetOptionsGUI(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(ScriptGUI, ccDynamicGUI, Dialog_GetOptionsGUI);
}

RuntimeScriptValue Sc_Dialog_SetOptionsGUI(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Dialog_SetOptionsGUI, ScriptGUI);
}

RuntimeScriptValue Sc_Dialog_GetOptionsGUIX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsGUIX);
}

RuntimeScriptValue Sc_Dialog_SetOptionsGUIX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsGUIX);
}

RuntimeScriptValue Sc_Dialog_GetOptionsGUIY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsGUIY);
}

RuntimeScriptValue Sc_Dialog_SetOptionsGUIY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsGUIY);
}

RuntimeScriptValue Sc_Dialog_GetOptionsPaddingX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsPaddingX);
}

RuntimeScriptValue Sc_Dialog_SetOptionsPaddingX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsPaddingX);
}

RuntimeScriptValue Sc_Dialog_GetOptionsPaddingY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsPaddingY);
}

RuntimeScriptValue Sc_Dialog_SetOptionsPaddingY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsPaddingY);
}

RuntimeScriptValue Sc_Dialog_GetOptionsBulletGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsBulletGraphic);
}

RuntimeScriptValue Sc_Dialog_SetOptionsBulletGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsBulletGraphic);
}

RuntimeScriptValue Sc_Dialog_GetOptionsFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsFont);
}

RuntimeScriptValue Sc_Dialog_SetOptionsFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsFont);
}

RuntimeScriptValue Sc_Dialog_GetOptionsNumbering(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsNumbering);
}

RuntimeScriptValue Sc_Dialog_SetOptionsNumbering(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsNumbering);
}

RuntimeScriptValue Sc_Dialog_GetOptionsHighlightColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsHighlightColor);
}

RuntimeScriptValue Sc_Dialog_SetOptionsHighlightColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsHighlightColor);
}

RuntimeScriptValue Sc_Dialog_GetOptionsReadColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsReadColor);
}

RuntimeScriptValue Sc_Dialog_SetOptionsReadColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsReadColor);
}

RuntimeScriptValue Sc_Dialog_GetOptionsTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsTextAlignment);
}

RuntimeScriptValue Sc_Dialog_SetOptionsTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsTextAlignment);
}

RuntimeScriptValue Sc_Dialog_GetOptionsGap(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetOptionsGap);
}

RuntimeScriptValue Sc_Dialog_SetOptionsGap(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetOptionsGap);
}

RuntimeScriptValue Sc_Dialog_GetMaxOptionsGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetMaxOptionsGUIWidth);
}

RuntimeScriptValue Sc_Dialog_SetMaxOptionsGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetMaxOptionsGUIWidth);
}

RuntimeScriptValue Sc_Dialog_GetMinOptionsGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetMinOptionsGUIWidth);
}

RuntimeScriptValue Sc_Dialog_SetMinOptionsGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Dialog_SetMinOptionsGUIWidth);
}

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetShowTextParser(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetShowTextParser);
}

// int (ScriptDialog *sd, int sayChosenOption)
RuntimeScriptValue Sc_Dialog_DisplayOptions(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptDialog, Dialog_DisplayOptions);
}

// int (ScriptDialog *sd, int option)
RuntimeScriptValue Sc_Dialog_GetOptionState(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptDialog, Dialog_GetOptionState);
}

// const char* (ScriptDialog *sd, int option)
RuntimeScriptValue Sc_Dialog_GetOptionText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptDialog, const char, myScriptStringImpl, Dialog_GetOptionText);
}

// int (ScriptDialog *sd, int option)
RuntimeScriptValue Sc_Dialog_HasOptionBeenChosen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptDialog, Dialog_HasOptionBeenChosen);
}

RuntimeScriptValue Sc_Dialog_SetHasOptionBeenChosen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_PBOOL(ScriptDialog, Dialog_SetHasOptionBeenChosen);
}

// void (ScriptDialog *sd, int option, int newState)
RuntimeScriptValue Sc_Dialog_SetOptionState(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDialog, Dialog_SetOptionState);
}

// void (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_Start(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDialog, Dialog_Start);
}

void RegisterDialogAPI()
{
    ScFnRegister dialog_api[] = {
        { "Dialog::GetByName",            API_FN_PAIR(Dialog_GetByName) },
        { "Dialog::Stop",                 API_FN_PAIR(Dialog_Stop) },
        { "Dialog::get_CurrentDialog",    API_FN_PAIR(Dialog_GetCurrentDialog) },
        { "Dialog::get_ExecutedOption",   API_FN_PAIR(Dialog_GetExecutedOption) },
        { "Dialog::get_AreOptionsDisplayed", API_FN_PAIR(Dialog_GetAreOptionsDisplayed) },
        { "Dialog::get_ID",               API_FN_PAIR(Dialog_GetID) },
        { "Dialog::get_OptionCount",      API_FN_PAIR(Dialog_GetOptionCount) },
        { "Dialog::get_OptionsBulletGraphic", API_FN_PAIR(Dialog_GetOptionsBulletGraphic) },
        { "Dialog::set_OptionsBulletGraphic", API_FN_PAIR(Dialog_SetOptionsBulletGraphic) },
        { "Dialog::get_OptionsFont",      API_FN_PAIR(Dialog_GetOptionsFont) },
        { "Dialog::set_OptionsFont",      API_FN_PAIR(Dialog_SetOptionsFont) },
        { "Dialog::get_OptionsGap",       API_FN_PAIR(Dialog_GetOptionsGap) },
        { "Dialog::set_OptionsGap",       API_FN_PAIR(Dialog_SetOptionsGap) },
        { "Dialog::get_OptionsGUI",       API_FN_PAIR(Dialog_GetOptionsGUI) },
        { "Dialog::set_OptionsGUI",       API_FN_PAIR(Dialog_SetOptionsGUI) },
        { "Dialog::get_OptionsGUIX",      API_FN_PAIR(Dialog_GetOptionsGUIX) },
        { "Dialog::set_OptionsGUIX",      API_FN_PAIR(Dialog_SetOptionsGUIX) },
        { "Dialog::get_OptionsGUIY",      API_FN_PAIR(Dialog_GetOptionsGUIY) },
        { "Dialog::set_OptionsGUIY",      API_FN_PAIR(Dialog_SetOptionsGUIY) },
        { "Dialog::get_OptionsHighlightColor", API_FN_PAIR(Dialog_GetOptionsHighlightColor) },
        { "Dialog::set_OptionsHighlightColor", API_FN_PAIR(Dialog_SetOptionsHighlightColor) },
        { "Dialog::get_OptionsMaxGUIWidth", API_FN_PAIR(Dialog_GetMaxOptionsGUIWidth) },
        { "Dialog::set_OptionsMaxGUIWidth", API_FN_PAIR(Dialog_SetMaxOptionsGUIWidth) },
        { "Dialog::get_OptionsMinGUIWidth", API_FN_PAIR(Dialog_GetMinOptionsGUIWidth) },
        { "Dialog::set_OptionsMinGUIWidth", API_FN_PAIR(Dialog_SetMinOptionsGUIWidth) },
        { "Dialog::get_OptionsNumbering", API_FN_PAIR(Dialog_GetOptionsNumbering) },
        { "Dialog::set_OptionsNumbering", API_FN_PAIR(Dialog_SetOptionsNumbering) },
        { "Dialog::get_OptionsPaddingX",  API_FN_PAIR(Dialog_GetOptionsPaddingX) },
        { "Dialog::set_OptionsPaddingX",  API_FN_PAIR(Dialog_SetOptionsPaddingX) },
        { "Dialog::get_OptionsPaddingY",  API_FN_PAIR(Dialog_GetOptionsPaddingY) },
        { "Dialog::set_OptionsPaddingY",  API_FN_PAIR(Dialog_SetOptionsPaddingY) },
        { "Dialog::get_OptionsReadColor",  API_FN_PAIR(Dialog_GetOptionsReadColor) },
        { "Dialog::set_OptionsReadColor",  API_FN_PAIR(Dialog_SetOptionsReadColor) },
        { "Dialog::get_OptionsTextAlignment", API_FN_PAIR(Dialog_GetOptionsTextAlignment) },
        { "Dialog::set_OptionsTextAlignment", API_FN_PAIR(Dialog_SetOptionsTextAlignment) },
        { "Dialog::get_ScriptName",       API_FN_PAIR(Dialog_GetScriptName) },
        { "Dialog::get_ShowTextParser",   API_FN_PAIR(Dialog_GetShowTextParser) },
        { "Dialog::DisplayOptions^1",     API_FN_PAIR(Dialog_DisplayOptions) },
        { "Dialog::GetOptionState^1",     API_FN_PAIR(Dialog_GetOptionState) },
        { "Dialog::GetOptionText^1",      API_FN_PAIR(Dialog_GetOptionText) },
        { "Dialog::HasOptionBeenChosen^1", API_FN_PAIR(Dialog_HasOptionBeenChosen) },
        { "Dialog::SetHasOptionBeenChosen^2", API_FN_PAIR(Dialog_SetHasOptionBeenChosen) },
        { "Dialog::SetOptionState^2",     API_FN_PAIR(Dialog_SetOptionState) },
        { "Dialog::Start^0",              API_FN_PAIR(Dialog_Start) },
    };

    ccAddExternalFunctions(dialog_api);
}
