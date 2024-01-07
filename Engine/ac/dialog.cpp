//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
#include "ac/gamestate.h"
#include "ac/gamesetupstruct.h"
#include "ac/global_character.h"
#include "ac/global_dialog.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_room.h"
#include "ac/global_translation.h"
#include "ac/keycode.h"
#include "ac/overlay.h"
#include "ac/mouse.h"
#include "ac/parser.h"
#include "ac/sys_events.h"
#include "ac/string.h"
#include "ac/dynobj/scriptdialogoptionsrendering.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/system.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "script/cc_instance.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "main/game_run.h"
#include "platform/base/agsplatformdriver.h"
#include "script/script.h"
#include "ac/spritecache.h"
#include "gfx/ddb.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"
#include "ac/mouse.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameState play;
extern int in_new_room;
extern CharacterInfo*playerchar;
extern SpriteCache spriteset;
extern AGSPlatformDriver *platform;
extern int cur_mode,cur_cursor;
extern IGraphicsDriver *gfxDriver;

std::vector<DialogTopic> dialog;
ScriptDialogOptionsRendering ccDialogOptionsRendering;
ScriptDrawingSurface* dialogOptionsRenderingSurface;

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

  int chose = show_dialog_options(sd->id, sayChosenOption, (game.options[OPT_RUNGAMEDLGOPTS] != 0));
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
  option--;

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
    option--;
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

int Dialog_GetShowTextParser(ScriptDialog *sd)
{
  return (dialog[sd->id].topicFlags & DTFLG_SHOWPARSER) ? 1 : 0;
}

