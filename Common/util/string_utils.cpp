//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "gui/guidefines.h"
#include "util/string_utils.h"
#include "util/stream.h"

using AGS::Common::Stream;

#define STD_BUFFER_SIZE 3000

void removeBackslashBracket(char *lbuffer) {
    char *slashoffs = lbuffer;
    while ((slashoffs = strstr(slashoffs, "\\[")) != NULL) {
        // remove the backslash
        memmove(slashoffs, slashoffs + 1, strlen(slashoffs));
    }
}

char lines[MAXLINE][200];
int  numlines;

// Project-dependent implementation
extern int wgettextwidth_compensate(const char *tex, int font);

// Break up the text into lines
void split_lines(const char *todis, int wii, int fonnt) {
    // v2.56.636: rewrote this function because the old version
    // was crap and buggy
    int i = 0;
    int nextCharWas;
    int splitAt;
    char *theline;
    // make a copy, since we change characters in the original string
    // and this might be in a read-only bit of memory
    char textCopyBuffer[STD_BUFFER_SIZE];
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

//=============================================================================
// FIXME: remove later when arrays of chars are replaced by string class
void fputstring(const char *sss, Common::Stream *out)
{
    int b = 0;
    while (sss[b] != 0) {
        out->WriteInt8(sss[b]);
        b++;
    }
    out->WriteInt8(0);
}

void fgetstring_limit(char *sss, Common::Stream *in, int bufsize)
{
    int b = -1;
    do {
        if (b < bufsize - 1)
            b++;
        sss[b] = in->ReadInt8();
        if (in->EOS())
            return;
    } while (sss[b] != 0);
}

void fgetstring(char *sss, Common::Stream *in)
{
    fgetstring_limit (sss, in, 50000000);
}
