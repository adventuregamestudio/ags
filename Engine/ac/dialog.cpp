//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

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
#include "ac/overlay.h"
#include "ac/mouse.h"
#include "ac/parser.h"
#include "ac/record.h"
#include "ac/string.h"
#include "ac/dynobj/scriptdialogoptionsrendering.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "font/fonts.h"
#include "script/cc_instance.h"
#include "gui/guimain.h"
#include "gui/guitextbox.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "script/script.h"
#include "ac/spritecache.h"
#include "gfx/ddb.h"
#include "gfx/gfx_util.h"
#include "gfx/graphicsdriver.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern GameSetupStruct game;
extern GameState play;
extern ccInstance *dialogScriptsInst;
extern int in_new_room;
extern int scrnwid,scrnhit;
extern CharacterInfo*playerchar;
extern SpriteCache spriteset;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern GUIMain*guis;
extern volatile int timerloop;
extern AGSPlatformDriver *platform;
extern int cur_mode,cur_cursor;
extern Bitmap *virtual_screen;
extern IGraphicsDriver *gfxDriver;

DialogTopic *dialog;
ScriptDialogOptionsRendering ccDialogOptionsRendering;
ScriptDrawingSurface* dialogOptionsRenderingSurface;

int said_speech_line; // used while in dialog to track whether screen needs updating

// Old dialog support
unsigned char** old_dialog_scripts;
char** old_speech_lines;

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

int run_dialog_script(DialogTopic*dtpp, int dialogID, int offse, int optionIndex) {
  said_speech_line = 0;
  int result = RUN_DIALOG_STAY;

  if (dialogScriptsInst)
  {
    char funcName[100];
    sprintf(funcName, "_run_dialog%d", dialogID);
    dialogScriptsInst->RunTextScriptIParam(funcName, RuntimeScriptValue().SetInt32(optionIndex));
    result = dialogScriptsInst->returnValue;
  }
  else
  {
    // old dialog format
    if (offse == -1)
      return result;	
	
    unsigned char* script = (unsigned char*)&old_dialog_scripts[dialogID][offse];

    unsigned short param1 = 0;
    unsigned short param2 = 0;
    int new_topic = 0;
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
            Display(get_translation(old_speech_lines[param2]));
          else
            DisplaySpeech(get_translation(old_speech_lines[param2]), param1);

          said_speech_line = 1;
          break;

        case DCMD_OPTOFF:
          get_dialog_script_parameters(script, &param1, NULL);
          SetDialogOption(dialogID, param1 + 1, 0);
          break;

        case DCMD_OPTON:
          get_dialog_script_parameters(script, &param1, NULL);
          SetDialogOption(dialogID, param1 + 1, DFLG_ON);
          break;

        case DCMD_RETURN:
          script_running = false;
          break;

        case DCMD_STOPDIALOG:
          result = RUN_DIALOG_STOP_DIALOG;
          script_running = false;
          break;

        case DCMD_OPTOFFFOREVER:
          get_dialog_script_parameters(script, &param1, NULL);
          SetDialogOption(dialogID, param1 + 1, DFLG_OFFPERM);
          break;

        case DCMD_RUNTEXTSCRIPT:
          get_dialog_script_parameters(script, &param1, NULL);
          result = run_dialog_request(param1);
          script_running = (result == RUN_DIALOG_STAY);
          break;

        case DCMD_GOTODIALOG:
          get_dialog_script_parameters(script, &param1, NULL);
          result = param1;
          script_running = false;
          break;

        case DCMD_PLAYSOUND:
          get_dialog_script_parameters(script, &param1, NULL);
          play_sound(param1);
          break;

        case DCMD_ADDINV:
          get_dialog_script_parameters(script, &param1, NULL);
          add_inventory(param1);
          break;

        case DCMD_SETSPCHVIEW:
          get_dialog_script_parameters(script, &param1, &param2);
          SetCharacterSpeechView(param1, param2);
          break;

        case DCMD_NEWROOM:
          get_dialog_script_parameters(script, &param1, NULL);
          NewRoom(param1);
          in_new_room = 1;
          result = RUN_DIALOG_STOP_DIALOG;
          script_running = false;
          break;

        case DCMD_SETGLOBALINT:
          get_dialog_script_parameters(script, &param1, &param2);
          SetGlobalInt(param1, param2);
          break;

        case DCMD_GIVESCORE:
          get_dialog_script_parameters(script, &param1, NULL);
          GiveScore(param1);
          break;

        case DCMD_GOTOPREVIOUS:
          result = RUN_DIALOG_GOTO_PREVIOUS;
          script_running = false;
          break;

        case DCMD_LOSEINV:
          get_dialog_script_parameters(script, &param1, NULL);
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
    mainloop(); // redraw the screen to make sure it looks right
    EnableInterface();
    // if we're not about to abort the dialog, switch back to arrow
    if (result != RUN_DIALOG_STOP_DIALOG)
      set_mouse_cursor(CURS_ARROW);
  }

  return result;
}

