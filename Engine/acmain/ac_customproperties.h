
#include "acrun/ac_scriptobject.h"
#include "ac/ac_characterinfo.h"


int GetInvProperty (int item, const char *property);
int InventoryItem_GetProperty(ScriptInvItem *scii, const char *property);
int GetCharacterProperty (int cha, const char *property);
int Character_GetProperty(CharacterInfo *chaa, const char *property);
void SetCharacterProperty (int who, int flag, int yesorno);
int GetHotspotProperty (int hss, const char *property);
int Hotspot_GetProperty (ScriptHotspot *hss, const char *property);
int GetObjectProperty (int hss, const char *property);
int Object_GetProperty (ScriptObject *objj, const char *property);
int GetRoomProperty (const char *property);
void GetInvPropertyText (int item, const char *property, char *bufer);
void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer);
const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property);
void GetCharacterPropertyText (int item, const char *property, char *bufer);
void Character_GetPropertyText(CharacterInfo *chaa, const char *property, char *bufer);
const char* Character_GetTextProperty(CharacterInfo *chaa, const char *property);
void GetHotspotPropertyText (int item, const char *property, char *bufer);
void Hotspot_GetPropertyText (ScriptHotspot *hss, const char *property, char *bufer);
const char* Hotspot_GetTextProperty(ScriptHotspot *hss, const char *property);
void GetObjectPropertyText (int item, const char *property, char *bufer);
void Object_GetPropertyText(ScriptObject *objj, const char *property, char *bufer);
const char* Object_GetTextProperty(ScriptObject *objj, const char *property);
void GetRoomPropertyText (const char *property, char *bufer);
const char* Room_GetTextProperty(const char *property);
