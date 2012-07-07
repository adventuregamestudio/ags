
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_controls.h"
#include "acmain/ac_commonheaders.h"
#include "gui/guiinv.h"
#include "ac/invwindow.h"
#include "plugin/agsplugin.h"
#include "gui/guitextbox.h"
#include "ac/game.h"
#include "ac/global_debug.h"
#include "ac/gui.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/gamesetupstruct.h"
#include "ac/roomstatus.h"
#include "ac/roomobject.h"
#include "gui/guimain.h"
#include "ac/global_game.h"
#include "ac/ac_roomstruct.h"
#include "sprcache.h"


#define ALLEGRO_KEYBOARD_HANDLER
// KEYBOARD HANDLER
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
int myerrno;
#else
int errno;
#endif

#if defined(MAC_VERSION) && !defined(IOS_VERSION)
#define EXTENDED_KEY_CODE 0x3f
#else
#define EXTENDED_KEY_CODE 0
#endif

#define AGS_KEYCODE_INSERT 382
#define AGS_KEYCODE_DELETE 383
#define AGS_KEYCODE_ALT_TAB 399
#define READKEY_CODE_ALT_TAB 0x4000


extern int ifacepopped;  // currently displayed pop-up GUI (-1 if none)
extern int mouse_on_iface;   // mouse cursor is over this interface
extern int mouse_on_iface_button;
extern int mouse_pushed_iface;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
extern int mouse_ifacebut_xoffs,mouse_ifacebut_yoffs;

extern char check_dynamic_sprites_at_exit;
extern int is_complete_overlay,is_text_overlay;

extern char noWalkBehindsAtAll;

extern int displayed_room;
extern int our_eip;
extern GameSetupStruct game;
extern GUIMain*guis;
extern RoomStatus*croom;
extern RoomObject*objs;
extern roomstruct thisroom;
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;
extern int in_new_room, new_room_was;


int restrict_until=0;

int my_readkey() {
  int gott=readkey();
  int scancode = ((gott >> 8) & 0x00ff);

  if (gott == READKEY_CODE_ALT_TAB)
  {
    // Alt+Tab, it gets stuck down unless we do this
    return AGS_KEYCODE_ALT_TAB;
  }

/*  char message[200];
  sprintf(message, "Scancode: %04X", gott);
  OutputDebugString(message);*/

  /*if ((scancode >= KEY_0_PAD) && (scancode <= KEY_9_PAD)) {
    // fix numeric pad keys if numlock is off (allegro 4.2 changed this behaviour)
    if ((key_shifts & KB_NUMLOCK_FLAG) == 0)
      gott = (gott & 0xff00) | EXTENDED_KEY_CODE;
  }*/

  if ((gott & 0x00ff) == EXTENDED_KEY_CODE) {
    gott = scancode + 300;

    // convert Allegro KEY_* numbers to scan codes
    // (for backwards compatibility we can't just use the
    // KEY_* constants now, it's too late)
    if ((gott>=347) & (gott<=356)) gott+=12;
    // F11-F12
    else if ((gott==357) || (gott==358)) gott+=76;
    // insert / numpad insert
    else if ((scancode == KEY_0_PAD) || (scancode == KEY_INSERT))
      gott = AGS_KEYCODE_INSERT;
    // delete / numpad delete
    else if ((scancode == KEY_DEL_PAD) || (scancode == KEY_DEL))
      gott = AGS_KEYCODE_DELETE;
    // Home
    else if (gott == 378) gott = 371;
    // End
    else if (gott == 379) gott = 379;
    // PgUp
    else if (gott == 380) gott = 373;
    // PgDn
    else if (gott == 381) gott = 381;
    // left arrow
    else if (gott==382) gott=375;
    // right arrow
    else if (gott==383) gott=377;
    // up arrow
    else if (gott==384) gott=372;
    // down arrow
    else if (gott==385) gott=380;
    // numeric keypad
    else if (gott==338) gott=379;
    else if (gott==339) gott=380;
    else if (gott==340) gott=381;
    else if (gott==341) gott=375;
    else if (gott==342) gott=376;
    else if (gott==343) gott=377;
    else if (gott==344) gott=371;
    else if (gott==345) gott=372;
    else if (gott==346) gott=373;
  }
  else
    gott = gott & 0x00ff;

  // Alt+X, abort (but only once game is loaded)
  if ((gott == play.abort_key) && (displayed_room >= 0)) {
    check_dynamic_sprites_at_exit = 0;
    quit("!|");
  }

  //sprintf(message, "Keypress: %d", gott);
  //OutputDebugString(message);

  return gott;
}