int write_dialog_options(Bitmap *ds, int dlgxp, int curyp, int numdisp, int mouseison, int areawid,
    int bullet_wid, int usingfont, DialogTopic*dtop, char*disporder, short*dispyp,
    int txthit, int utextcol) {
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

    break_up_text_into_lines(areawid-(8+bullet_wid),usingfont,get_translation(dtop->optionnames[disporder[ww]]));
    dispyp[ww]=curyp;
    if (game.dialog_bullet > 0)
      wputblock(ds, dlgxp,curyp,spriteset[game.dialog_bullet],1);
    int cc;
    if (game.options[OPT_DIALOGNUMBERED]) {
      char tempbfr[20];
      int actualpicwid = 0;
      if (game.dialog_bullet > 0)
        actualpicwid = spritewidth[game.dialog_bullet]+3;

      sprintf (tempbfr, "%d.", ww + 1);
      wouttext_outline (ds, dlgxp + actualpicwid, curyp, usingfont, text_color, tempbfr);
    }
    for (cc=0;cc<numlines;cc++) {
      wouttext_outline(ds, dlgxp+((cc==0) ? 0 : 9)+bullet_wid, curyp, usingfont, text_color, lines[cc]);
      curyp+=txthit;
    }
    if (ww < numdisp-1)
      curyp += multiply_up_coordinate(game.options[OPT_DIALOGGAP]);
  }
  return curyp;
}



#define GET_OPTIONS_HEIGHT {\
  needheight = 0;\
  for (ww=0;ww<numdisp;ww++) {\
    break_up_text_into_lines(areawid-(8+bullet_wid),usingfont,get_translation(dtop->optionnames[disporder[ww]]));\
    needheight += (numlines * txthit) + multiply_up_coordinate(game.options[OPT_DIALOGGAP]);\
  }\
  if (parserInput) needheight += parserInput->hit + multiply_up_coordinate(game.options[OPT_DIALOGGAP]);\
 }


void draw_gui_for_dialog_options(Bitmap *ds, GUIMain *guib, int dlgxp, int dlgyp) {
  if (guib->bgcol != 0) {
    color_t draw_color = ds->GetCompatibleColor(guib->bgcol);
    ds->FillRect(Rect(dlgxp, dlgyp, dlgxp + guib->wid, dlgyp + guib->hit), draw_color);
  }
  if (guib->bgpic > 0)
      AGS::Engine::GfxUtil::DrawSpriteWithTransparency(ds, spriteset[guib->bgpic], dlgxp, dlgyp);
}

bool get_custom_dialog_options_dimensions(int dlgnum)
{
  ccDialogOptionsRendering.Reset();
  ccDialogOptionsRendering.dialogID = dlgnum;

  getDialogOptionsDimensionsFunc.params[0].SetDynamicObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
  run_function_on_non_blocking_thread(&getDialogOptionsDimensionsFunc);

  if ((ccDialogOptionsRendering.width > 0) &&
      (ccDialogOptionsRendering.height > 0))
  {
    return true;
  }
  return false;
}

#define MAX_TOPIC_HISTORY 50
#define DLG_OPTION_PARSER 99

