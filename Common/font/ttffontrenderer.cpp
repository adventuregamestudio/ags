
#ifndef USE_ALFONT
#define USE_ALFONT
#endif

#include "wgt2allg.h"
#include "alfont.h"

#include "font/ttffontrenderer.h"

// project-specific implementation
extern bool ShouldAntiAliasText();

extern "C"
{
  extern FILE *clibfopen(char *, char *);
  extern long cliboffset(char *);
  extern long clibfilesize(char *);
  extern long last_opened_size;
}

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <sys/stat.h>
#define _fileno fileno
off_t _filelength(int fd) {
	struct stat st;
	fstat(fd, &st);
	return st.st_size;
}
#endif



TTFFontRenderer ttfRenderer;

#ifdef USE_ALFONT
ALFONT_FONT *tempttffnt;
ALFONT_FONT *get_ttf_block(wgtfont fontptr)
{
  memcpy(&tempttffnt, &fontptr[4], sizeof(tempttffnt));
  return tempttffnt;
}
#endif // USE_ALFONT


// ***** TTF RENDERER *****
#ifdef USE_ALFONT	// declaration was not under USE_ALFONT though

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
