/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#pragma unmanaged
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#include "wgt2allg_nofunc.h"
#include "acroom_nofunc.h"
#include "acruntim.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <sys/stat.h>
#define _fileno fileno
off_t _filelength(int fd) {
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}
#endif

typedef unsigned char* wgtfont;

extern FILE *fopen_shared(char *, char *);
extern int flength_shared(FILE *ffi);

extern "C"
{
  extern FILE *clibfopen(char *, char *);
  extern long cliboffset(char *);
  extern long clibfilesize(char *);
  extern long last_opened_size;
}

class WFNFontRenderer : public IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize);
  virtual void FreeMemory(int fontNumber);
  virtual bool SupportsExtendedCharacters(int fontNumber) { return false; }
  virtual int GetTextWidth(const char *text, int fontNumber);
  virtual int GetTextHeight(const char *text, int fontNumber);
  virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) ;
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber);
  virtual void EnsureTextValidForFont(char *text, int fontNumber);

private:
  int printchar(int xxx, int yyy, wgtfont foo, int charr);
};

class TTFFontRenderer : public IAGSFontRenderer {
public:
  virtual bool LoadFromDisk(int fontNumber, int fontSize);
  virtual void FreeMemory(int fontNumber);
  virtual bool SupportsExtendedCharacters(int fontNumber) { return true; }
  virtual int GetTextWidth(const char *text, int fontNumber);
  virtual int GetTextHeight(const char *text, int fontNumber);
  virtual void RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour) ;
  virtual void AdjustYCoordinateForFont(int *ycoord, int fontNumber);
  virtual void EnsureTextValidForFont(char *text, int fontNumber);
};

extern bool ShouldAntiAliasText();
extern int our_eip;
int texttrans = 0;
int textcol;
int wtext_multiply = 1;
wgtfont fonts[MAX_FONTS];
IAGSFontRenderer* fontRenderers[MAX_FONTS];
WFNFontRenderer wfnRenderer;
TTFFontRenderer ttfRenderer;

#ifdef USE_ALFONT
ALFONT_FONT *tempttffnt;
ALFONT_FONT *get_ttf_block(wgtfont fontptr)
{
  memcpy(&tempttffnt, &fontptr[4], sizeof(tempttffnt));
  return tempttffnt;
}
#endif // USE_ALFONT

void init_font_renderer()
{
#ifdef USE_ALFONT
  alfont_init();
  alfont_text_mode(-1);
#endif

  for (int i = 0; i < MAX_FONTS; i++)
    fontRenderers[i] = NULL;
  wtexttransparent(TEXTFG);
}

extern void set_our_eip(int eip);
extern int  get_our_eip();

void shutdown_font_renderer()
{
#ifdef USE_ALFONT
  set_our_eip(9919);
  alfont_exit();
#endif
}

void adjust_y_coordinate_for_text(int* ypos, int fontnum)
{
  fontRenderers[fontnum]->AdjustYCoordinateForFont(ypos, fontnum);
}

void ensure_text_valid_for_font(char *text, int fontnum)
{
  fontRenderers[fontnum]->EnsureTextValidForFont(text, fontnum);
}

int wgettextwidth(const char *texx, int fontNumber)
{
  return fontRenderers[fontNumber]->GetTextWidth(texx, fontNumber);
}

int wgettextheight(const char *text, int fontNumber)
{
  return fontRenderers[fontNumber]->GetTextHeight(text, fontNumber);
}

void wouttextxy(int xxx, int yyy, int fontNumber, const char *texx)
{
  if (yyy > abuf->cb)
    return;                   // each char is clipped but this speeds it up

  if (fontRenderers[fontNumber] != NULL)
  {
    fontRenderers[fontNumber]->RenderText(texx, fontNumber, abuf, xxx, yyy, textcol);
  }
}

// Loads a font from disk
bool wloadfont_size(int fontNumber, int fsize)
{
  if (ttfRenderer.LoadFromDisk(fontNumber, fsize))
  {
    fontRenderers[fontNumber] = &ttfRenderer;
    return true;
  }
  else if (wfnRenderer.LoadFromDisk(fontNumber, fsize))
  {
    fontRenderers[fontNumber] = &wfnRenderer;
    return true;
  }

  return false;
}

void wgtprintf(int xxx, int yyy, int fontNumber, char *fmt, ...)
{
  char tbuffer[2000];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(tbuffer, fmt, ap);
  va_end(ap);
  wouttextxy(xxx, yyy, fontNumber, tbuffer);
}

void wtextcolor(int nval)
{
  __my_setcolor(&textcol, nval);
}

void wfreefont(int fontNumber)
{
  if (fontRenderers[fontNumber] != NULL)
    fontRenderers[fontNumber]->FreeMemory(fontNumber);

  fontRenderers[fontNumber] = NULL;
}

void wtexttransparent(int coo)
{
  texttrans = coo;
}


// **** WFN Renderer ****

const char *WFN_FILE_SIGNATURE = "WGT Font File  ";

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
#ifndef ALLEGRO_BIG_ENDIAN

    char* fontaddr = psp_get_char(foon, thisCharacter);

    short tabaddr_d;
    memcpy(&tabaddr_d, (char*)((int)fontaddr + 0), 2);

    totlen += tabaddr_d;

#else
    tabaddr = (short *)&foon[15];
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[0])];     // get address table
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[thisCharacter])];      // use table to find character
    totlen += __short_swap_endian(tabaddr[0]);
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
#ifndef ALLEGRO_BIG_ENDIAN

    char* fontaddr = psp_get_char(foon, thisCharacter);
    short tabaddr_d;
    memcpy(&tabaddr_d, (char*)((int)fontaddr + 2), 2);

    int charHeight = tabaddr_d;