int show_dialog_options(int dlgnum, int sayChosenOption, bool runGameLoopsInBackground) 
{
  int dlgxp,dlgyp = get_fixed_pixel_size(160);
  int usingfont=FONT_NORMAL;
  int txthit = wgetfontheight(usingfont);
  int curswas=cur_cursor;
  int bullet_wid = 0, needheight;
  IDriverDependantBitmap *ddb = NULL;
  Bitmap *subBitmap = NULL;
  GUITextBox *parserInput = NULL;
  DialogTopic*dtop = NULL;

  if ((dlgnum < 0) || (dlgnum >= game.numdialog))
    quit("!RunDialog: invalid dialog number specified");

  can_run_delayed_command();

  play.in_conversation ++;

  update_polled_stuff_if_runtime();

  if (game.dialog_bullet > 0)
    bullet_wid = spritewidth[game.dialog_bullet]+3;

  // numbered options, leave space for the numbers
  if (game.options[OPT_DIALOGNUMBERED])
    bullet_wid += wgettextwidth_compensate("9. ", usingfont);

  said_text = 0;

  update_polled_stuff_if_runtime();

  Bitmap *tempScrn = BitmapHelper::CreateBitmap(BitmapHelper::GetScreenBitmap()->GetWidth(), BitmapHelper::GetScreenBitmap()->GetHeight(), final_col_dep);

  set_mouse_cursor(CURS_ARROW);

  dtop=&dialog[dlgnum];

  int ww,chose=-1,numdisp=0;

  //get_real_screen();
  Bitmap *ds = SetVirtualScreen(virtual_screen);

  char disporder[MAXTOPICOPTIONS];
  short dispyp[MAXTOPICOPTIONS];
  int parserActivated = 0;
  if ((dtop->topicFlags & DTFLG_SHOWPARSER) && (play.disable_dialog_parser == 0)) {
    parserInput = new GUITextBox();
    parserInput->hit = txthit + get_fixed_pixel_size(4);
    parserInput->exflags = 0;
    parserInput->font = usingfont;
  }

  numdisp=0;
  for (ww=0;ww<dtop->numoptions;ww++) {
    if ((dtop->optionflags[ww] & DFLG_ON)==0) continue;
    ensure_text_valid_for_font(dtop->optionnames[ww], usingfont);
    disporder[numdisp]=ww;
    numdisp++;
  }
  if (numdisp<1) quit("!DoDialog: all options have been turned off");
  // Don't display the options if there is only one and the parser
  // is not enabled.
  color_t draw_color;
  if ((numdisp > 1) || (parserInput != NULL) || (play.show_single_dialog_option)) {
    draw_color = ds->GetCompatibleColor(0); //ds->FillRect(Rect(0,dlgyp-1,scrnwid-1,dlgyp+numdisp*txthit+1);
    int areawid, is_textwindow = 0;
    int forecol = 14, savedwid;

    int mouseison=-1,curyp;
    int mousewason=-10;
    int dirtyx = 0, dirtyy = 0;
    int dirtywidth = virtual_screen->GetWidth(), dirtyheight = virtual_screen->GetHeight();
    bool usingCustomRendering = false;

    dlgxp = 1;
    if (get_custom_dialog_options_dimensions(dlgnum))
    {
      usingCustomRendering = true;
      dirtyx = multiply_up_coordinate(ccDialogOptionsRendering.x);
      dirtyy = multiply_up_coordinate(ccDialogOptionsRendering.y);
      dirtywidth = multiply_up_coordinate(ccDialogOptionsRendering.width);
      dirtyheight = multiply_up_coordinate(ccDialogOptionsRendering.height);
    }
    else if (game.options[OPT_DIALOGIFACE] > 0)
    {
      GUIMain*guib=&guis[game.options[OPT_DIALOGIFACE]];
      if (guib->is_textwindow()) {
        // text-window, so do the QFG4-style speech options
        is_textwindow = 1;
        forecol = guib->fgcol;
      }
      else {
        dlgxp = guib->x;
        dlgyp = guib->y;
        draw_gui_for_dialog_options(ds, guib, dlgxp, dlgyp);

        dirtyx = dlgxp;
        dirtyy = dlgyp;
        dirtywidth = guib->wid;
        dirtyheight = guib->hit;

        areawid=guib->wid - 5;

        GET_OPTIONS_HEIGHT

        if (game.options[OPT_DIALOGUPWARDS]) {
          // They want the options upwards from the bottom
          dlgyp = (guib->y + guib->hit) - needheight;
        }
        
      }
    }
    else {
      //dlgyp=(scrnhit-numdisp*txthit)-1;
      areawid=scrnwid-5;
      GET_OPTIONS_HEIGHT
      dlgyp = scrnhit - needheight;
      ds->FillRect(Rect(0,dlgyp-1,scrnwid-1,scrnhit-1), draw_color);

      dirtyx = 0;
      dirtyy = dlgyp - 1;
      dirtywidth = scrnwid;
      dirtyheight = scrnhit - dirtyy;
    }
    if (!is_textwindow)
      areawid -= multiply_up_coordinate(play.dialog_options_x) * 2;

    int orixp = dlgxp, oriyp = dlgyp;
    int wantRefresh = 0;
    mouseison=-10;
    
    update_polled_stuff_if_runtime();
    //->Blit(virtual_screen, tempScrn, 0, 0, 0, 0, screen->GetWidth(), screen->GetHeight());
    if (!play.mouse_cursor_hidden)
      domouse(1);
    update_polled_stuff_if_runtime();

 redraw_options:

    wantRefresh = 1;

    if (usingCustomRendering)
    {
      tempScrn = recycle_bitmap(tempScrn, final_col_dep, 
        multiply_up_coordinate(ccDialogOptionsRendering.width), 
        multiply_up_coordinate(ccDialogOptionsRendering.height));
    }

    tempScrn->ClearTransparent();
    if (!usingCustomRendering)
    {
      ds = SetVirtualScreen(tempScrn);
    }

    dlgxp = orixp;
    dlgyp = oriyp;
    // lengthy drawing to screen, so lock it for speed
    //acquire_screen();

    if (usingCustomRendering)
    {
      ccDialogOptionsRendering.surfaceToRenderTo = dialogOptionsRenderingSurface;
      ccDialogOptionsRendering.surfaceAccessed = false;
      dialogOptionsRenderingSurface->linkedBitmapOnly = tempScrn;
      dialogOptionsRenderingSurface->hasAlphaChannel = false;

      renderDialogOptionsFunc.params[0].SetDynamicObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
      run_function_on_non_blocking_thread(&renderDialogOptionsFunc);

      if (!ccDialogOptionsRendering.surfaceAccessed)
        quit("!dialog_options_get_dimensions was implemented, but no dialog_options_render function drew anything to the surface");

      if (parserInput)
      {
        parserInput->x = multiply_up_coordinate(ccDialogOptionsRendering.parserTextboxX);
        curyp = multiply_up_coordinate(ccDialogOptionsRendering.parserTextboxY);
        areawid = multiply_up_coordinate(ccDialogOptionsRendering.parserTextboxWidth);
        if (areawid == 0)
          areawid = tempScrn->GetWidth();
      }
    }
    else if (is_textwindow) {
      // text window behind the options
      areawid = multiply_up_coordinate(play.max_dialogoption_width);
      int biggest = 0;
      for (ww=0;ww<numdisp;ww++) {
        break_up_text_into_lines(areawid-(8+bullet_wid),usingfont,get_translation(dtop->optionnames[disporder[ww]]));
        if (longestline > biggest)
          biggest = longestline;
      }
      if (biggest < areawid - (12+bullet_wid))
        areawid = biggest + (12+bullet_wid);

      if (areawid < multiply_up_coordinate(play.min_dialogoption_width)) {
        areawid = multiply_up_coordinate(play.min_dialogoption_width);
        if (play.min_dialogoption_width > play.max_dialogoption_width)
          quit("!game.min_dialogoption_width is larger than game.max_dialogoption_width");
      }

      GET_OPTIONS_HEIGHT

      savedwid = areawid;
      int txoffs=0,tyoffs=0,yspos = scrnhit/2-needheight/2;
      int xspos = scrnwid/2 - areawid/2;
      // shift window to the right if QG4-style full-screen pic
      if ((game.options[OPT_SPEECHTYPE] == 3) && (said_text > 0))
        xspos = (scrnwid - areawid) - get_fixed_pixel_size(10);

      // needs to draw the right text window, not the default
      push_screen(ds);
      Bitmap *text_window_ds = ds;
      draw_text_window(&text_window_ds, false, &txoffs,&tyoffs,&xspos,&yspos,&areawid,NULL,needheight, game.options[OPT_DIALOGIFACE]);
      ds = pop_screen();
      // snice draw_text_window incrases the width, restore it
      areawid = savedwid;
      //wnormscreen();

      dirtyx = xspos;
      dirtyy = yspos;
      dirtywidth = text_window_ds->GetWidth();
      dirtyheight = text_window_ds->GetHeight();

      wputblock(ds, xspos,yspos,text_window_ds,1);
      delete text_window_ds;

      // Ignore the dialog_options_x/y offsets when using a text window
      txoffs += xspos;
      tyoffs += yspos;
      dlgyp = tyoffs;
      curyp = write_dialog_options(ds, txoffs,tyoffs,numdisp,mouseison,areawid,bullet_wid,usingfont,dtop,disporder,dispyp,txthit,forecol);
      if (parserInput)
        parserInput->x = txoffs;
    }
    else {

      if (wantRefresh) {
        // redraw the black background so that anti-alias
        // fonts don't re-alias themselves
        if (game.options[OPT_DIALOGIFACE] == 0) {
          draw_color = ds->GetCompatibleColor(16);
          ds->FillRect(Rect(0,dlgyp-1,scrnwid-1,scrnhit-1), draw_color);
        }
        else {
          GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
          if (!guib->is_textwindow())
            draw_gui_for_dialog_options(ds, guib, dlgxp, dlgyp);
        }
      }

      dirtyx = 0;
      dirtywidth = scrnwid;

      if (game.options[OPT_DIALOGIFACE] > 0) 
      {
        // the whole GUI area should be marked dirty in order
        // to ensure it gets drawn
        GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
        dirtyheight = guib->hit;
        dirtyy = dlgyp;
      }
      else
      {
        dirtyy = dlgyp - 1;
        dirtyheight = needheight + 1;
      }

      dlgxp += multiply_up_coordinate(play.dialog_options_x);
      dlgyp += multiply_up_coordinate(play.dialog_options_y);

      // if they use a negative dialog_options_y, make sure the
      // area gets marked as dirty
      if (dlgyp < dirtyy)
        dirtyy = dlgyp;

      //curyp = dlgyp + 1;
      curyp = dlgyp;
      curyp = write_dialog_options(ds, dlgxp,curyp,numdisp,mouseison,areawid,bullet_wid,usingfont,dtop,disporder,dispyp,txthit,forecol);

      /*if (curyp > scrnhit) {
        dlgyp = scrnhit - (curyp - dlgyp);
        ds->FillRect(Rect(0,dlgyp-1,scrnwid-1,scrnhit-1);
        goto redraw_options;
      }*/
      if (parserInput)
        parserInput->x = dlgxp;
    }

    if (parserInput) {
      // Set up the text box, if present
      parserInput->y = curyp + multiply_up_coordinate(game.options[OPT_DIALOGGAP]);
      parserInput->wid = areawid - get_fixed_pixel_size(10);
      parserInput->textcol = playerchar->talkcolor;
      if (mouseison == DLG_OPTION_PARSER)
        parserInput->textcol = forecol;

      if (game.dialog_bullet)  // the parser X will get moved in a second
        wputblock(ds, parserInput->x, parserInput->y, spriteset[game.dialog_bullet], 1);

      parserInput->wid -= bullet_wid;
      parserInput->x += bullet_wid;

      parserInput->Draw(ds);
      parserInput->activated = 0;
    }

    wantRefresh = 0;
    ds = SetVirtualScreen(virtual_screen);

    update_polled_stuff_if_runtime();

    subBitmap = recycle_bitmap(subBitmap, tempScrn->GetColorDepth(), dirtywidth, dirtyheight);
    subBitmap = gfxDriver->ConvertBitmapToSupportedColourDepth(subBitmap);

    update_polled_stuff_if_runtime();

    if (usingCustomRendering)
    {
      subBitmap->Blit(tempScrn, 0, 0, 0, 0, tempScrn->GetWidth(), tempScrn->GetHeight());
      invalidate_rect(dirtyx, dirtyy, dirtyx + subBitmap->GetWidth(), dirtyy + subBitmap->GetHeight());
    }
    else
    {
      subBitmap->Blit(tempScrn, dirtyx, dirtyy, 0, 0, dirtywidth, dirtyheight);
    }

    if ((ddb != NULL) && 
      ((ddb->GetWidth() != dirtywidth) ||
       (ddb->GetHeight() != dirtyheight)))
    {
      gfxDriver->DestroyDDB(ddb);
      ddb = NULL;
    }
    if (ddb == NULL)
      ddb = gfxDriver->CreateDDBFromBitmap(subBitmap, false, false);
    else
      gfxDriver->UpdateDDBFromBitmap(ddb, subBitmap, false);

    render_graphics(ddb, dirtyx, dirtyy);

    while (1) {

      if (runGameLoopsInBackground)
      {
        play.disabled_user_interface++;
        mainloop(false, ddb, dirtyx, dirtyy);
        play.disabled_user_interface--;
      }
      else
      {
        timerloop = 0;
        NEXT_ITERATION();

        render_graphics(ddb, dirtyx, dirtyy);
      
        update_polled_stuff_and_crossfade();
      }

      if (kbhit()) {
        int gkey = getch();
        if (parserInput) {
          wantRefresh = 1;
          // type into the parser 
          if ((gkey == 361) || ((gkey == ' ') && (strlen(parserInput->text) == 0))) {
            // write previous contents into textbox (F3 or Space when box is empty)
            for (unsigned int i = strlen(parserInput->text); i < strlen(play.lastParserEntry); i++) {
              parserInput->KeyPress(play.lastParserEntry[i]);
            }
            //domouse(2);
            goto redraw_options;
          }
          else if ((gkey >= 32) || (gkey == 13) || (gkey == 8)) {
            parserInput->KeyPress(gkey);
            if (!parserInput->activated) {
              //domouse(2);
              goto redraw_options;
            }
          }
        }
        // Allow selection of options by keyboard shortcuts
        else if ((gkey >= '1') && (gkey <= '9')) {
          gkey -= '1';
          if (gkey < numdisp) {
            chose = disporder[gkey];
            break;
          }
        }
      }
      mousewason=mouseison;
      mouseison=-1;
      if (usingCustomRendering)
      {
        if ((mousex >= dirtyx) && (mousey >= dirtyy) &&
            (mousex < dirtyx + tempScrn->GetWidth()) &&
            (mousey < dirtyy + tempScrn->GetHeight()))
        {
          getDialogOptionUnderCursorFunc.params[0].SetDynamicObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
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
      else if ((mousey <= dlgyp) || (mousey > curyp)) ;
      else {
        mouseison=numdisp-1;
        for (ww=0;ww<numdisp;ww++) {
          if (mousey < dispyp[ww]) { mouseison=ww-1; break; }
        }
        if ((mouseison<0) | (mouseison>=numdisp)) mouseison=-1;
      }

      if (parserInput != NULL) {
        int relativeMousey = mousey;
        if (usingCustomRendering)
          relativeMousey -= dirtyy;

        if ((relativeMousey > parserInput->y) && 
            (relativeMousey < parserInput->y + parserInput->hit))
          mouseison = DLG_OPTION_PARSER;

        if (parserInput->activated)
          parserActivated = 1;
      }

      int mouseButtonPressed = mgetbutton();

      if (mouseButtonPressed != NONE) {
        if (mouseison < 0) 
        {
          if (usingCustomRendering)
          {
            runDialogOptionMouseClickHandlerFunc.params[0].SetDynamicObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionMouseClickHandlerFunc.params[1].SetInt32(mouseButtonPressed + 1);
            run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);

            if (runDialogOptionMouseClickHandlerFunc.atLeastOneImplementationExists)
              goto redraw_options;
          }
          continue;
        }
        if (mouseison == DLG_OPTION_PARSER) {
          // they clicked the text box
          parserActivated = 1;
        }
        else if (usingCustomRendering)
        {
          chose = mouseison;
          break;
        }
        else {
          chose=disporder[mouseison];
          break;
        }
      }

      if (usingCustomRendering)
      {
        int mouseWheelTurn = check_mouse_wheel();
        if (mouseWheelTurn != 0)
        {
            runDialogOptionMouseClickHandlerFunc.params[0].SetDynamicObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
            runDialogOptionMouseClickHandlerFunc.params[1].SetInt32((mouseWheelTurn < 0) ? 9 : 8);
            run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);

            if (runDialogOptionMouseClickHandlerFunc.atLeastOneImplementationExists)
              goto redraw_options;

            continue;
        }
      }

      if (parserActivated) {
        // They have selected a custom parser-based option
        if (parserInput->text[0] != 0) {
          chose = DLG_OPTION_PARSER;
          break;
        }
        else {
          parserActivated = 0;
          parserInput->activated = 0;
        }
      }
      if (mousewason != mouseison) {
        //domouse(2);
        goto redraw_options;
      }
      while ((timerloop == 0) && (play.fast_forward == 0)) {
        update_polled_stuff_if_runtime();
        platform->YieldCPU();
      }

    }
    if (!play.mouse_cursor_hidden)
      domouse(2);
  }
  else 
    chose = disporder[0];  // only one choice, so select it

  while (kbhit()) getch(); // empty keyboard buffer
  //leave_real_screen();
  construct_virtual_screen(true);

  if (parserActivated) 
  {
    strcpy (play.lastParserEntry, parserInput->text);
    ParseText (parserInput->text);
    chose = CHOSE_TEXTPARSER;
  }

  if (parserInput) {
    delete parserInput;
    parserInput = NULL;
  }

  if (ddb != NULL)
    gfxDriver->DestroyDDB(ddb);
  delete subBitmap;

  set_mouse_cursor(curswas);
  // In case it's the QFG4 style dialog, remove the black screen
  play.in_conversation--;
  remove_screen_overlay(OVER_COMPLETE);

  delete tempScrn;

  if (chose != CHOSE_TEXTPARSER)
  {
    dtop->optionflags[chose] |= DFLG_HASBEENCHOSEN;

    bool sayTheOption = false;
    if (sayChosenOption == SAYCHOSEN_YES)
    {
      sayTheOption = true;
    }
    else if (sayChosenOption == SAYCHOSEN_USEFLAG)
    {
      sayTheOption = ((dtop->optionflags[chose] & DFLG_NOREPEAT) == 0);
    }

    if (sayTheOption)
      DisplaySpeech(get_translation(dtop->optionnames[chose]), game.playercharacter);
  }

  return chose;
}

