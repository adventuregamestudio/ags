
#include "util/wgt2allg.h"
#include "screenoverlay.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

void ScreenOverlay::ReadFromFile(DataStream *in)
{
    // Skipping bmp and pic pointer values
    in->ReadInt32();
    in->ReadInt32();
    type = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    timeout = in->ReadInt32();
    bgSpeechForChar = in->ReadInt32();
    associatedOverlayHandle = in->ReadInt32();
    hasAlphaChannel = in->ReadBool();
    positionRelativeToScreen = in->ReadBool();
    in->Seek(Common::kSeekCurrent, get_padding(sizeof(int8_t) * 2));
}

void ScreenOverlay::WriteToFile(DataStream *out)
{
    char padding[3] = {0,0,0};
    // Writing bitmap "pointers" to correspond to full structure writing
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(type);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(timeout);
    out->WriteInt32(bgSpeechForChar);
    out->WriteInt32(associatedOverlayHandle);
    out->WriteBool(hasAlphaChannel);
    out->WriteBool(positionRelativeToScreen);
    out->Write(padding, get_padding(sizeof(int8_t) * 2));
}
