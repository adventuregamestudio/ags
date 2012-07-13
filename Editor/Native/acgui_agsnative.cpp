//
// Implementation from acgui.h and acgui.cpp specific to AGS.Native library
//

// Headers, as they are in acgui.cpp
#pragma unmanaged
//#include "wgt2allg_nofunc.h"
#include "wgt2allg.h"
//#include "acroom_nofunc.h"
//#include "acruntim.h"
//#include "acgui.h"
#include "gui/guimain.h"
#include "gui/guilabel.h"
#include "gui/guitextbox.h"
#include "gui/guibutton.h"
#include "gui/guilistbox.h"
#include <ctype.h>

#include "bigend.h"

#undef CROOM_NOFUNCTIONS

//=============================================================================
// AGS.Native-specific implementation split out of acgui.h
//=============================================================================

int GUIObject::IsClickable()
{
  // make sure the button can be selected in the editor
  return 1;
}

void wouttext_outline(int xxp, int yyp, int usingfont, char *texx)
{
  wouttextxy(xxp, yyp, usingfont, texx);
}

//=============================================================================
// AGS.Native-specific implementation split out of acgui.cpp
//=============================================================================

int final_col_dep = 32;

bool is_sprite_alpha(int spr)
{
  return false;
}

void set_eip_guiobj(int eip)
{
  // do nothing
}

int get_eip_guiobj()
{
  return 0;
}

int wgettextwidth_compensate(const char *tex, int font)
{
  return wgettextwidth(tex, font);
}

void GUILabel::Draw_replace_macro_tokens(char *oritext, char *text)
{
  strcpy(oritext, text);
}

// [IKM] 2012-07-14: Copied those two functions here (same are in the Engine),
// although this sucks. I guess it would be better if all similar utility
// functions would be put in Common, regardless of where they are used.
// Will do that later.
//------------------------------------------------------------------------
void removeBackslashBracket(char *lbuffer) {
    char *slashoffs;
    while ((slashoffs = strstr(lbuffer, "\\[")) != NULL) {
        // remove the backslash
        memmove(slashoffs, slashoffs + 1, strlen(slashoffs));
    }
}
// Break up the text into lines, using normal Western left-right style
void split_lines_leftright(const char *todis, int wii, int fonnt) {
    // v2.56.636: rewrote this function because the old version
    // was crap and buggy
    int i = 0;
    int nextCharWas;
    int splitAt;
    char *theline;
    // make a copy, since we change characters in the original string
    // and this might be in a read-only bit of memory
    char textCopyBuffer[3000];
    strcpy(textCopyBuffer, todis);
    theline = textCopyBuffer;

    while (1) {
        splitAt = -1;

        if (theline[i] == 0) {
            // end of the text, add the last line if necessary
            if (i > 0) {
                strcpy(lines[numlines], theline);
                removeBackslashBracket(lines[numlines]);
                numlines++;
            }
            break;
        }

        // temporarily terminate the line here and test its width
        nextCharWas = theline[i + 1];
        theline[i + 1] = 0;

        // force end of line with the [ character (except if \[ )
        if ((theline[i] == '[') && ((i == 0) || (theline[i - 1] != '\\')))
            splitAt = i;
        // otherwise, see if we are too wide
        else if (wgettextwidth_compensate(theline, fonnt) >= wii) {
            int endline = i;
            while ((theline[endline] != ' ') && (endline > 0))
                endline--;

            // single very wide word, display as much as possible
            if (endline == 0)
                endline = i - 1;

            splitAt = endline;
        }

        // restore the character that was there before
        theline[i + 1] = nextCharWas;

        if (splitAt >= 0) {
            // add this line
            nextCharWas = theline[splitAt];
            theline[splitAt] = 0;
            strcpy(lines[numlines], theline);
            removeBackslashBracket(lines[numlines]);
            numlines++;
            theline[splitAt] = nextCharWas;
            if (numlines >= MAXLINE) {
                strcat(lines[numlines-1], "...");
                break;
            }
            // the next line starts from here
            theline += splitAt;
            // skip the space or bracket that caused the line break
            if ((theline[0] == ' ') || (theline[0] == '['))
                theline++;
            i = -1;
        }

        i++;
    }
}
//-----------------------------------------------------------------------------

void GUILabel::Draw_split_lines(char *teptr, int wid, int font, int &numlines)
{
  numlines=0;
  split_lines_leftright(teptr, wid, font);
}

void GUITextBox::Draw_text_box_contents()
{
  // print something fake so we can see what it looks like
  wouttext_outline(x + 2, y + 2, font, "Text Box Contents");
}

void GUIListBox::Draw_items_fix()
{
  numItemsTemp = numItems;
  numItems = 2;
  items[0] = "Sample selected";
  items[1] = "Sample item";
  //
  // [IKM] 2012-06-08: a-a-a-and just to test this thing is working....
  numItems = 3;
  items[2] = "This item means HACK!! >:]";
  // ^_^
  //
}

void GUIListBox::Draw_items_unfix()
{
  numItems = numItemsTemp;
}

void GUIButton::Draw_set_oritext(char *oritext, const char *text)
{
  strcpy(oritext, text);

  // original code was:
  //      oritext = text; 
  // it seems though the 'text' variable is assumed to be a null-terminated string
  // oritext is assumed to be made long enough by caller function
}
