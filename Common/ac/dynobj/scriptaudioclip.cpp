
#include "ac/dynobj/scriptaudioclip.h"

void ScriptAudioClip::ReadFromFile(FILE *f)
{
    char padding[3] = {0,0,0};

    id = getw(f);
    fread(scriptName, sizeof(char), SCRIPTAUDIOCLIP_SCRIPTNAMELENGTH, f);
    fread(fileName, sizeof(char), SCRIPTAUDIOCLIP_FILENAMELENGTH, f);
    bundlingType = fgetc(f);
    type = fgetc(f);
    fileType = fgetc(f);
    defaultRepeat = fgetc(f);
    defaultPriority = getshort(f);
    defaultVolume = getshort(f);
    fread(&padding, sizeof(char),
        get_padding(SCRIPTAUDIOCLIP_SCRIPTNAMELENGTH + SCRIPTAUDIOCLIP_FILENAMELENGTH), f);
    reserved = getw(f);
}
