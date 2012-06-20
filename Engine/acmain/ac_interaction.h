
#include "ac/dynobj/scriptinvitem.h"
#include "ac/dynobj/scriptobject.h"
#include "ac/dynobj/scriptregion.h"
#include "ac/dynobj/scripthotspot.h"
#include "ac/ac_characterinfo.h"

void RunInventoryInteraction (int iit, int modd);
void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood);
void RunObjectInteraction (int aa, int mood);
void Object_RunInteraction(ScriptObject *objj, int mode);
void RunCharacterInteraction (int cc, int mood);
void Character_RunInteraction(CharacterInfo *chaa, int mood);
int check_click_on_character(int xx,int yy,int mood);
void RunRegionInteraction (int regnum, int mood);
void Region_RunInteraction(ScriptRegion *ssr, int mood);
void RunHotspotInteraction (int hotspothere, int mood);
void Hotspot_RunInteraction (ScriptHotspot *hss, int mood);
void ProcessClick(int xx,int yy,int mood);
int IsInventoryInteractionAvailable (int item, int mood);
int InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood);
int IsInteractionAvailable (int xx,int yy,int mood);