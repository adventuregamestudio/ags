#ifndef __AC_FONT_H
#define __AC_FONT_H

void init_font_renderer();
void shutdown_font_renderer();
void adjust_y_coordinate_for_text(int* ypos, int fontnum);
void ensure_text_valid_for_font(char *text, int fontnum);
int wgettextwidth(const char *texx, int fontNumber);
int wgettextheight(const char *text, int fontNumber);
void wouttextxy(int xxx, int yyy, int fontNumber, const char *texx);
// Loads a font from disk
bool wloadfont_size(int fontNumber, int fsize);
void wgtprintf(int xxx, int yyy, int fontNumber, char *fmt, ...);
void wtextcolor(int nval);
void wfreefont(int fontNumber);
void wtexttransparent(int coo);

extern int texttrans;
extern int textcol;
extern int wtext_multiply;

#endif // __AC_FONT_H