int IsButtonDown(int which) {
    if ((which < 1) || (which > 3))
        quit("!IsButtonDown: only works with eMouseLeft, eMouseRight, eMouseMiddle");
    if (misbuttondown(which-1))
        return 1;
    return 0;
}


int IsKeyPressed (int keycode) {
#ifdef ALLEGRO_KEYBOARD_HANDLER
    if (keyboard_needs_poll())
        poll_keyboard();
    if (keycode >= 300) {
        // function keys are 12 lower in allegro 4
        if ((keycode>=359) & (keycode<=368)) keycode-=12;
        // F11-F12
        else if ((keycode==433) || (keycode==434)) keycode-=76;
        // left arrow
        else if (keycode==375) keycode=382;
        // right arrow
        else if (keycode==377) keycode=383;
        // up arrow
        else if (keycode==372) keycode=384;
        // down arrow
        else if (keycode==380) keycode=385;
        // numeric keypad
        else if (keycode==379) keycode=338;
        else if (keycode==380) keycode=339;
        else if (keycode==381) keycode=340;
        else if (keycode==375) keycode=341;
        else if (keycode==376) keycode=342;
        else if (keycode==377) keycode=343;
        else if (keycode==371) keycode=344;
        else if (keycode==372) keycode=345;
        else if (keycode==373) keycode=346;
        // insert
        else if (keycode == AGS_KEYCODE_INSERT) keycode = KEY_INSERT + 300;
        // delete
        else if (keycode == AGS_KEYCODE_DELETE) keycode = KEY_DEL + 300;

        // deal with shift/ctrl/alt
        if (keycode == 403) keycode = KEY_LSHIFT;
        else if (keycode == 404) keycode = KEY_RSHIFT;
        else if (keycode == 405) keycode = KEY_LCONTROL;
        else if (keycode == 406) keycode = KEY_RCONTROL;
        else if (keycode == 407) keycode = KEY_ALT;
        else keycode -= 300;

        if (rec_iskeypressed(keycode))
            return 1;
        // deal with numeric pad keys having different codes to arrow keys
        if ((keycode == KEY_LEFT) && (rec_iskeypressed(KEY_4_PAD) != 0))
            return 1;
        if ((keycode == KEY_RIGHT) && (rec_iskeypressed(KEY_6_PAD) != 0))
            return 1;
        if ((keycode == KEY_UP) && (rec_iskeypressed(KEY_8_PAD) != 0))
            return 1;
        if ((keycode == KEY_DOWN) && (rec_iskeypressed(KEY_2_PAD) != 0))
            return 1;
        // PgDn/PgUp are equivalent to 3 and 9 on numeric pad
        if ((keycode == KEY_9_PAD) && (rec_iskeypressed(KEY_PGUP) != 0))
            return 1;
        if ((keycode == KEY_3_PAD) && (rec_iskeypressed(KEY_PGDN) != 0))
            return 1;
        // Home/End are equivalent to 7 and 1
        if ((keycode == KEY_7_PAD) && (rec_iskeypressed(KEY_HOME) != 0))
            return 1;
        if ((keycode == KEY_1_PAD) && (rec_iskeypressed(KEY_END) != 0))
            return 1;
        // insert/delete have numpad equivalents
        if ((keycode == KEY_INSERT) && (rec_iskeypressed(KEY_0_PAD) != 0))
            return 1;
        if ((keycode == KEY_DEL) && (rec_iskeypressed(KEY_DEL_PAD) != 0))
            return 1;

        return 0;
    }
    // convert ascii to scancode
    else if ((keycode >= 'A') && (keycode <= 'Z'))
    {
        keycode = platform->ConvertKeycodeToScanCode(keycode);
    }
    else if ((keycode >= '0') && (keycode <= '9'))
        keycode -= ('0' - KEY_0);
    else if (keycode == 8)
        keycode = KEY_BACKSPACE;
    else if (keycode == 9)
        keycode = KEY_TAB;
    else if (keycode == 13) {
        // check both the main return key and the numeric pad enter
        if (rec_iskeypressed(KEY_ENTER))
            return 1;
        keycode = KEY_ENTER_PAD;
    }
    else if (keycode == ' ')
        keycode = KEY_SPACE;
    else if (keycode == 27)
        keycode = KEY_ESC;
    else if (keycode == '-') {
        // check both the main - key and the numeric pad
        if (rec_iskeypressed(KEY_MINUS))
            return 1;
        keycode = KEY_MINUS_PAD;
    }
    else if (keycode == '+') {
        // check both the main + key and the numeric pad
        if (rec_iskeypressed(KEY_EQUALS))
            return 1;
        keycode = KEY_PLUS_PAD;
    }
    else if (keycode == '/') {
        // check both the main / key and the numeric pad
        if (rec_iskeypressed(KEY_SLASH))
            return 1;
        keycode = KEY_SLASH_PAD;
    }
    else if (keycode == '=')
        keycode = KEY_EQUALS;
    else if (keycode == '[')
        keycode = KEY_OPENBRACE;
    else if (keycode == ']')
        keycode = KEY_CLOSEBRACE;
    else if (keycode == '\\')
        keycode = KEY_BACKSLASH;
    else if (keycode == ';')
        keycode = KEY_SEMICOLON;
    else if (keycode == '\'')
        keycode = KEY_QUOTE;
    else if (keycode == ',')
        keycode = KEY_COMMA;
    else if (keycode == '.')
        keycode = KEY_STOP;
    else {
        DEBUG_CONSOLE("IsKeyPressed: unsupported keycode %d", keycode);
        return 0;
    }

    if (rec_iskeypressed(keycode))
        return 1;
    return 0;
#else
    // old allegro version
    quit("allegro keyboard handler not in use??");
#endif
}



