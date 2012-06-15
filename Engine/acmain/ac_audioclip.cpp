
#include "acmain/ac_maindefines.h"

// Create the missing game.audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
extern "C" {
    extern int clibGetNumFiles();
    extern char *clibGetFileName(int index);
}

void BuildAudioClipArray()
{
    char temp_name[30];
    int temp_number;
    char temp_extension[10];

    int number_of_files = clibGetNumFiles();
    int i;
    for (i = 0; i < number_of_files; i++)
    {
        if (sscanf(clibGetFileName(i), "%5s%d.%3s", temp_name, &temp_number, temp_extension) == 3)
        {
            if (stricmp(temp_name, "music") == 0)
            {
                sprintf(game.audioClips[game.audioClipCount].scriptName, "aMusic%d", temp_number);
                sprintf(game.audioClips[game.audioClipCount].fileName, "music%d.%s", temp_number, temp_extension);
                game.audioClips[game.audioClipCount].bundlingType = (stricmp(temp_extension, "mid") == 0) ? AUCL_BUNDLE_EXE : AUCL_BUNDLE_VOX;
                game.audioClips[game.audioClipCount].type = 2;
                game.audioClips[game.audioClipCount].defaultRepeat = 1;
            }
            else if (stricmp(temp_name, "sound") == 0)
            {
                sprintf(game.audioClips[game.audioClipCount].scriptName, "aSound%d", temp_number);
                sprintf(game.audioClips[game.audioClipCount].fileName, "sound%d.%s", temp_number, temp_extension);
                game.audioClips[game.audioClipCount].bundlingType = AUCL_BUNDLE_EXE;
                game.audioClips[game.audioClipCount].type = 3;
            }
            else
            {
                continue;
            }

            game.audioClips[game.audioClipCount].defaultVolume = 100;
            game.audioClips[game.audioClipCount].defaultPriority = 50;
            game.audioClips[game.audioClipCount].id = game.audioClipCount;

            if (stricmp(temp_extension, "mp3") == 0)
                game.audioClips[game.audioClipCount].fileType = eAudioFileMP3;
            else if (stricmp(temp_extension, "wav") == 0)
                game.audioClips[game.audioClipCount].fileType = eAudioFileWAV;
            else if (stricmp(temp_extension, "voc") == 0)
                game.audioClips[game.audioClipCount].fileType = eAudioFileVOC;
            else if (stricmp(temp_extension, "mid") == 0)
                game.audioClips[game.audioClipCount].fileType = eAudioFileMIDI;
            else if ((stricmp(temp_extension, "mod") == 0) || (stricmp(temp_extension, "xm") == 0)
                || (stricmp(temp_extension, "s3m") == 0) || (stricmp(temp_extension, "it") == 0))
                game.audioClips[game.audioClipCount].fileType = eAudioFileMOD;
            else if (stricmp(temp_extension, "ogg") == 0)
                game.audioClips[game.audioClipCount].fileType = eAudioFileOGG;

            game.audioClipCount++;
        }
    }
}

