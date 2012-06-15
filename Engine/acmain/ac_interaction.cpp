
#include "acmain/ac_maindefines.h"


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
