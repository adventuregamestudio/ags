
#include "util/wgt2allg.h"
#include "ac/gamesetupstructbase.h"

void GameSetupStructBase::ReadFromFile(FILE *fp)
{
    //#ifdef ALLEGRO_BIG_ENDIAN
    fread(&gamename[0], sizeof(char), 50, fp);
    fseek(fp, 2, SEEK_CUR);    // skip the array padding
    fread(options, sizeof(int), 100, fp);
    fread(&paluses[0], sizeof(unsigned char), 256, fp);
    // colors are an array of chars
    fread(&defpal[0], sizeof(char), sizeof(color)*256, fp);
    numviews = getw(fp);
    numcharacters = getw(fp);
    playercharacter = getw(fp);
    totalscore = getw(fp);
    numinvitems = getshort(fp);//__getshort__bigendian(fp);
    fseek(fp, 2, SEEK_CUR);    // skip the padding
    numdialog = getw(fp);
    numdlgmessage = getw(fp);
    numfonts = getw(fp);
    color_depth = getw(fp);
    target_win = getw(fp);
    dialog_bullet = getw(fp);
    hotdot = getshort(fp);//__getshort__bigendian(fp);
    hotdotouter = getshort(fp);//__getshort__bigendian(fp);
    uniqueid = getw(fp);
    numgui = getw(fp);
    numcursors = getw(fp);
    default_resolution = getw(fp);
    default_lipsync_frame = getw(fp);
    invhotdotsprite = getw(fp);
    fread(reserved, sizeof(int), 17, fp);
    // read the final ptrs so we know to load dictionary, scripts etc
    fread(messages, sizeof(int), MAXGLOBALMES, fp);
    dict = (WordsDictionary *) getw(fp);
    globalscript = (char *) getw(fp);
    chars = (CharacterInfo *) getw(fp);
    compiled_script = (ccScript *) getw(fp);
    //#else
    //    throw "GameSetupStructBase::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
    //#endif
}

void GameSetupStructBase::WriteToFile(FILE *fp)
{
    fwrite(&gamename[0], sizeof(char), 50, fp);
    char padding[2];
    fwrite(&padding, sizeof(char), 2, fp);    // skip the array padding
    fwrite(options, sizeof(int), 100, fp);
    fwrite(&paluses[0], sizeof(unsigned char), 256, fp);
    // colors are an array of chars
    fwrite(&defpal[0], sizeof(char), sizeof(color)*256, fp);
    putw(numviews, fp);
    putw(numcharacters, fp);
    putw(playercharacter, fp);
    putw(totalscore, fp);
    putshort(numinvitems, fp);//__getshort__bigendian(fp);
    fwrite(&padding, sizeof(char), 2, fp);    // skip the padding
    putw(numdialog, fp);
    putw(numdlgmessage, fp);
    putw(numfonts, fp);
    putw(color_depth, fp);
    putw(target_win, fp);
    putw(dialog_bullet, fp);
    putshort(hotdot, fp);//__getshort__bigendian(fp);
    putshort(hotdotouter, fp);//__getshort__bigendian(fp);
    putw(uniqueid, fp);
    putw(numgui, fp);
    putw(numcursors, fp);
    putw(default_resolution, fp);
    putw(default_lipsync_frame, fp);
    putw(invhotdotsprite, fp);
    fwrite(reserved, sizeof(int), 17, fp);
    // write the final ptrs so we know to load dictionary, scripts etc
    fwrite(messages, sizeof(int), MAXGLOBALMES, fp);
    putw((int32)dict, fp);
    putw((int32)globalscript, fp);
    putw((int32)chars, fp);
    putw((int32)compiled_script, fp);
}
