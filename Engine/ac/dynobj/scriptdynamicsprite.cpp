
#include "util/wgt2allg.h"
#include "ac/dynobj/scriptdynamicsprite.h"
#include "ac/dynamicsprite.h"

int ScriptDynamicSprite::Dispose(const char *address, bool force) {
    // always dispose
    if ((slot) && (!force))
        free_dynamic_sprite(slot);

    delete this;
    return 1;
}

const char *ScriptDynamicSprite::GetType() {
    return "DynamicSprite";
}

int ScriptDynamicSprite::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(slot);
    return EndSerialize();
}

void ScriptDynamicSprite::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    slot = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite(int theSlot) {
    slot = theSlot;
    ccRegisterManagedObject(this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite() {
    slot = 0;
}
