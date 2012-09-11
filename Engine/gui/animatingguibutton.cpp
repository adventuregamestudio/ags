
#include "gui/animatingguibutton.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void AnimatingGUIButton::ReadFromFile(CDataStream *in)
{
    buttonid = in->ReadInt16();
    ongui = in->ReadInt16();
    onguibut = in->ReadInt16();
    view = in->ReadInt16();
    loop = in->ReadInt16();
    frame = in->ReadInt16();
    speed = in->ReadInt16();
    repeat = in->ReadInt16();
    wait = in->ReadInt16();
    in->Seek(Common::kSeekCurrent, 3);
}

void AnimatingGUIButton::WriteToFile(CDataStream *out)
{
    char padding[3] = {0,0,0};
    out->WriteInt16(buttonid);
    out->WriteInt16(ongui);
    out->WriteInt16(onguibut);
    out->WriteInt16(view);
    out->WriteInt16(loop);
    out->WriteInt16(frame);
    out->WriteInt16(speed);
    out->WriteInt16(repeat);
    out->WriteInt16(wait);
    out->Write(padding, sizeof(char));
}
