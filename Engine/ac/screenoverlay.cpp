
#include "util/wgt2allg.h"
#include "screenoverlay.h"

void ScreenOverlay::ReadFromFile(FILE *f)
{
    // Skipping bmp and pic pointer values
    getw(f);
    getw(f);
    type = getw(f);
    x = getw(f);
    y = getw(f);
    timeout = getw(f);
    bgSpeechForChar = getw(f);
    associatedOverlayHandle = getw(f);
    // [IKM] According to C++ standard, "sizeof(bool) is not required to be 1"
    // On other hand, ScreenOverlay is serialized only when game saves, and
    // so we may assume it is unserialized on the same system... for now.
    fread(&hasAlphaChannel, sizeof(bool), 1, f);
    fread(&positionRelativeToScreen, sizeof(bool), 1, f);
    fseek(f, get_padding(sizeof(bool) * 2), SEEK_CUR);
}

void ScreenOverlay::WriteToFile(FILE *f)
{
    char padding[3] = {0,0,0};
    // Writing bitmap "pointers" to correspond to full structure writing
    putw(0, f);
    putw(0, f);
    putw(type, f);
    putw(x, f);
    putw(y, f);
    putw(timeout, f);
    putw(bgSpeechForChar, f);
    putw(associatedOverlayHandle, f);
    fwrite(&hasAlphaChannel, sizeof(bool), 1, f);
    fwrite(&positionRelativeToScreen, sizeof(bool), 1, f);
    fwrite(padding, sizeof(char), get_padding(sizeof(bool) * 2), f);
}
