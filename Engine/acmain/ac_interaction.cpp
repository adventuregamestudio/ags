
#include <stdio.h>
#include "wgt2allg.h"
#include "ac/global_hotspot.h"
#include "ac/hotspot.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_interaction.h"
#include "acmain/ac_commonheaders.h"

void NewInteractionCommand::remove () {
  if (children != NULL) {
    children->reset();
    delete children;
  }
  children = NULL;
  parent = NULL;
  type = 0;
}

void RunInventoryInteraction (int iit, int modd) {
    if ((iit < 0) || (iit >= game.numinvitems))
        quit("!RunInventoryInteraction: invalid inventory number");

    evblocknum = iit;
    if (modd == MODE_LOOK)
        run_event_block_inv(iit, 0);
    else if (modd == MODE_HAND)
        run_event_block_inv(iit, 1);
    else if (modd == MODE_USE) {
        play.usedinv = playerchar->activeinv;
        run_event_block_inv(iit, 3);
    }
    else if (modd == MODE_TALK)
        run_event_block_inv(iit, 2);
    else // other click on invnetory
        run_event_block_inv(iit, 4);
}

void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood) {
    RunInventoryInteraction(iitem->id, mood);
}

void RunRegionInteraction (int regnum, int mood) {
    if ((regnum < 0) || (regnum >= MAX_REGIONS))
        quit("!RunRegionInteraction: invalid region speicfied");
    if ((mood < 0) || (mood > 2))
        quit("!RunRegionInteraction: invalid event specified");

    // We need a backup, because region interactions can run
    // while another interaction (eg. hotspot) is in a Wait
    // command, and leaving our basename would call the wrong
    // script later on
    char *oldbasename = evblockbasename;
    int   oldblocknum = evblocknum;

    evblockbasename = "region%d";
    evblocknum = regnum;

    if (thisroom.regionScripts != NULL)
    {
        run_interaction_script(thisroom.regionScripts[regnum], mood);
    }
    else
    {
        run_interaction_event(&croom->intrRegion[regnum], mood);
    }

    evblockbasename = oldbasename;
    evblocknum = oldblocknum;
}

void Region_RunInteraction(ScriptRegion *ssr, int mood) {
    RunRegionInteraction(ssr->id, mood);
}

void ProcessClick(int xx,int yy,int mood) {
    getloctype_throughgui = 1;
    int loctype = GetLocationType (xx, yy);
    xx += divide_down_coordinate(offsetx); 
    yy += divide_down_coordinate(offsety);

    if ((mood==MODE_WALK) && (game.options[OPT_NOWALKMODE]==0)) {
        int hsnum=get_hotspot_at(xx,yy);
        if (hsnum<1) ;
        else if (thisroom.hswalkto[hsnum].x<1) ;
        else if (play.auto_use_walkto_points == 0) ;
        else {
            xx=thisroom.hswalkto[hsnum].x;
            yy=thisroom.hswalkto[hsnum].y;
            DEBUG_CONSOLE("Move to walk-to point hotspot %d", hsnum);
        }
        walk_character(game.playercharacter,xx,yy,0, true);
        return;
    }
    play.usedmode=mood;

    if (loctype == 0) {
        // click on nothing -> hotspot 0
        getloctype_index = 0;
        loctype = LOCTYPE_HOTSPOT;
    }

    if (loctype == LOCTYPE_CHAR) {
        if (check_click_on_character(xx,yy,mood)) return;
    }
    else if (loctype == LOCTYPE_OBJ) {
        if (check_click_on_object(xx,yy,mood)) return;
    }
    else if (loctype == LOCTYPE_HOTSPOT) 
        RunHotspotInteraction (getloctype_index, mood);
}



int IsInventoryInteractionAvailable (int item, int mood) {
  if ((item < 0) || (item >= MAX_INV))
    quit("!IsInventoryInteractionAvailable: invalid inventory number");

  play.check_interaction_only = 1;

  RunInventoryInteraction(item, mood);

  int ciwas = play.check_interaction_only;
  play.check_interaction_only = 0;

  if (ciwas == 2)
    return 1;

  return 0;
}

int InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood) {
  return IsInventoryInteractionAvailable(iitem->id, mood);
}




int IsInteractionAvailable (int xx,int yy,int mood) {
  getloctype_throughgui = 1;
  int loctype = GetLocationType (xx, yy);
  xx += divide_down_coordinate(offsetx); 
  yy += divide_down_coordinate(offsety);

  // You can always walk places
  if ((mood==MODE_WALK) && (game.options[OPT_NOWALKMODE]==0))
    return 1;

  play.check_interaction_only = 1;

  if (loctype == 0) {
    // click on nothing -> hotspot 0
    getloctype_index = 0;
    loctype = LOCTYPE_HOTSPOT;
  }
  
  if (loctype == LOCTYPE_CHAR) {
    check_click_on_character(xx,yy,mood);
  }
  else if (loctype == LOCTYPE_OBJ) {
    check_click_on_object(xx,yy,mood);
  }
  else if (loctype == LOCTYPE_HOTSPOT)
    RunHotspotInteraction (getloctype_index, mood);

  int ciwas = play.check_interaction_only;
  play.check_interaction_only = 0;

  if (ciwas == 2)
    return 1;

  return 0;
}
