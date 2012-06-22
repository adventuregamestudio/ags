
#include "ac/dynobj/cc_character.h"
#include "ac/characterinfo.h"
#include "wgt2allg.h"
#include "ac/ac_gamesetupstruct.h"

extern GameSetupStruct game;

// return the type name of the object
const char *CCCharacter::GetType() {
    return "Character";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCCharacter::Serialize(const char *address, char *buffer, int bufsize) {
    CharacterInfo *chaa = (CharacterInfo*)address;
    StartSerialize(buffer);
    SerializeInt(chaa->index_id);
    return EndSerialize();
}

void CCCharacter::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &game.chars[num], this);
}
