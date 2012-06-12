
#include "wgt2allg_nofunc.h"        // macros, typedef
#include "ac_gamesetupstruct.h"

#ifdef ALLEGRO_BIG_ENDIAN
void GameSetupStructBase::ReadFromFile(FILE *fp)
{
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
    numinvitems = __getshort__bigendian(fp);
    fseek(fp, 2, SEEK_CUR);    // skip the padding
    numdialog = getw(fp);
    numdlgmessage = getw(fp);
    numfonts = getw(fp);
    color_depth = getw(fp);
    target_win = getw(fp);
    dialog_bullet = getw(fp);
    hotdot = __getshort__bigendian(fp);
    hotdotouter = __getshort__bigendian(fp);
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
}
#endif