const char* Dialog_GetOptionText(ScriptDialog *sd, int option)
{
  if ((option < 1) || (option > dialog[sd->id].numoptions))
    quit("!Dialog.GetOptionText: Invalid option number specified");

  option--;

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

#define RUN_DIALOG_STAY          -1
#define RUN_DIALOG_STOP_DIALOG   -2
#define RUN_DIALOG_GOTO_PREVIOUS -4
// dialog manager stuff

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

int run_dialog_script(int dialogID, int offse, int optionIndex) {
  said_speech_line = 0;
  int result = RUN_DIALOG_STAY;

  if (dialogScriptsInst)
  {
    char func_name[100];
    snprintf(func_name, sizeof(func_name), "_run_dialog%d", dialogID);
    RuntimeScriptValue params[]{ optionIndex };
    RunScriptFunction(dialogScriptsInst.get(), func_name, 1, params);
    result = dialogScriptsInst->returnValue;
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
          if (in_new_room <= 0)
              in_new_room = 1; // set only in case NewRoom was scheduled
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
    DisableInterface();
    UpdateGameOnce(); // redraw the screen to make sure it looks right
    EnableInterface();
    // if we're not about to abort the dialog, switch back to arrow
    if (result != RUN_DIALOG_STOP_DIALOG)
      set_mouse_cursor(CURS_ARROW);
  }

  return result;
}

int write_dialog_options(Bitmap *ds, bool ds_has_alpha, int dlgxp, int curyp, int numdisp, int mouseison, int areawid,
    int bullet_wid, int usingfont, DialogTopic*dtop, int*disporder, short*dispyp,
    int linespacing, int utextcol, int padding) {
  int ww;

  color_t text_color;
  for (ww=0;ww<numdisp;ww++) {

    if ((dtop->optionflags[disporder[ww]] & DFLG_HASBEENCHOSEN) &&
        (play.read_dialog_option_colour >= 0)) {
      // 'read' colour
      text_color = ds->GetCompatibleColor(play.read_dialog_option_colour);
    }
    else {
      // 'unread' colour
      text_color = ds->GetCompatibleColor(playerchar->talkcolor);
    }

    if (mouseison==ww) {
      if (text_color == ds->GetCompatibleColor(utextcol))
        text_color = ds->GetCompatibleColor(13); // the normal colour is the same as highlight col
      else text_color = ds->GetCompatibleColor(utextcol);
    }

    const char *draw_text = skip_voiceover_token(get_translation(dtop->optionnames[disporder[ww]]));
    break_up_text_into_lines(draw_text, Lines, areawid-(2*padding+2+bullet_wid), usingfont);
    dispyp[ww]=curyp;
    if (game.dialog_bullet > 0)
    {
        draw_gui_sprite_v330(ds, game.dialog_bullet, dlgxp, curyp, ds_has_alpha);
    }
    if (game.options[OPT_DIALOGNUMBERED] == kDlgOptNumbering) {
      char tempbfr[20];
      int actualpicwid = 0;
      if (game.dialog_bullet > 0)
        actualpicwid = game.SpriteInfos[game.dialog_bullet].Width+3;

      snprintf(tempbfr, sizeof(tempbfr), "%d.", ww + 1);
      wouttext_outline (ds, dlgxp + actualpicwid, curyp, usingfont, text_color, tempbfr);
    }
    for (size_t cc=0;cc<Lines.Count();cc++) {
      wouttext_outline(ds, dlgxp+((cc==0) ? 0 : 9)+bullet_wid, curyp, usingfont, text_color, Lines[cc].GetCStr());
      curyp+=linespacing;
    }
    if (ww < numdisp-1)
      curyp += data_to_game_coord(game.options[OPT_DIALOGGAP]);
  }
  return curyp;
}

void draw_gui_for_dialog_options(Bitmap *ds, GUIMain *guib, int dlgxp, int dlgyp) {
  if (guib->BgColor != 0) {
    color_t draw_color = ds->GetCompatibleColor(guib->BgColor);
    ds->FillRect(Rect(dlgxp, dlgyp, dlgxp + guib->Width, dlgyp + guib->Height), draw_color);
  }
  if (guib->BgImage > 0)
      GfxUtil::DrawSpriteWithTransparency(ds, spriteset[guib->BgImage], dlgxp, dlgyp);
}

bool get_custom_dialog_options_dimensions(int dlgnum)
{
  ccDialogOptionsRendering.Reset();
  ccDialogOptionsRendering.dialogID = dlgnum;

  getDialogOptionsDimensionsFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
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
struct DialogOptions
{
    int dlgnum;
    bool runGameLoopsInBackground;

    int dlgxp;
    int dlgyp;
    int dialog_abs_x; // absolute dialog position on screen
    int padding;
    int usingfont;
    int lineheight;
    int linespacing;
    int curswas;
    int bullet_wid;
    int needheight;
    IDriverDependantBitmap *ddb;
    Bitmap *subBitmap;
    GUITextBox *parserInput;
    DialogTopic*dtop;

    // display order of options
    int disporder[MAXTOPICOPTIONS];
    // display Y coordinate of options
    short dispyp[MAXTOPICOPTIONS];
    // number of displayed options
    int numdisp;
    // last chosen option
    int chose;

    Bitmap *tempScrn;
    int parserActivated;

    int curyp;
    bool needRedraw;
    bool wantRefresh; // FIXME: merge with needRedraw? or better names
    bool usingCustomRendering;
    bool newCustomRender; // using newer (post-3.5.0 render API)
    int orixp;
    int oriyp;
    int areawid;
    int is_textwindow;
    int dirtyx;
    int dirtyy;
    int dirtywidth;
    int dirtyheight;

    int mouseison;

    int forecol;

    void Prepare(int _dlgnum, bool _runGameLoopsInBackground);
    void Show();
    void Redraw();
    // Runs the dialog options update;
    // returns whether should continue to run options loop, or stop
    bool Run();
    // Process all the buffered input events; returns if handled
    bool RunControls();
    // Process single key event; returns if handled
    bool RunKey(const KeyInput &ki);
    // Process single mouse event; returns if handled
    bool RunMouse(eAGSMouseButton mbut);
    // Process mouse wheel scroll
    bool RunMouseWheel(int mwheelz);
    void Close();

private:
    void CalcOptionsHeight();
};

void DialogOptions::CalcOptionsHeight()
{
    needheight = 0;
    for (int i = 0; i < numdisp; ++i)
    {
        const char *draw_text = skip_voiceover_token(get_translation(dtop->optionnames[disporder[i]]));
        break_up_text_into_lines(draw_text, Lines, areawid-(2*padding+2+bullet_wid), usingfont);
        needheight += get_text_lines_surf_height(usingfont, Lines.Count()) + data_to_game_coord(game.options[OPT_DIALOGGAP]);
    }
    if (parserInput)
    {
        needheight += parserInput->GetHeight() + data_to_game_coord(game.options[OPT_DIALOGGAP]);
    }
}

void DialogOptions::Prepare(int _dlgnum, bool _runGameLoopsInBackground)
{
  dlgnum = _dlgnum;
  runGameLoopsInBackground = _runGameLoopsInBackground;

  dlgyp = get_fixed_pixel_size(160);
  usingfont=FONT_NORMAL;
  lineheight = get_font_height_outlined(usingfont);
  linespacing = get_font_linespacing(usingfont);
  curswas=cur_cursor;
  bullet_wid = 0;
  ddb = nullptr;
  subBitmap = nullptr;
  parserInput = nullptr;
  dtop = nullptr;

  if ((dlgnum < 0) || (dlgnum >= game.numdialog))
    quit("!RunDialog: invalid dialog number specified");

  can_run_delayed_command();

  play.in_conversation ++;

  if (game.dialog_bullet > 0)
    bullet_wid = game.SpriteInfos[game.dialog_bullet].Width+3;

  // numbered options, leave space for the numbers
  if (game.options[OPT_DIALOGNUMBERED] == kDlgOptNumbering)
    bullet_wid += get_text_width_outlined("9. ", usingfont);

  said_text = 0;

  const Rect &ui_view = play.GetUIViewport();
  tempScrn = BitmapHelper::CreateBitmap(ui_view.GetWidth(), ui_view.GetHeight(), game.GetColorDepth());

  set_mouse_cursor(CURS_ARROW);

  dtop=&dialog[dlgnum];

  chose=-1;
  numdisp=0;

  parserActivated = 0;
  if ((dtop->topicFlags & DTFLG_SHOWPARSER) && (play.disable_dialog_parser == 0)) {
    parserInput = new GUITextBox();
    parserInput->SetHeight(lineheight + get_fixed_pixel_size(4));
    parserInput->SetShowBorder(true);
    parserInput->Font = usingfont;
  }

  numdisp=0;
  for (int i = 0; i < dtop->numoptions; ++i) {
    if ((dtop->optionflags[i] & DFLG_ON)==0) continue;
    ensure_text_valid_for_font(dtop->optionnames[i], usingfont);
    disporder[numdisp]=i;
    numdisp++;
  }
}

void DialogOptions::Show()
{
  if (numdisp < 1)
  {
      debug_script_warn("Dialog: all options have been turned off, stopping dialog.");
      return;
  }
  // Don't display the options if there is only one and the parser
  // is not enabled.
  if (!((numdisp > 1) || (parserInput != nullptr) || (play.show_single_dialog_option)))
  {
      chose = disporder[0];  // only one choice, so select it
      return;
  }

    is_textwindow = 0;
    forecol = play.dialog_options_highlight_color;

    mouseison = -1;
    const Rect &ui_view = play.GetUIViewport();
    dirtyx = 0;
    dirtyy = 0;
    dirtywidth = ui_view.GetWidth();
    dirtyheight = ui_view.GetHeight();
    usingCustomRendering = false;


    dlgxp = 1;
    if (get_custom_dialog_options_dimensions(dlgnum))
    {
      usingCustomRendering = true;
      dirtyx = data_to_game_coord(ccDialogOptionsRendering.x);
      dirtyy = data_to_game_coord(ccDialogOptionsRendering.y);
      dirtywidth = data_to_game_coord(ccDialogOptionsRendering.width);
      dirtyheight = data_to_game_coord(ccDialogOptionsRendering.height);
      dialog_abs_x = dirtyx;
    }
    else if (game.options[OPT_DIALOGIFACE] > 0)
    {
      GUIMain*guib=&guis[game.options[OPT_DIALOGIFACE]];
      if (guib->IsTextWindow()) {
        // text-window, so do the QFG4-style speech options
        is_textwindow = 1;
        forecol = guib->FgColor;
      }
      else {
        dlgxp = guib->X;
        dlgyp = guib->Y;

        dirtyx = dlgxp;
        dirtyy = dlgyp;
        dirtywidth = guib->Width;
        dirtyheight = guib->Height;
        dialog_abs_x = guib->X;

        areawid=guib->Width - 5;
        padding = TEXTWINDOW_PADDING_DEFAULT;

        CalcOptionsHeight();

        if (game.options[OPT_DIALOGUPWARDS]) {
          // They want the options upwards from the bottom
          dlgyp = (guib->Y + guib->Height) - needheight;
        }
        
      }
    }
    else {
      //dlgyp=(play.viewport.GetHeight()-numdisp*txthit)-1;
      areawid= ui_view.GetWidth()-5;
      padding = TEXTWINDOW_PADDING_DEFAULT;
      CalcOptionsHeight();
      dlgyp = ui_view.GetHeight() - needheight;

      dirtyx = 0;
      dirtyy = dlgyp - 1;
      dirtywidth = ui_view.GetWidth();
      dirtyheight = ui_view.GetHeight() - dirtyy;
      dialog_abs_x = 0;
    }
    if (!is_textwindow)
      areawid -= data_to_game_coord(play.dialog_options_x) * 2;

    newCustomRender = usingCustomRendering && game.options[OPT_DIALOGOPTIONSAPI] >= 0;
    orixp = dlgxp;
    oriyp = dlgyp;
    needRedraw = false;
    wantRefresh = false;
    mouseison=-10;

    Redraw();
    while(Run());

    // Close custom dialog options
    if (usingCustomRendering)
    {
        runDialogOptionCloseFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&runDialogOptionCloseFunc);
    }
}

void DialogOptions::Redraw()
{
    wantRefresh = true;

    if (usingCustomRendering)
    {
      tempScrn = recycle_bitmap(tempScrn, game.GetColorDepth(), 
        data_to_game_coord(ccDialogOptionsRendering.width), 
        data_to_game_coord(ccDialogOptionsRendering.height));
    }

    tempScrn->ClearTransparent();
    Bitmap *ds = tempScrn;

    dlgxp = orixp;
    dlgyp = oriyp;
    const Rect &ui_view = play.GetUIViewport();

    bool options_surface_has_alpha = false;

    if (usingCustomRendering)
    {
      ccDialogOptionsRendering.surfaceToRenderTo = dialogOptionsRenderingSurface;
      ccDialogOptionsRendering.surfaceAccessed = false;
      dialogOptionsRenderingSurface->linkedBitmapOnly = tempScrn;
      dialogOptionsRenderingSurface->hasAlphaChannel = ccDialogOptionsRendering.hasAlphaChannel;
      options_surface_has_alpha = dialogOptionsRenderingSurface->hasAlphaChannel != 0;

      renderDialogOptionsFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
      run_function_on_non_blocking_thread(&renderDialogOptionsFunc);

      if (!ccDialogOptionsRendering.surfaceAccessed)
          debug_script_warn("dialog_options_get_dimensions was implemented, but no dialog_options_render function drew anything to the surface");

      if (parserInput)
      {
        parserInput->X = data_to_game_coord(ccDialogOptionsRendering.parserTextboxX);
        curyp = data_to_game_coord(ccDialogOptionsRendering.parserTextboxY);
        areawid = data_to_game_coord(ccDialogOptionsRendering.parserTextboxWidth);
        if (areawid == 0)
          areawid = tempScrn->GetWidth();
      }
      ccDialogOptionsRendering.needRepaint = false;
    }
    else if (is_textwindow) {
      // text window behind the options
      areawid = data_to_game_coord(play.max_dialogoption_width);
      int biggest = 0;
      padding = guis[game.options[OPT_DIALOGIFACE]].Padding;
      for (int i = 0; i < numdisp; ++i) {
        const char *draw_text = skip_voiceover_token(get_translation(dtop->optionnames[disporder[i]]));
        break_up_text_into_lines(draw_text, Lines, areawid-((2*padding+2)+bullet_wid), usingfont);
        if (longestline > biggest)
          biggest = longestline;
      }
      if (biggest < areawid - ((2*padding+6)+bullet_wid))
        areawid = biggest + ((2*padding+6)+bullet_wid);

      if (areawid < data_to_game_coord(play.min_dialogoption_width)) {
        areawid = data_to_game_coord(play.min_dialogoption_width);
        if (play.min_dialogoption_width > play.max_dialogoption_width)
          quit("!game.min_dialogoption_width is larger than game.max_dialogoption_width");
      }

      CalcOptionsHeight();

      int savedwid = areawid;
      int txoffs=0,tyoffs=0,yspos = ui_view.GetHeight()/2-(2*padding+needheight)/2;
      int xspos = ui_view.GetWidth()/2 - areawid/2;
      // shift window to the right if QG4-style full-screen pic
      if ((game.options[OPT_SPEECHTYPE] == 3) && (said_text > 0))
        xspos = (ui_view.GetWidth() - areawid) - get_fixed_pixel_size(10);

      // needs to draw the right text window, not the default
      Bitmap *text_window_ds = nullptr;
      draw_text_window(&text_window_ds, false, &txoffs,&tyoffs,&xspos,&yspos,&areawid,nullptr,needheight, game.options[OPT_DIALOGIFACE]);
      options_surface_has_alpha = guis[game.options[OPT_DIALOGIFACE]].HasAlphaChannel();
      // since draw_text_window incrases the width, restore it
      areawid = savedwid;

      dirtyx = xspos;
      dirtyy = yspos;
      dirtywidth = text_window_ds->GetWidth();
      dirtyheight = text_window_ds->GetHeight();
      dialog_abs_x = txoffs + xspos;

      GfxUtil::DrawSpriteWithTransparency(ds, text_window_ds, xspos, yspos);
      // TODO: here we rely on draw_text_window always assigning new bitmap to text_window_ds;
      // should make this more explicit
      delete text_window_ds;

      // Ignore the dialog_options_x/y offsets when using a text window
      txoffs += xspos;
      tyoffs += yspos;
      dlgyp = tyoffs;
      curyp = write_dialog_options(ds, options_surface_has_alpha, txoffs,tyoffs,numdisp,mouseison,areawid,bullet_wid,usingfont,dtop,disporder,dispyp,linespacing,forecol,padding);
      if (parserInput)
        parserInput->X = txoffs;
    }
    else {

      if (wantRefresh) {
        // redraw the black background so that anti-alias
        // fonts don't re-alias themselves
        if (game.options[OPT_DIALOGIFACE] == 0) {
          color_t draw_color = ds->GetCompatibleColor(16);
          ds->FillRect(Rect(0,dlgyp-1, ui_view.GetWidth()-1, ui_view.GetHeight()-1), draw_color);
        }
        else {
          GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
          if (!guib->IsTextWindow())
            draw_gui_for_dialog_options(ds, guib, dlgxp, dlgyp);
        }
      }

      dirtyx = 0;
      dirtywidth = ui_view.GetWidth();

      if (game.options[OPT_DIALOGIFACE] > 0) 
      {
        // the whole GUI area should be marked dirty in order
        // to ensure it gets drawn
        GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
        dirtyheight = guib->Height;
        dirtyy = dlgyp;
        options_surface_has_alpha = guib->HasAlphaChannel();
      }
      else
      {
        dirtyy = dlgyp - 1;
        dirtyheight = needheight + 1;
        options_surface_has_alpha = false;
      }

      dlgxp += data_to_game_coord(play.dialog_options_x);
      dlgyp += data_to_game_coord(play.dialog_options_y);

      // if they use a negative dialog_options_y, make sure the
      // area gets marked as dirty
      if (dlgyp < dirtyy)
        dirtyy = dlgyp;

      curyp = dlgyp;
      curyp = write_dialog_options(ds, options_surface_has_alpha, dlgxp,curyp,numdisp,mouseison,areawid,bullet_wid,usingfont,dtop,disporder,dispyp,linespacing,forecol,padding);

      if (parserInput)
        parserInput->X = dlgxp;
    }

    if (parserInput) {
      // Set up the text box, if present
      parserInput->Y = curyp + data_to_game_coord(game.options[OPT_DIALOGGAP]);
      parserInput->SetWidth(areawid - get_fixed_pixel_size(10));
      parserInput->TextColor = playerchar->talkcolor;
      if (mouseison == DLG_OPTION_PARSER)
        parserInput->TextColor = forecol;

      if (game.dialog_bullet)  // the parser X will get moved in a second
      {
          draw_gui_sprite_v330(ds, game.dialog_bullet, parserInput->X, parserInput->Y, options_surface_has_alpha);
      }

      parserInput->SetWidth(parserInput->GetWidth() - bullet_wid);
      parserInput->X += bullet_wid;

      parserInput->Draw(ds, parserInput->X, parserInput->Y);
      parserInput->IsActivated = false;
    }

    wantRefresh = false;

    subBitmap = recycle_bitmap(subBitmap,
        gfxDriver->GetCompatibleBitmapFormat(tempScrn->GetColorDepth()), dirtywidth, dirtyheight);

    if (usingCustomRendering)
    {
      subBitmap->Blit(tempScrn, 0, 0, 0, 0, tempScrn->GetWidth(), tempScrn->GetHeight());
      invalidate_rect(dirtyx, dirtyy, dirtyx + subBitmap->GetWidth(), dirtyy + subBitmap->GetHeight(), false);
    }
    else
    {
      subBitmap->Blit(tempScrn, dirtyx, dirtyy, 0, 0, dirtywidth, dirtyheight);
    }

    if ((ddb != nullptr) && 
      ((ddb->GetWidth() != dirtywidth) ||
       (ddb->GetHeight() != dirtyheight)))
    {
      gfxDriver->DestroyDDB(ddb);
      ddb = nullptr;
    }
    
    if (ddb == nullptr)
      ddb = gfxDriver->CreateDDBFromBitmap(subBitmap, options_surface_has_alpha, false);
    else
      gfxDriver->UpdateDDBFromBitmap(ddb, subBitmap, options_surface_has_alpha);

    if (runGameLoopsInBackground)
    {
        render_graphics(ddb, dirtyx, dirtyy);
    }
}

bool DialogOptions::Run()
{
    // Run() can be called in a loop, so keep events going.
    sys_evt_process_pending();

    // Optionally run full game update, otherwise only minimal auto & overlay update
    if (runGameLoopsInBackground)
    {
        play.disabled_user_interface++;
        UpdateGameOnce(false, ddb, dirtyx, dirtyy);
        play.disabled_user_interface--;
    }
    else
    {
        update_audio_system_on_game_loop();
        UpdateCursorAndDrawables();
        render_graphics(ddb, dirtyx, dirtyy);
    }

    needRedraw = false;

    // For >= 3.4.0 custom options rendering: run "dialog_options_repexec"
    if (newCustomRender)
    {
        runDialogOptionRepExecFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
        run_function_on_non_blocking_thread(&runDialogOptionRepExecFunc);
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
        if ((mousex >= dirtyx) && (mousey >= dirtyy) &&
            (mousex < dirtyx + tempScrn->GetWidth()) &&
            (mousey < dirtyy + tempScrn->GetHeight()))
        {
            // Run "dialog_options_get_active"
            getDialogOptionUnderCursorFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            run_function_on_non_blocking_thread(&getDialogOptionUnderCursorFunc);

            if (!getDialogOptionUnderCursorFunc.atLeastOneImplementationExists)
            quit("!The script function dialog_options_get_active is not implemented. It must be present to use a custom dialogue system.");

            mouseison = ccDialogOptionsRendering.activeOptionID;
        }
        else
        {
            ccDialogOptionsRendering.activeOptionID = -1;
        }
    }
    else if (mousex >= dialog_abs_x && mousex < (dialog_abs_x + areawid) &&
            mousey >= dlgyp && mousey < curyp)
    {
        // Default rendering: detect option under mouse
        mouseison = numdisp-1;
        for (int i = 0; i < numdisp; ++i)
        {
            if (mousey < dispyp[i]) { mouseison=i-1; break; }
        }
        if ((mouseison<0) | (mouseison>=numdisp)) mouseison=-1;
    }

    // Handle mouse over parser
    if (parserInput)
    {
        int relativeMousey = mousey;
        if (usingCustomRendering)
            relativeMousey -= dirtyy;

        if ((relativeMousey > parserInput->Y) &&
            (relativeMousey < parserInput->Y + parserInput->GetHeight()))
            mouseison = DLG_OPTION_PARSER;
    }

    // Handle player's input
    RunControls();
    ags_clear_input_buffer();

    // Post user input, processing changes
    // Handle default rendering changing an active option
    if (!usingCustomRendering)
    {
        needRedraw = (mousewason != mouseison);
    }
    // Handle new parser's state
    if (parserInput && parserInput->IsActivated)
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
        if (!parserInput->Text.IsEmpty() != 0)
        {
            chose = DLG_OPTION_PARSER;
        }
        else
        {
            parserActivated = 0;
            parserInput->IsActivated = 0;
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
        needRedraw = ccDialogOptionsRendering.needRepaint;
    }

    // Finally, if the option has been chosen, then break the options loop
    if (chose >= 0)
        return false;

    // Redraw if needed
    if (needRedraw)
        Redraw();

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
    for (InputType type = ags_inputevent_ready(); type != kInputNone; type = ags_inputevent_ready())
    {
        if (type == kInputKeyboard)
        {
            KeyInput ki;
            if (run_service_key_controls(ki) && !play.IsIgnoringInput() &&
                RunKey(ki))
            {
                return true; // handled
            }
        }
        else if (type == kInputMouse)
        {
            eAGSMouseButton mbut;
            if (run_service_mb_controls(mbut) && !play.IsIgnoringInput() &&
                RunMouse(mbut))
            {
                return true; // handled
            }
        }
    }
    // Finally handle mouse wheel
    return RunMouseWheel(ags_check_mouse_wheel());
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
        if ((agskey == eAGSKeyCodeF3) || ((agskey == eAGSKeyCodeSpace) && (parserInput->Text.GetLength() == 0)))
        {
            // write previous contents into textbox (F3 or Space when box is empty)
            size_t last_len = ustrlen(play.lastParserEntry);
            size_t cur_len = ustrlen(parserInput->Text.GetCStr());
            // [ikm] CHECKME: tbh I don't quite get the logic here (it was like this in original code);
            // but what we do is copying only the last part of the previous string
            if (cur_len < last_len)
            {
                const char *entry = play.lastParserEntry;
                // TODO: utility function for advancing N utf-8 chars
                for (size_t i = 0; i < cur_len; ++i) ugetxc(&entry);
                parserInput->Text.Append(entry);
            }
            needRedraw = true;
            return true; // handled
        }
        else if ((agskey >= eAGSKeyCodeSpace) || (agskey == eAGSKeyCodeReturn) || (agskey == eAGSKeyCodeBackspace))
        {
            parserInput->OnKeyPress(ki);
            needRedraw = true;
            return true; // handled
        }
    }
    else if (newCustomRender)
    {
        if (old_keyhandle || (ki.UChar == 0))
        { // "dialog_options_key_press"
            runDialogOptionKeyPressHandlerFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionKeyPressHandlerFunc.params[1].SetInt32(AGSKeyToScriptKey(ki.Key));
            runDialogOptionKeyPressHandlerFunc.params[2].SetInt32(ki.Mod);
            run_function_on_non_blocking_thread(&runDialogOptionKeyPressHandlerFunc);
        }
        if (!old_keyhandle && (ki.UChar > 0))
        { // "dialog_options_text_input"
            runDialogOptionTextInputHandlerFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionTextInputHandlerFunc.params[1].SetInt32(ki.UChar);
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

bool DialogOptions::RunMouse(eAGSMouseButton mbut)
{
    if (mbut > kMouseNone)
    {
        if (mouseison < 0 && !newCustomRender)
        {
            if (usingCustomRendering)
            {
                runDialogOptionMouseClickHandlerFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
                runDialogOptionMouseClickHandlerFunc.params[1].SetInt32(mbut);
                run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);
                needRedraw = runDialogOptionMouseClickHandlerFunc.atLeastOneImplementationExists;
            }
        }
        else if (mouseison == DLG_OPTION_PARSER)
        {
            // they clicked the text box
            parserActivated = 1;
        }
        else if (newCustomRender)
        {
            runDialogOptionMouseClickHandlerFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionMouseClickHandlerFunc.params[1].SetInt32(mbut);
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
        runDialogOptionMouseClickHandlerFunc.params[0].SetScriptObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
        runDialogOptionMouseClickHandlerFunc.params[1].SetInt32((mwheelz < 0) ? 9 : 8);
        run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);
        needRedraw = !newCustomRender && runDialogOptionMouseClickHandlerFunc.atLeastOneImplementationExists;
        return true; // handled
    }
    return false; // not handled
}

void DialogOptions::Close()
{
  ags_clear_input_buffer();
  invalidate_screen();

  if (parserActivated) 
  {
    assert(parserInput);
    snprintf(play.lastParserEntry, MAX_MAXSTRLEN, "%s", parserInput->Text.GetCStr());
    ParseText (parserInput->Text.GetCStr());
    chose = CHOSE_TEXTPARSER;
  }

  if (parserInput) {
    delete parserInput;
    parserInput = nullptr;
  }

  if (ddb != nullptr)
    gfxDriver->DestroyDDB(ddb);
  delete subBitmap;

  set_mouse_cursor(curswas);
  // In case it's the QFG4 style dialog, remove the black screen
  play.in_conversation--;
  remove_screen_overlay(OVER_COMPLETE);

  delete tempScrn;
}

int show_dialog_options(int _dlgnum, int sayChosenOption, bool _runGameLoopsInBackground) 
{
  DialogOptions dlgopt;
  dlgopt.Prepare(_dlgnum, _runGameLoopsInBackground);
  dlgopt.Show();
  dlgopt.Close();

  int dialog_choice = dlgopt.chose;
  if (dialog_choice >= 0) // NOTE: this condition also excludes CHOSE_TEXTPARSER
  {
    assert(dialog_choice >= 0 && dialog_choice < MAXTOPICOPTIONS);
    DialogTopic *dialog_topic = dlgopt.dtop;
    int &option_flags = dialog_topic->optionflags[dialog_choice];
    const char *option_name = dlgopt.dtop->optionnames[dialog_choice];

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

    if (sayTheOption)
      DisplaySpeech(get_translation(option_name), game.playercharacter);
  }

  return dialog_choice;
}

// Dialog execution state
struct DialogExec
{
    int DlgNum = -1;
    int DlgWas = -1;
    // CHECKME: this may be unnecessary, investigate later
    bool IsFirstEntry = true;
    // nested dialogs "stack"
    std::stack<int> TopicHist;

    DialogExec(int start_dlgnum) : DlgNum(start_dlgnum) {}
    int HandleDialogResult(int res);
    void Run();
};

int DialogExec::HandleDialogResult(int res)
{
    // Handle goto-previous, see if there's any previous dialog in history
    if (res == RUN_DIALOG_GOTO_PREVIOUS)
    {
        if (TopicHist.size() == 0)
            return RUN_DIALOG_STOP_DIALOG;
        res = TopicHist.top();
        TopicHist.pop();
    }
    // Continue to the next dialog
    if (res >= 0)
    {
        // save the old topic number in the history, and switch to the new one
        TopicHist.push(DlgNum);
        DlgNum = res;
        return DlgNum;
    }
    return res;
}

void DialogExec::Run()
{
    while (DlgNum >= 0)
    {
        if (DlgNum < 0 || DlgNum >= game.numdialog)
            quitprintf("!RunDialog: invalid dialog number specified: %d", DlgNum);

        // current dialog object
        DialogTopic *dtop = &dialog[DlgNum];
        int res = 0; // dialog execution result
        // If a new dialog topic: run dialog entry point
        if (DlgNum != DlgWas)
        {
            res = run_dialog_script(DlgNum, dtop->startupentrypoint, 0);
            DlgWas = DlgNum;

            // Handle the dialog entry's result
            res = HandleDialogResult(res);
            if (res == RUN_DIALOG_STOP_DIALOG)
                return; // stop the dialog
            IsFirstEntry = false;
            if (res != RUN_DIALOG_STAY)
                continue; // skip to the next dialog
        }

        // Show current dialog's options
        int chose = show_dialog_options(DlgNum, SAYCHOSEN_USEFLAG, (game.options[OPT_RUNGAMEDLGOPTS] != 0));
        if (chose == CHOSE_TEXTPARSER)
        {
            said_speech_line = 0;
            res = run_dialog_request(DlgNum);
            if (said_speech_line > 0)
            {
                // fix the problem with the close-up face remaining on screen
                DisableInterface();
                UpdateGameOnce(); // redraw the screen to make sure it looks right
                EnableInterface();
                set_mouse_cursor(CURS_ARROW);
            }
        }
        else if (chose >= 0)
        { // chose some option - run its script
            res = run_dialog_script(DlgNum, dtop->entrypoints[chose], chose + 1);
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

void do_conversation(int dlgnum)
{
    EndSkippingUntilCharStops();

    // AGS 2.x always makes the mouse cursor visible when displaying a dialog.
    if (loaded_game_file_version <= kGameVersion_272)
        play.mouse_cursor_hidden = 0;

    DialogExec dlgexec(dlgnum);
    dlgexec.Run();
    // CHECKME: find out if this is safe to do always, regardless of number of iterations
    if (dlgexec.IsFirstEntry)
    {
        // bail out from first startup script
        remove_screen_overlay(OVER_COMPLETE);
        play.in_conversation--;
    }
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


RuntimeScriptValue Sc_Dialog_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptDialog, ccDynamicDialog, Dialog_GetByName, const char);
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
        { "Dialog::get_ID",               API_FN_PAIR(Dialog_GetID) },
        { "Dialog::get_OptionCount",      API_FN_PAIR(Dialog_GetOptionCount) },
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
