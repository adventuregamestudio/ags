
#include "ac/richgamemedia.h"
#include "util/datastream.h"

using AGS::Common::DataStream;

void RICH_GAME_MEDIA_HEADER::ReadFromFile(DataStream *in)
{
    dwMagicNumber = in->ReadInt32();
    dwHeaderVersion = in->ReadInt32();
    dwHeaderSize = in->ReadInt32();
    dwThumbnailOffsetLowerDword = in->ReadInt32();
    dwThumbnailOffsetHigherDword = in->ReadInt32();
    dwThumbnailSize = in->ReadInt32();
    in->Read(guidGameId, 16);
    in->ReadArrayOfInt16((int16_t*)szGameName, RM_MAXLENGTH);
    in->ReadArrayOfInt16((int16_t*)szSaveName, RM_MAXLENGTH);
    in->ReadArrayOfInt16((int16_t*)szLevelName, RM_MAXLENGTH);
    in->ReadArrayOfInt16((int16_t*)szComments, RM_MAXLENGTH);
}

void RICH_GAME_MEDIA_HEADER::WriteToFile(DataStream *out)
{
    out->WriteInt32(dwMagicNumber);
    out->WriteInt32(dwHeaderVersion);
    out->WriteInt32(dwHeaderSize);
    out->WriteInt32(dwThumbnailOffsetLowerDword);
    out->WriteInt32(dwThumbnailOffsetHigherDword);
    out->WriteInt32(dwThumbnailSize);
    out->Write(guidGameId, 16);
    out->WriteArrayOfInt16((int16_t*)szGameName, RM_MAXLENGTH);
    out->WriteArrayOfInt16((int16_t*)szSaveName, RM_MAXLENGTH);
    out->WriteArrayOfInt16((int16_t*)szLevelName, RM_MAXLENGTH);
    out->WriteArrayOfInt16((int16_t*)szComments, RM_MAXLENGTH);
}
