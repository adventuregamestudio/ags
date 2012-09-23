
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
  if ( extendedCharacters[fontNumber] ) return; 

  // replace any extended characters with question marks
  while (text[0]!=0) {
    if ((unsigned char)text[0] > 127 ) // Why was this "> 126" ?
    {
      text[0] = '?';
    }
    text++;
  }
}


// Get the position of a character in a bitmap font.
// Rewritten code to fulfill the alignment restrictions of pointers
// on the MIPS processor. Can and should be done more efficiently.
unsigned char* psp_get_char(wgtfont foon, int thisCharacter)
{
  unsigned char* tabaddr_ptr = NULL;
  unsigned short tabaddr_value = 0;

  //tabaddr = (unsigned short *)&foon[15];
  tabaddr_ptr = (unsigned char*)&foon[15];

  //tabaddr = (unsigned short *)&foon[tabaddr[0]];     // get address table
  memcpy(&tabaddr_value, tabaddr_ptr, 2);
  tabaddr_ptr = (unsigned char*)&foon[tabaddr_value];

  //tabaddr = (unsigned short *)&foon[tabaddr[(unsigned char)charr]];      // use table to find character
  memcpy(&tabaddr_value, &tabaddr_ptr[thisCharacter*2], 2);

  return (unsigned char*)&foon[tabaddr_value];
}


int WFNFontRenderer::GetTextWidth(const char *texx, int fontNumber)
{
  wgtfont foon = fonts[fontNumber];
  //unsigned short *tabaddr;
  int totlen = 0;
  unsigned int dd;

  unsigned char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if ( !extendedCharacters[fontNumber] && (thisCharacter >= 128) )  thisCharacter = '?';
#ifdef ALLEGRO_BIG_ENDIAN
    unsigned short *tabaddr; // [IKM] 2012-06-13: added by guess, just to make this compile
    tabaddr = (unsigned short *)&foon[15];
    tabaddr = (unsigned short *)&foon[__short_swap_endian(tabaddr[0])];     // get address table
    tabaddr = (unsigned short *)&foon[__short_swap_endian(tabaddr[thisCharacter])];      // use table to find character
    totlen += __short_swap_endian(tabaddr[0]);
#else
    //tabaddr = (unsigned short *)&foon[15];
    //tabaddr = (unsigned short *)&foon[tabaddr[0]];     // get address table
    //tabaddr = (unsigned short *)&foon[tabaddr[thisCharacter]];      // use table to find character
    unsigned char* fontaddr = psp_get_char(foon, (unsigned char)thisCharacter);

    unsigned short tabaddr_d;
    memcpy(&tabaddr_d, (unsigned char*)((long)fontaddr + 0), 2);

    totlen += tabaddr_d;
#endif
  }
  return totlen * wtext_multiply;
}

int WFNFontRenderer::GetTextHeight(const char *texx, int fontNumber)
{
  //unsigned short *tabaddr;
  int highest = 0;
  unsigned int dd;
  wgtfont foon = fonts[fontNumber];

  unsigned char thisCharacter;
  for (dd = 0; dd < strlen(texx); dd++) 
  {
    thisCharacter = texx[dd];
    if ( !extendedCharacters[fontNumber] && (thisCharacter >= 128) )  thisCharacter = '?';
#ifdef ALLEGRO_BIG_ENDIAN
    unsigned short *tabaddr; // [IKM] 2012-06-13: added by guess, just to make this compile
    tabaddr = (unsigned short *)&foon[15];
    tabaddr = (unsigned short *)&foon[__short_swap_endian(tabaddr[0])];     // get address table
    tabaddr = (unsigned short *)&foon[__short_swap_endian(tabaddr[thisCharacter])];      // use table to find character
    totlen += __short_swap_endian(tabaddr[0]);
#else
    //tabaddr = (unsigned short *)&foon[15];
    //tabaddr = (unsigned short *)&foon[tabaddr[0]];     // get address table
    //tabaddr = (unsigned short *)&foon[tabaddr[thisCharacter]];      // use table to find character
    unsigned char* fontaddr = psp_get_char(foon, (unsigned char)thisCharacter);
    unsigned short tabaddr_d;
    memcpy(&tabaddr_d, (unsigned char*)((long)fontaddr + 2), 2);

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
    x += printchar(x, y, fontNumber, text[ee]);

  set_our_eip(oldeip);
}

int WFNFontRenderer::printchar(int xxx, int yyy, int fontNumber, int charr)
{
  wgtfont foo = fonts[fontNumber];
  //unsigned short *tabaddr = (unsigned short *)&foo[15];
  unsigned char *actdata;
  int tt, ss, bytewid, orixp = xxx;

  if ( !extendedCharacters[fontNumber] && ((unsigned char)charr >= 128) ) 
    charr = '?';

#ifdef ALLEGRO_BIG_ENDIAN
  unsigned short *tabaddr; // [IKM] 2012-06-13: added by guess, just to make this compile
  tabaddr = (unsigned short *)&foo[__short_swap_endian(tabaddr[0])];
  tabaddr = (unsigned short *)&foo[__short_swap_endian(tabaddr[(unsigned char)charr])];
  int charWidth = __short_swap_endian(tabaddr[0]);
  int charHeight = __short_swap_endian(tabaddr[1]);
  actdata = (unsigned char *)&tabaddr[2];
#else
  
  //tabaddr = (unsigned short *)&foo[tabaddr[0]];        // get address table
  //tabaddr = (unsigned short *)&foo[tabaddr[(unsigned char)charr]];    // use table to find character (MUST BE UNSIGNED! - Alan)
  unsigned char* tabaddr = psp_get_char(foo, (unsigned char)charr);

  unsigned short tabaddr_d;
  //int charWidth = tabaddr[0];
  memcpy(&tabaddr_d, (unsigned char*)((long)tabaddr), 2);
  int charWidth = tabaddr_d;

  //int charHeight = tabaddr[1];
  memcpy(&tabaddr_d, (unsigned char*)((long)tabaddr + 2), 2);
  int charHeight = tabaddr_d;

  actdata = (unsigned char *)&tabaddr[2*2];  // [AlanDrake] 2012-07-03: moved here because I doubt that 2*2 will work for big endian
#endif
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

  // Check if this font supports extended ASCII
  unsigned char* tabaddr_ptr = (unsigned char*)&fonts[fontNumber][15];
  unsigned short tabaddr_value = 0;
  memcpy(&tabaddr_value, tabaddr_ptr, 2); // address table
  extendedCharacters[fontNumber] = ((lenof - tabaddr_value)/2 > 128); // Are there more than 128 entries ?
  
  return true;
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  free(fonts[fontNumber]);
  fonts[fontNumber] = NULL;
}