// check_controls: checks mouse & keyboard interface
void check_controls() {
  int numevents_was = numevents;
  our_eip = 1007;
  NEXT_ITERATION();

  int aa,mongu=-1;
  // If all GUIs are off, skip the loop
  if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0)) ;
  else {
    // Scan for mouse-y-pos GUIs, and pop one up if appropriate
    // Also work out the mouse-over GUI while we're at it
    int ll;
    for (ll = 0; ll < game.numgui;ll++) {
      aa = play.gui_draw_order[ll];
      if (guis[aa].is_mouse_on_gui()) mongu=aa;

      if (guis[aa].popup!=POPUP_MOUSEY) continue;
      if (is_complete_overlay>0) break;  // interfaces disabled
  //    if (play.disabled_user_interface>0) break;
      if (ifacepopped==aa) continue;
      if (guis[aa].on==-1) continue;
      // Don't allow it to be popped up while skipping cutscene
      if (play.fast_forward) continue;
      
      if (mousey < guis[aa].popupyp) {
        set_mouse_cursor(CURS_ARROW);
        guis[aa].on=1; guis_need_update = 1;
        ifacepopped=aa; PauseGame();
        break;
      }
    }
  }

  mouse_on_iface=mongu;
  if ((ifacepopped>=0) && (mousey>=guis[ifacepopped].y+guis[ifacepopped].hit))
    remove_popup_interface(ifacepopped);

  // check mouse clicks on GUIs
  static int wasbutdown=0,wasongui=0;

  if ((wasbutdown>0) && (misbuttondown(wasbutdown-1))) {
    for (aa=0;aa<guis[wasongui].numobjs;aa++) {
      if (guis[wasongui].objs[aa]->activated<1) continue;
      if (guis[wasongui].get_control_type(aa)!=GOBJ_SLIDER) continue;
      // GUI Slider repeatedly activates while being dragged
      guis[wasongui].objs[aa]->activated=0;
      setevent(EV_IFACECLICK, wasongui, aa, wasbutdown);
      break;
      }
    }
  else if ((wasbutdown>0) && (!misbuttondown(wasbutdown-1))) {
    guis[wasongui].mouse_but_up();
    int whichbut=wasbutdown;
    wasbutdown=0;

    for (aa=0;aa<guis[wasongui].numobjs;aa++) {
      if (guis[wasongui].objs[aa]->activated<1) continue;
      guis[wasongui].objs[aa]->activated=0;
      if (!IsInterfaceEnabled()) break;

      int cttype=guis[wasongui].get_control_type(aa);
      if ((cttype == GOBJ_BUTTON) || (cttype == GOBJ_SLIDER) || (cttype == GOBJ_LISTBOX)) {
        setevent(EV_IFACECLICK, wasongui, aa, whichbut);
      }
      else if (cttype == GOBJ_INVENTORY) {
        mouse_ifacebut_xoffs=mousex-(guis[wasongui].objs[aa]->x)-guis[wasongui].x;
        mouse_ifacebut_yoffs=mousey-(guis[wasongui].objs[aa]->y)-guis[wasongui].y;
        int iit=offset_over_inv((GUIInv*)guis[wasongui].objs[aa]);
        if (iit>=0) {
          evblocknum=iit;
          play.used_inv_on = iit;
          if (game.options[OPT_HANDLEINVCLICKS]) {
            // Let the script handle the click
            // LEFTINV is 5, RIGHTINV is 6
            setevent(EV_TEXTSCRIPT,TS_MCLICK, whichbut + 4);
          }
          else if (whichbut==2)  // right-click is always Look
            run_event_block_inv(iit, 0);
          else if (cur_mode == MODE_HAND)
            SetActiveInventory(iit);
          else
            RunInventoryInteraction (iit, cur_mode);
          evblocknum=-1;
        }
      }
      else quit("clicked on unknown control type");
      if (guis[wasongui].popup==POPUP_MOUSEY)
        remove_popup_interface(wasongui);
      break;
    }

    run_on_event(GE_GUI_MOUSEUP, wasongui);
  }

  aa=mgetbutton();
  if (aa>NONE) {
    if ((play.in_cutscene == 3) || (play.in_cutscene == 4))
      start_skipping_cutscene();
    if ((play.in_cutscene == 5) && (aa == RIGHT))
      start_skipping_cutscene();

    if (play.fast_forward) { }
    else if ((play.wait_counter > 0) && (play.key_skip_wait > 1))
      play.wait_counter = -1;
    else if (is_text_overlay > 0) {
      if (play.cant_skip_speech & SKIP_MOUSECLICK)
        remove_screen_overlay(OVER_TEXTMSG);
    }
    else if (!IsInterfaceEnabled()) ;  // blocking cutscene, ignore mouse
    else if (platform->RunPluginHooks(AGSE_MOUSECLICK, aa+1)) {
      // plugin took the click
      DEBUG_CONSOLE("Plugin handled mouse button %d", aa+1);
    }
    else if (mongu>=0) {
      if (wasbutdown==0) {
        DEBUG_CONSOLE("Mouse click over GUI %d", mongu);
        guis[mongu].mouse_but_down();
        // run GUI click handler if not on any control
        if ((guis[mongu].mousedownon < 0) && (guis[mongu].clickEventHandler[0] != 0))
          setevent(EV_IFACECLICK, mongu, -1, aa + 1);

        run_on_event(GE_GUI_MOUSEDOWN, mongu);
      }
      wasongui=mongu;
      wasbutdown=aa+1;
    }
    else setevent(EV_TEXTSCRIPT,TS_MCLICK,aa+1);
//    else run_text_script_iparam(gameinst,"on_mouse_click",aa+1);
  }
  aa = check_mouse_wheel();
  if (aa < 0)
    setevent (EV_TEXTSCRIPT, TS_MCLICK, 9);
  else if (aa > 0)
    setevent (EV_TEXTSCRIPT, TS_MCLICK, 8);

  // check keypresses
  if (kbhit()) {
    // in case they press the finish-recording button, make sure we know
    int was_playing = play.playback;

    int kgn = getch();
    if (kgn==0) kgn=getch()+300;
//    if (kgn==367) restart_game();
//    if (kgn==2) Display("numover: %d character movesped: %d, animspd: %d",numscreenover,playerchar->walkspeed,playerchar->animspeed);
//    if (kgn==2) CreateTextOverlay(50,60,170,FONT_SPEECH,14,"This is a test screen overlay which shouldn't disappear");
//    if (kgn==2) { Display("Crashing"); strcpy(NULL, NULL); }
//    if (kgn == 2) FaceLocation (game.playercharacter, playerchar->x + 1, playerchar->y);
    //if (kgn == 2) SetCharacterIdle (game.playercharacter, 5, 0);
    //if (kgn == 2) Display("Some for?ign text");
    //if (kgn == 2) do_conversation(5);

    if (kgn == play.replay_hotkey) {
      // start/stop recording
      if (play.recording)
        stop_recording();
      else if ((play.playback) || (was_playing))
        ;  // do nothing (we got the replay of the stop key)
      else
        replay_start_this_time = 1;
    }

    check_skip_cutscene_keypress (kgn);

    if (play.fast_forward) { }
    else if (platform->RunPluginHooks(AGSE_KEYPRESS, kgn)) {
      // plugin took the keypress
      DEBUG_CONSOLE("Keypress code %d taken by plugin", kgn);
    }
    else if ((kgn == '`') && (play.debug_mode > 0)) {
      // debug console
      display_console = !display_console;
    }
    else if ((is_text_overlay > 0) &&
             (play.cant_skip_speech & SKIP_KEYPRESS) &&
             (kgn != 434)) {
      // 434 = F12, allow through for screenshot of text
      // (though atm with one script at a time that won't work)
      // only allow a key to remove the overlay if the icon bar isn't up
      if (IsGamePaused() == 0) {
        // check if it requires a specific keypress
        if ((play.skip_speech_specific_key > 0) &&
          (kgn != play.skip_speech_specific_key)) { }
        else
          remove_screen_overlay(OVER_TEXTMSG);
      }
    }
    else if ((play.wait_counter > 0) && (play.key_skip_wait > 0)) {
      play.wait_counter = -1;
      DEBUG_CONSOLE("Keypress code %d ignored - in Wait", kgn);
    }
    else if ((kgn == 5) && (display_fps == 2)) {
      // if --fps paramter is used, Ctrl+E will max out frame rate
      SetGameSpeed(1000);
      display_fps = 2;
    }
    else if ((kgn == 4) && (play.debug_mode > 0)) {
      // ctrl+D - show info
      char infobuf[900];
      int ff;
	  // MACPORT FIX 9/6/5: added last %s
      sprintf(infobuf,"In room %d %s[Player at %d, %d (view %d, loop %d, frame %d)%s%s%s",
        displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""), playerchar->x,playerchar->y,
        playerchar->view + 1, playerchar->loop,playerchar->frame,
        (IsGamePaused() == 0) ? "" : "[Game paused.",
        (play.ground_level_areas_disabled == 0) ? "" : "[Ground areas disabled.",
        (IsInterfaceEnabled() == 0) ? "[Game in Wait state" : "");
      for (ff=0;ff<croom->numobj;ff++) {
        if (ff >= 8) break; // buffer not big enough for more than 7
        sprintf(&infobuf[strlen(infobuf)],
          "[Object %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%d slot:%d trnsp:%d clkble:%d",
          ff, objs[ff].x, objs[ff].y,
          (spriteset[objs[ff].num] != NULL) ? spritewidth[objs[ff].num] : 0,
          (spriteset[objs[ff].num] != NULL) ? spriteheight[objs[ff].num] : 0,
          objs[ff].on,
          (objs[ff].moving > 0) ? "yes" : "no", objs[ff].cycling,
          objs[ff].num, objs[ff].transparent,
          ((objs[ff].flags & OBJF_NOINTERACT) != 0) ? 0 : 1 );
      }
      Display(infobuf);
      int chd = game.playercharacter;
      char bigbuffer[STD_BUFFER_SIZE] = "CHARACTERS IN THIS ROOM:[";
      for (ff = 0; ff < game.numcharacters; ff++) {
        if (game.chars[ff].room != displayed_room) continue;
        if (strlen(bigbuffer) > 430) {
          strcat(bigbuffer, "and more...");
          Display(bigbuffer);
          strcpy(bigbuffer, "CHARACTERS IN THIS ROOM (cont'd):[");
        }
        chd = ff;
        sprintf(&bigbuffer[strlen(bigbuffer)], 
          "%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%d anim:%d follow:%d flags:%X wait:%d zoom:%d)[",
          game.chars[chd].scrname, game.chars[chd].view+1, game.chars[chd].loop, game.chars[chd].frame,
          game.chars[chd].x, game.chars[chd].y, game.chars[chd].z,
          game.chars[chd].idleview, game.chars[chd].idletime, game.chars[chd].idleleft,
          game.chars[chd].walking, game.chars[chd].animating, game.chars[chd].following,
          game.chars[chd].flags, game.chars[chd].wait, charextra[chd].zoom);
      }
      Display(bigbuffer);

    }
