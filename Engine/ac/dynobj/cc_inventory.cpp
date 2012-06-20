
#include "ac/dynobj/cc_inventory.h"
#include "ac/dynobj/scriptinvitem.h"
#include "ac/ac_characterinfo.h"

extern ScriptInvItem scrInv[MAX_INV];

// return the type name of the object
const char *CCInventory::GetType() {
    return "Inventory";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCInventory::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptInvItem *shh = (ScriptInvItem*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
}

void CCInventory::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrInv[num], this);
}