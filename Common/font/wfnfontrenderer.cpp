
#ifndef USE_ALFONT
#define USE_ALFONT
#endif

#include "util/wgt2allg.h"
#include "alfont.h"

#include "font/wfnfontrenderer.h"
#include "util/datastream.h"
#include "util/file.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
using AGS::Common::DataStream;
using namespace AGS; // FIXME later

extern void set_our_eip(int eip);
extern int  get_our_eip();

extern DataStream *fopen_shared(char *,
                                 Common::FileOpenMode open_mode = Common::kFile_Open,
                                 Common::FileWorkMode work_mode = Common::kFile_Read);
extern int flength_shared(DataStream *ffi);


// **** WFN Renderer ****

const char *WFN_FILE_SIGNATURE = "WGT Font File  ";
WFNFontRenderer wfnRenderer;


void WFNFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // Do nothing
}

void WFNFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  // replace any extended characters with question marks
  while (text[0]!=0) {
    if ((unsigned char)text[0] > 126) 
    {
      text[0] = '?';
    }
    text++;
  }
}


// Get the position of a character in a bitmap font.
// Rewritten code to fulfill the alignment restrictions of pointers
// on the MIPS processor. Can and should be done more efficiently.
char* psp_get_char(wgtfont foon, int thisCharacter)
{
  char* tabaddr_ptr = NULL;
  short tabaddr_value = 0;

  //tabaddr = (short *)&foon[15];
  tabaddr_ptr = (char*)&foon[15];

  //tabaddr = (short *)&foon[tabaddr[0]];     // get address table
  memcpy(&tabaddr_value, tabaddr_ptr, 2);
  tabaddr_ptr = (char*)&foon[tabaddr_value];

  //tabaddr = (short *)&foon[tabaddr[thisCharacter]];      // use table to find character
  memcpy(&tabaddr_value, &tabaddr_ptr[thisCharacter*2], 2);

  return (char*)&foon[tabaddr_value];
}


int WFNFontRenderer::GetTextWidth(const char *texx, int fontNumber)
{
  wgtfont foon = fonts[fontNumber];

  int totlen = 0;
  unsigned int dd;

  char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if ((thisCharacter >= 128) || (thisCharacter < 0)) thisCharacter = '?';
#ifdef ALLEGRO_BIG_ENDIAN
    short *tabaddr; // [IKM] 2012-06-13: added by guess, just to make this compile
    tabaddr = (short *)&foon[15];
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[0])];     // get address table
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[thisCharacter])];      // use table to find character
    totlen += __short_swap_endian(tabaddr[0]);
#else
    char* fontaddr = psp_get_char(foon, thisCharacter);

    short tabaddr_d;
    memcpy(&tabaddr_d, (char*)((long)fontaddr + 0), 2);

    totlen += tabaddr_d;
#endif
  }
  return totlen * wtext_multiply;
}

int WFNFontRenderer::GetTextHeight(const char *texx, int fontNumber)
{
  int highest = 0;
  unsigned int dd;
  wgtfont foon = fonts[fontNumber];

  char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if ((thisCharacter >= 128) || (thisCharacter < 0)) thisCharacter = '?';
#ifdef ALLEGRO_BIG_ENDIAN
    short *tabaddr; // [IKM] 2012-06-13: added by guess, just to make this compile
    tabaddr = (short *)&foon[15];
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[0])];     // get address table
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[thisCharacter])];      // use table to find character
    int charHeight = __short_swap_endian(tabaddr[1]);
#else
    char* fontaddr = psp_get_char(foon, thisCharacter);
    short tabaddr_d;
    memcpy(&tabaddr_d, (char*)((long)fontaddr + 2), 2);

    int charHeight = tabaddr_d;
#endif
    if (charHeight > highest)
      highest = charHeight;
  }
  return highest * wtext_multiply;
}

void WFNFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  unsigned int ee;

  int oldeip = get_our_eip();
  set_our_eip(415);

  for (ee = 0; ee < strlen(text); ee++)
    x += printchar(x, y, fonts[fontNumber], text[ee]);

  set_our_eip(oldeip);
}

int WFNFontRenderer::printchar(int xxx, int yyy, wgtfont foo, int charr)
{
  unsigned char *actdata;
  int tt, ss, bytewid, orixp = xxx;

  if ((charr > 127) || (charr < 0))
    charr = '?';

#ifdef ALLEGRO_BIG_ENDIAN
  short *tabaddr; // [IKM] 2012-06-13: added by guess, just to make this compile
  tabaddr = (short *)&foo[__short_swap_endian(tabaddr[0])];
  tabaddr = (short *)&foo[__short_swap_endian(tabaddr[charr])];
  int charWidth = __short_swap_endian(tabaddr[0]);
  int charHeight = __short_swap_endian(tabaddr[1]);
#else
  char* tabaddr = psp_get_char(foo, charr);

  short tabaddr_d;
  memcpy(&tabaddr_d, (char*)((long)tabaddr), 2);
  int charWidth = tabaddr_d;

  memcpy(&tabaddr_d, (char*)((long)tabaddr + 2), 2);
  int charHeight = tabaddr_d;
#endif

  actdata = (unsigned char *)&tabaddr[2*2];
  bytewid = ((charWidth - 1) / 8) + 1;

  // MACPORT FIX: switch now using charWidth and charHeight
  for (tt = 0; tt < charHeight; tt++) {
    for (ss = 0; ss < charWidth; ss++) {
      if (((actdata[tt * bytewid + (ss / 8)] & (0x80 >> (ss % 8))) != 0)) {
        if (wtext_multiply > 1) {
          abuf->FillRect(Rect(xxx + ss, yyy + tt, xxx + ss + (wtext_multiply - 1),
                   yyy + tt + (wtext_multiply - 1)), textcol);
        } 
        else
        {
          abuf->PutPixel(xxx + ss, yyy + tt, textcol);
        }
      }

      xxx += wtext_multiply - 1;
    }
    yyy += wtext_multiply - 1;
    xxx = orixp;
  }
  return charWidth * wtext_multiply;
}

bool WFNFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  char filnm[20];
  DataStream *ffi = NULL;
  char mbuffer[16];
  long lenof;

  sprintf(filnm, "agsfnt%d.wfn", fontNumber);
  ffi = fopen_shared(filnm);
  if (ffi == NULL)
  {
    // actual font not found, try font 0 instead
    strcpy(filnm, "agsfnt0.wfn");
    ffi = fopen_shared(filnm);
    if (ffi == NULL)
      return false;
  }

  mbuffer[15] = 0;
  ffi->ReadArray(mbuffer, 15, 1);
  if (strcmp(mbuffer, WFN_FILE_SIGNATURE) != 0) {
    delete ffi;
    return false;
  }

  lenof = flength_shared(ffi);

  wgtfont tempalloc = (wgtfont) malloc(lenof + 40);
  delete ffi;

  ffi = fopen_shared(filnm);
  ffi->ReadArray(tempalloc, lenof, 1);
  delete ffi;

  fonts[fontNumber] = tempalloc;
  return true;
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  free(fonts[fontNumber]);
  fonts[fontNumber] = NULL;
}