/*    else if (kgn == 21) {
      play.debug_mode++;
      script_debug(5,0);
      play.debug_mode--;
    }*/
    else if ((kgn == 22) && (play.wait_counter < 1) && (is_text_overlay == 0) && (restrict_until == 0)) {
      // make sure we can't interrupt a Wait()
      // and desync the music to cutscene
      play.debug_mode++;
      script_debug (1,0);
      play.debug_mode--;
    }
    else if (inside_script) {
      // Don't queue up another keypress if it can't be run instantly
      DEBUG_CONSOLE("Keypress %d ignored (game blocked)", kgn);
    }
    else {
      int keywasprocessed = 0;
      // determine if a GUI Text Box should steal the click
      // it should do if a displayable character (32-255) is
      // pressed, but exclude control characters (<32) and
      // extended keys (eg. up/down arrow; 256+)
      if ( ((kgn >= 32) && (kgn != '[') && (kgn < 256)) || (kgn == 13) || (kgn == 8) ) {
        int uu,ww;
        for (uu=0;uu<game.numgui;uu++) {
          if (guis[uu].on < 1) continue;
          for (ww=0;ww<guis[uu].numobjs;ww++) {
            // not a text box, ignore it
            if ((guis[uu].objrefptr[ww] >> 16)!=GOBJ_TEXTBOX)
              continue;
            GUITextBox*guitex=(GUITextBox*)guis[uu].objs[ww];
            // if the text box is disabled, it cannot except keypresses
            if ((guitex->IsDisabled()) || (!guitex->IsVisible()))
              continue;
            guitex->KeyPress(kgn);
            if (guitex->activated) {
              guitex->activated = 0;
              setevent(EV_IFACECLICK, uu, ww, 1);
            }
            keywasprocessed = 1;
          }
        }
      }
      if (!keywasprocessed) {
        if ((kgn>='a') & (kgn<='z')) kgn-=32;
        DEBUG_CONSOLE("Running on_key_press keycode %d", kgn);
        setevent(EV_TEXTSCRIPT,TS_KEYPRESS,kgn);
      }
    }
