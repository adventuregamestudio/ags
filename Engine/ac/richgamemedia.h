
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__RICHGAMEMEDIA_H
#define __AGS_EE_AC__RICHGAMEMEDIA_H

// Windows Vista Rich Save Games, modified to be platform-agnostic

#define RM_MAXLENGTH    1024
#define RM_MAGICNUMBER  "RGMH"

#pragma pack(push)
#pragma pack(1)
typedef struct _RICH_GAME_MEDIA_HEADER
{
    long       dwMagicNumber;
    long       dwHeaderVersion;
    long       dwHeaderSize;
    long       dwThumbnailOffsetLowerDword;
    long       dwThumbnailOffsetHigherDword;
    long       dwThumbnailSize;
    unsigned char guidGameId[16];
    unsigned short szGameName[RM_MAXLENGTH];
    unsigned short szSaveName[RM_MAXLENGTH];
    unsigned short szLevelName[RM_MAXLENGTH];
    unsigned short szComments[RM_MAXLENGTH];
} RICH_GAME_MEDIA_HEADER;
#pragma pack(pop)

#endif // __AGS_EE_AC__RICHGAMEMEDIA_H