#else
    tabaddr = (short *)&foon[15];
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[0])];     // get address table
    tabaddr = (short *)&foon[__short_swap_endian(tabaddr[thisCharacter])];      // use table to find character
    int charHeight = __short_swap_endian(tabaddr[1]);
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

#ifndef ALLEGRO_BIG_ENDIAN

  char* tabaddr = psp_get_char(foo, charr);

  short tabaddr_d;
  memcpy(&tabaddr_d, (char*)((int)tabaddr), 2);
  int charWidth = tabaddr_d;

  memcpy(&tabaddr_d, (char*)((int)tabaddr + 2), 2);
  int charHeight = tabaddr_d;

#else
  tabaddr = (short *)&foo[__short_swap_endian(tabaddr[0])];
  tabaddr = (short *)&foo[__short_swap_endian(tabaddr[charr])];
  int charWidth = __short_swap_endian(tabaddr[0]);
  int charHeight = __short_swap_endian(tabaddr[1]);
#endif

  actdata = (unsigned char *)&tabaddr[2*2];
  bytewid = ((charWidth - 1) / 8) + 1;

  // MACPORT FIX: switch now using charWidth and charHeight
  for (tt = 0; tt < charHeight; tt++) {
    for (ss = 0; ss < charWidth; ss++) {
      if (((actdata[tt * bytewid + (ss / 8)] & (0x80 >> (ss % 8))) != 0)) {
        if (wtext_multiply > 1) {
          rectfill(abuf, xxx + ss, yyy + tt, xxx + ss + (wtext_multiply - 1),
                   yyy + tt + (wtext_multiply - 1), textcol);
        } 
        else
        {
          putpixel(abuf, xxx + ss, yyy + tt, textcol);
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
  FILE *ffi = NULL;
  char mbuffer[16];
  long lenof;

  sprintf(filnm, "agsfnt%d.wfn", fontNumber);
  ffi = fopen_shared(filnm, "rb");
  if (ffi == NULL)
  {
    // actual font not found, try font 0 instead
    strcpy(filnm, "agsfnt0.wfn");
    ffi = fopen_shared(filnm, "rb");
    if (ffi == NULL)
      return false;
  }

  mbuffer[15] = 0;
  fread(mbuffer, 15, 1, ffi);
  if (strcmp(mbuffer, WFN_FILE_SIGNATURE) != 0) {
    fclose(ffi);
    return false;
  }

  lenof = flength_shared(ffi);

  wgtfont tempalloc = (wgtfont) malloc(lenof + 40);
  fclose(ffi);

  ffi = fopen_shared(filnm, "rb");
  fread(tempalloc, lenof, 1, ffi);
  fclose(ffi);

  fonts[fontNumber] = tempalloc;
  return true;
}

void WFNFontRenderer::FreeMemory(int fontNumber)
{
  free(fonts[fontNumber]);
  fonts[fontNumber] = NULL;
}


// ***** TTF RENDERER *****
#ifdef USE_ALFONT

void TTFFontRenderer::AdjustYCoordinateForFont(int *ycoord, int fontNumber)
{
  // TTF fonts already have space at the top, so try to remove the gap
  ycoord[0]--;
}

void TTFFontRenderer::EnsureTextValidForFont(char *text, int fontNumber)
{
  // do nothing, TTF can handle all characters
}

int TTFFontRenderer::GetTextWidth(const char *text, int fontNumber)
{
  return alfont_text_length(get_ttf_block(fonts[fontNumber]), text);
}

int TTFFontRenderer::GetTextHeight(const char *text, int fontNumber)
{
  return alfont_text_height(get_ttf_block(fonts[fontNumber]));
}

void TTFFontRenderer::RenderText(const char *text, int fontNumber, BITMAP *destination, int x, int y, int colour)
{
  if (y > destination->cb)  // optimisation
    return;

  ALFONT_FONT *alfpt = get_ttf_block(fonts[fontNumber]);
  // Y - 1 because it seems to get drawn down a bit
  if ((ShouldAntiAliasText()) && (bitmap_color_depth(abuf) > 8))
    alfont_textout_aa(abuf, alfpt, text, x, y - 1, colour);
  else
    alfont_textout(abuf, alfpt, text, x, y - 1, colour);
}

bool TTFFontRenderer::LoadFromDisk(int fontNumber, int fontSize)
{
  char filnm[20];
  sprintf(filnm, "agsfnt%d.ttf", fontNumber);

  // we read the font in manually to make it load from library file
  FILE *reader = clibfopen(filnm, "rb");
  char *membuffer;

  if (reader == NULL)
    return false;

  long lenof = clibfilesize(filnm);

  // if not in the library, get size manually
  if (lenof < 1)
  {
	  lenof = _filelength(_fileno(reader));
  }

  membuffer = (char *)malloc(lenof);
  fread(membuffer, lenof, 1, reader);
  fclose(reader);

  ALFONT_FONT *alfptr = alfont_load_font_from_mem(membuffer, lenof);
  free(membuffer);

  if (alfptr == NULL)
    return false;

  if (fontSize > 0)
    alfont_set_font_size(alfptr, fontSize);

  wgtfont tempalloc = (wgtfont) malloc(20);
  strcpy((char *)tempalloc, "TTF");
  memcpy(&((char *)tempalloc)[4], &alfptr, sizeof(alfptr));
  fonts[fontNumber] = tempalloc;

  return true;
}

void TTFFontRenderer::FreeMemory(int fontNumber)
{
  alfont_destroy_font(get_ttf_block(fonts[fontNumber]));
  free(fonts[fontNumber]);
  fonts[fontNumber] = NULL;
}

#endif   // USE_ALFONT