//    run_text_script_iparam(gameinst,"on_key_press",kgn);
  }

  if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) &&
      (in_new_room == 0) && (new_room_was == 0)) {
    // Only allow walking off edges if not in wait mode, and
    // if not in Player Enters Screen (allow walking in from off-screen)
    int edgesActivated[4] = {0, 0, 0, 0};
    // Only do it if nothing else has happened (eg. mouseclick)
    if ((numevents == numevents_was) &&
        ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0)) {

      if (playerchar->x <= thisroom.left)
        edgesActivated[0] = 1;
      else if (playerchar->x >= thisroom.right)
        edgesActivated[1] = 1;
      if (playerchar->y >= thisroom.bottom)
        edgesActivated[2] = 1;
      else if (playerchar->y <= thisroom.top)
        edgesActivated[3] = 1;

      if ((play.entered_edge >= 0) && (play.entered_edge <= 3)) {
        // once the player is no longer outside the edge, forget the stored edge
        if (edgesActivated[play.entered_edge] == 0)
          play.entered_edge = -10;
        // if we are walking in from off-screen, don't activate edges
        else
          edgesActivated[play.entered_edge] = 0;
      }

      for (int ii = 0; ii < 4; ii++) {
        if (edgesActivated[ii])
          setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, ii);
      }
    }
  }
  our_eip = 1008;

}  // end check_controls
