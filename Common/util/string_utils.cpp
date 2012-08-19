
#include <string.h>
#include "ac/common.h"
#include "gui/guidefines.h"
#include "util/string_utils.h"

#define STD_BUFFER_SIZE 3000

void removeBackslashBracket(char *lbuffer) {
    char *slashoffs;
    while ((slashoffs = strstr(lbuffer, "\\[")) != NULL) {
        // remove the backslash
        memmove(slashoffs, slashoffs + 1, strlen(slashoffs));
    }
}

extern char lines[MAXLINE][200];
extern int  numlines;

extern int wgettextwidth_compensate(const char *tex, int font);

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

void fputstring(const char *sss, FILE *ddd) {
    int b = 0;
    while (sss[b] != 0) {
        fputc(sss[b], ddd);
        b++;
    }
    fputc(0,ddd);
}

void fgetstring_limit(char *sss, FILE *ddd, int bufsize) {
    int b = -1;
    do {
        if (b < bufsize - 1)
            b++;
        sss[b] = fgetc(ddd);
        if (feof(ddd))
            return;
    } while (sss[b] != 0);
}

void fgetstring(char *sss, FILE *ddd) {
    fgetstring_limit (sss, ddd, 50000000);
}