void do_conversation(int dlgnum) 
{
  EndSkippingUntilCharStops();

  // AGS 2.x always makes the mouse cursor visible when displaying a dialog.
  if (loaded_game_file_version <= kGameVersion_272)
    play.mouse_cursor_hidden = 0;

  int dlgnum_was = dlgnum;
  int previousTopics[MAX_TOPIC_HISTORY];
  int numPrevTopics = 0;
  DialogTopic *dtop = &dialog[dlgnum];

  // run the startup script
  int tocar = run_dialog_script(dtop, dlgnum, dtop->startupentrypoint, 0);
  if ((tocar == RUN_DIALOG_STOP_DIALOG) ||
      (tocar == RUN_DIALOG_GOTO_PREVIOUS)) 
  {
    // 'stop' or 'goto-previous' from first startup script
    remove_screen_overlay(OVER_COMPLETE);
    play.in_conversation--;
    return;
  }
  else if (tocar >= 0)
    dlgnum = tocar;

  while (dlgnum >= 0)
  {
    if (dlgnum >= game.numdialog)
      quit("!RunDialog: invalid dialog number specified");

    dtop = &dialog[dlgnum];

    if (dlgnum != dlgnum_was) 
    {
      // dialog topic changed, so play the startup
      // script for the new topic
      tocar = run_dialog_script(dtop, dlgnum, dtop->startupentrypoint, 0);
      dlgnum_was = dlgnum;
      if (tocar == RUN_DIALOG_GOTO_PREVIOUS) {
        if (numPrevTopics < 1) {
          // goto-previous on first topic -- end dialog
          tocar = RUN_DIALOG_STOP_DIALOG;
        }
        else {
          tocar = previousTopics[numPrevTopics - 1];
          numPrevTopics--;
        }
      }
      if (tocar == RUN_DIALOG_STOP_DIALOG)
        break;
      else if (tocar >= 0) {
        // save the old topic number in the history
        if (numPrevTopics < MAX_TOPIC_HISTORY) {
          previousTopics[numPrevTopics] = dlgnum;
          numPrevTopics++;
        }
        dlgnum = tocar;
        continue;
      }
    }

    int chose = show_dialog_options(dlgnum, SAYCHOSEN_USEFLAG, (game.options[OPT_RUNGAMEDLGOPTS] != 0));

    if (chose == CHOSE_TEXTPARSER)
    {
      said_speech_line = 0;
  
      tocar = run_dialog_request(dlgnum);

      if (said_speech_line > 0) {
        // fix the problem with the close-up face remaining on screen
        DisableInterface();
        mainloop(); // redraw the screen to make sure it looks right
        EnableInterface();
        set_mouse_cursor(CURS_ARROW);
      }
    }
    else 
    {
      tocar = run_dialog_script(dtop, dlgnum, dtop->entrypoints[chose], chose + 1);
    }

    if (tocar == RUN_DIALOG_GOTO_PREVIOUS) {
      if (numPrevTopics < 1) {
        tocar = RUN_DIALOG_STOP_DIALOG;
      }
      else {
        tocar = previousTopics[numPrevTopics - 1];
        numPrevTopics--;
      }
    }
    if (tocar == RUN_DIALOG_STOP_DIALOG) break;
    else if (tocar >= 0) {
      // save the old topic number in the history
      if (numPrevTopics < MAX_TOPIC_HISTORY) {
        previousTopics[numPrevTopics] = dlgnum;
        numPrevTopics++;
      }
      dlgnum = tocar;
    }

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
#include "ac/dynobj/scriptstring.h"

extern ScriptString myScriptStringImpl;

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetID);
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
    ccAddExternalObjectFunction("Dialog::get_ID",               Sc_Dialog_GetID);
    ccAddExternalObjectFunction("Dialog::get_OptionCount",      Sc_Dialog_GetOptionCount);
    ccAddExternalObjectFunction("Dialog::get_ShowTextParser",   Sc_Dialog_GetShowTextParser);
    ccAddExternalObjectFunction("Dialog::DisplayOptions^1",     Sc_Dialog_DisplayOptions);
    ccAddExternalObjectFunction("Dialog::GetOptionState^1",     Sc_Dialog_GetOptionState);
    ccAddExternalObjectFunction("Dialog::GetOptionText^1",      Sc_Dialog_GetOptionText);
    ccAddExternalObjectFunction("Dialog::HasOptionBeenChosen^1", Sc_Dialog_HasOptionBeenChosen);
    ccAddExternalObjectFunction("Dialog::SetOptionState^2",     Sc_Dialog_SetOptionState);
    ccAddExternalObjectFunction("Dialog::Start^0",              Sc_Dialog_Start);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Dialog::get_ID",               (void*)Dialog_GetID);
    ccAddExternalFunctionForPlugin("Dialog::get_OptionCount",      (void*)Dialog_GetOptionCount);
    ccAddExternalFunctionForPlugin("Dialog::get_ShowTextParser",   (void*)Dialog_GetShowTextParser);
    ccAddExternalFunctionForPlugin("Dialog::DisplayOptions^1",     (void*)Dialog_DisplayOptions);
    ccAddExternalFunctionForPlugin("Dialog::GetOptionState^1",     (void*)Dialog_GetOptionState);
    ccAddExternalFunctionForPlugin("Dialog::GetOptionText^1",      (void*)Dialog_GetOptionText);
    ccAddExternalFunctionForPlugin("Dialog::HasOptionBeenChosen^1", (void*)Dialog_HasOptionBeenChosen);
    ccAddExternalFunctionForPlugin("Dialog::SetOptionState^2",     (void*)Dialog_SetOptionState);
    ccAddExternalFunctionForPlugin("Dialog::Start^0",              (void*)Dialog_Start);
}
