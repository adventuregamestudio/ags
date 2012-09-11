
#include "ac/dialogtopic.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void DialogTopic::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadArray(optionnames, 150*sizeof(char), MAXTOPICOPTIONS);
    in->ReadArrayOfInt32(optionflags, MAXTOPICOPTIONS);
    optionscripts = (unsigned char *) in->ReadInt32();
    in->ReadArrayOfInt16(entrypoints, MAXTOPICOPTIONS);
    startupentrypoint = in->ReadInt16();//__getshort__bigendian(fp);
    codesize = in->ReadInt16();//__getshort__bigendian(fp);
    numoptions = in->ReadInt32();
    topicFlags = in->ReadInt32();
//#else
//    throw "DialogTopic::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}