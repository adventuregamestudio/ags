
#include "ac/string.h"
#include "util/wgt2allg.h"  //strupr and strlwr on Linux
#include "ac/common.h"
#include "ac/display.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptstring.h"
#include "debug/debug.h"
#include "util/string_utils.h"
#include "platform/bigend.h"    //stricmp()

extern char lines[MAXLINE][200];
extern int  numlines;
extern GameSetupStruct game;
extern GameState play;
extern int longestline;

#ifdef WINDOWS_VERSION
#define strlwr _strlwr
#define strupr _strupr
#endif

int String_IsNullOrEmpty(const char *thisString) 
{
    if ((thisString == NULL) || (thisString[0] == 0))
        return 1;

    return 0;
}

const char* String_Copy(const char *srcString) {
    return CreateNewScriptString(srcString);
}

const char* String_Append(const char *thisString, const char *extrabit) {
    char *buffer = (char*)malloc(strlen(thisString) + strlen(extrabit) + 1);
    strcpy(buffer, thisString);
    strcat(buffer, extrabit);
    return CreateNewScriptString(buffer, false);
}

const char* String_AppendChar(const char *thisString, char extraOne) {
    char *buffer = (char*)malloc(strlen(thisString) + 2);
    sprintf(buffer, "%s%c", thisString, extraOne);
    return CreateNewScriptString(buffer, false);
}

const char* String_ReplaceCharAt(const char *thisString, int index, char newChar) {
    if ((index < 0) || (index >= (int)strlen(thisString)))
        quit("!String.ReplaceCharAt: index outside range of string");

    char *buffer = (char*)malloc(strlen(thisString) + 1);
    strcpy(buffer, thisString);
    buffer[index] = newChar;
    return CreateNewScriptString(buffer, false);
}

const char* String_Truncate(const char *thisString, int length) {
    if (length < 0)
        quit("!String.Truncate: invalid length");

    if (length >= (int)strlen(thisString))
        return thisString;

    char *buffer = (char*)malloc(length + 1);
    strncpy(buffer, thisString, length);
    buffer[length] = 0;
    return CreateNewScriptString(buffer, false);
}

const char* String_Substring(const char *thisString, int index, int length) {
    if (length < 0)
        quit("!String.Substring: invalid length");
    if ((index < 0) || (index > (int)strlen(thisString)))
        quit("!String.Substring: invalid index");

    char *buffer = (char*)malloc(length + 1);
    strncpy(buffer, &thisString[index], length);
    buffer[length] = 0;
    return CreateNewScriptString(buffer, false);
}

int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive) {

    if (caseSensitive) {
        return strcmp(thisString, otherString);
    }
    else {
        return stricmp(thisString, otherString);
    }
}

int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

    if (caseSensitive) {
        return (strncmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
    }
    else {
        return (strnicmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
    }
}

int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

    int checkAtOffset = strlen(thisString) - strlen(checkForString);

    if (checkAtOffset < 0)
    {
        return 0;
    }

    if (caseSensitive) 
    {
        return (strcmp(&thisString[checkAtOffset], checkForString) == 0) ? 1 : 0;
    }
    else 
    {
        return (stricmp(&thisString[checkAtOffset], checkForString) == 0) ? 1 : 0;
    }
}

const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
{
    char resultBuffer[STD_BUFFER_SIZE] = "";
    int thisStringLen = (int)strlen(thisString);
    int outputSize = 0;
    for (int i = 0; i < thisStringLen; i++)
    {
        bool matchHere = false;
        if (caseSensitive)
        {
            matchHere = (strncmp(&thisString[i], lookForText, strlen(lookForText)) == 0);
        }
        else
        {
            matchHere = (strnicmp(&thisString[i], lookForText, strlen(lookForText)) == 0);
        }

        if (matchHere)
        {
            strcpy(&resultBuffer[outputSize], replaceWithText);
            outputSize += strlen(replaceWithText);
            i += strlen(lookForText) - 1;
        }
        else
        {
            resultBuffer[outputSize] = thisString[i];
            outputSize++;
        }
    }

    resultBuffer[outputSize] = 0;

    return CreateNewScriptString(resultBuffer, true);
}

const char* String_LowerCase(const char *thisString) {
    char *buffer = (char*)malloc(strlen(thisString) + 1);
    strcpy(buffer, thisString);
    strlwr(buffer);
    return CreateNewScriptString(buffer, false);
}

const char* String_UpperCase(const char *thisString) {
    char *buffer = (char*)malloc(strlen(thisString) + 1);
    strcpy(buffer, thisString);
    strupr(buffer);
    return CreateNewScriptString(buffer, false);
}

const char* String_Format(const char *texx, ...) {
    char displbuf[STD_BUFFER_SIZE];

    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    return CreateNewScriptString(displbuf);
}

int String_GetChars(const char *texx, int index) {
    if ((index < 0) || (index >= (int)strlen(texx)))
        return 0;
    return texx[index];
}

int StringToInt(char*stino) {
    return atoi(stino);
}

int StrContains (const char *s1, const char *s2) {
    VALIDATE_STRING (s1);
    VALIDATE_STRING (s2);
    char *tempbuf1 = (char*)malloc(strlen(s1) + 1);
    char *tempbuf2 = (char*)malloc(strlen(s2) + 1);
    strcpy(tempbuf1, s1);
    strcpy(tempbuf2, s2);
    strlwr(tempbuf1);
    strlwr(tempbuf2);

    char *offs = strstr (tempbuf1, tempbuf2);
    free(tempbuf1);
    free(tempbuf2);

    if (offs == NULL)
        return -1;

    return (offs - tempbuf1);
}

//=============================================================================

const char *CreateNewScriptString(const char *fromText, bool reAllocate) {
    ScriptString *str;
    if (reAllocate) {
        str = new ScriptString(fromText);
    }
    else {
        str = new ScriptString();
        str->text = (char*)fromText;
    }

    ccRegisterManagedObject(str->text, str);

    /*long handle = ccRegisterManagedObject(str->text, str);
    char buffer[1000];
    sprintf(buffer, "String %p (handle %d) allocated: '%s'", str->text, handle, str->text);
    write_log(buffer);*/

    return str->text;
}

void split_lines_rightleft (char *todis, int wii, int fonnt) {
    // start on the last character
    char *thisline = todis + strlen(todis) - 1;
    char prevlwas, *prevline = NULL;
    // work backwards
    while (thisline >= todis) {

        int needBreak = 0;
        if (thisline <= todis) 
            needBreak = 1;
        // ignore \[ sequence
        else if ((thisline > todis) && (thisline[-1] == '\\')) { }
        else if (thisline[0] == '[') {
            needBreak = 1;
            thisline++;
        }
        else if (wgettextwidth_compensate(thisline, fonnt) >= wii) {
            // go 'back' to the nearest word
            while ((thisline[0] != ' ') && (thisline[0] != 0))
                thisline++;

            if (thisline[0] == 0)
                quit("!Single word too wide for window");

            thisline++;
            needBreak = 1;
        }

        if (needBreak) {
            strcpy(lines[numlines], thisline);
            removeBackslashBracket(lines[numlines]);
            numlines++;
            if (prevline) {
                prevline[0] = prevlwas;
            }
            thisline--;
            prevline = thisline;
            prevlwas = prevline[0];
            prevline[0] = 0;
        }

        thisline--;
    }
    if (prevline)
        prevline[0] = prevlwas;
}

char *reverse_text(char *text) {
    int stlen = strlen(text), rr;
    char *backwards = (char*)malloc(stlen + 1);
    for (rr = 0; rr < stlen; rr++)
        backwards[rr] = text[(stlen - rr) - 1];
    backwards[stlen] = 0;
    return backwards;
}

void wouttext_reverseifnecessary(int x, int y, int font, char *text) {
    char *backwards = NULL;
    char *otext = text;
    if (game.options[OPT_RIGHTLEFTWRITE]) {
        backwards = reverse_text(text);
        otext = backwards;
    }

    wouttext_outline(x, y, font, otext);

    if (backwards)
        free(backwards);
}

void break_up_text_into_lines(int wii,int fonnt,char*todis) {
    if (fonnt == -1)
        fonnt = play.normal_font;

    //  char sofar[100];
    if (todis[0]=='&') {
        while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
        if (todis[0]==' ') todis++;
    }
    numlines=0;
    longestline=0;

    // Don't attempt to display anything if the width is tiny
    if (wii < 3)
        return;

    int rr;

    if (game.options[OPT_RIGHTLEFTWRITE] == 0)
    {
        split_lines_leftright(todis, wii, fonnt);
    }
    else {
        // Right-to-left just means reverse the text then
        // write it as normal
        char *backwards = reverse_text(todis);
        split_lines_rightleft (backwards, wii, fonnt);
        free(backwards);
    }

    for (rr=0;rr<numlines;rr++) {
        if (wgettextwidth_compensate(lines[rr],fonnt) > longestline)
            longestline = wgettextwidth_compensate(lines[rr],fonnt);
    }
}

int MAXSTRLEN = MAX_MAXSTRLEN;
void check_strlen(char*ptt) {
    MAXSTRLEN = MAX_MAXSTRLEN;
    long charstart = (long)&game.chars[0];
    long charend = charstart + sizeof(CharacterInfo)*game.numcharacters;
    if (((long)&ptt[0] >= charstart) && ((long)&ptt[0] <= charend))
        MAXSTRLEN=30;
}

/*void GetLanguageString(int indxx,char*buffr) {
VALIDATE_STRING(buffr);
char*bptr=get_language_text(indxx);
if (bptr==NULL) strcpy(buffr,"[language string error]");
else strncpy(buffr,bptr,199);
buffr[199]=0;
}*/

void my_strncpy(char *dest, const char *src, int len) {
    // the normal strncpy pads out the string with zeros up to the
    // max length -- we don't want that
    if (strlen(src) >= (unsigned)len) {
        strncpy(dest, src, len);
        dest[len] = 0;
    }
    else
        strcpy(dest, src);
}

// Custom printf, needed because floats are pushed as 8 bytes
void my_sprintf(char *buffer, const char *fmt, va_list ap) {
    int bufidx = 0;
    const char *curptr = fmt;
    const char *endptr;
    char spfbuffer[STD_BUFFER_SIZE];
    char fmtstring[100];
    int numargs = -1;

    while (1) {
        // copy across everything until the next % (or end of string)
        endptr = strchr(curptr, '%');
        if (endptr == NULL)
            endptr = &curptr[strlen(curptr)];
        while (curptr < endptr) {
            buffer[bufidx] = *curptr;
            curptr++;
            bufidx++;
        }
        // at this point, curptr and endptr should be equal and pointing
        // to the % or \0
        if (*curptr == 0)
            break;
        if (curptr[1] == '%') {
            // "%%", so just write a % to the output
            buffer[bufidx] = '%';
            bufidx++;
            curptr += 2;
            continue;
        }
        // find the end of the % clause
        while ((*endptr != 'd') && (*endptr != 'f') && (*endptr != 'c') &&
            (*endptr != 0) && (*endptr != 's') && (*endptr != 'x') &&
            (*endptr != 'X'))
            endptr++;

        if (numargs >= 0) {
            numargs--;
            // if there are not enough arguments, just copy the %d
            // to the output string rather than trying to format it
            if (numargs < 0)
                endptr = &curptr[strlen(curptr)];
        }

        if (*endptr == 0) {
            // something like %p which we don't support, so just write
            // the % to the output
            buffer[bufidx] = '%';
            bufidx++;
            curptr++;
            continue;
        }
        // move endptr to 1 after the end character
        endptr++;

        // copy the %d or whatever
        strncpy(fmtstring, curptr, (endptr - curptr));
        fmtstring[endptr - curptr] = 0;

        unsigned int theArg = va_arg(ap, unsigned int);

        // use sprintf to parse the actual %02d type thing
        if (endptr[-1] == 'f') {
            // floats are pushed as 8-bytes, so ensure that it knows this is a float
            float floatArg;
            memcpy(&floatArg, &theArg, sizeof(float));
            sprintf(spfbuffer, fmtstring, floatArg);
        }
        else if ((theArg == (int)buffer) && (endptr[-1] == 's'))
            quit("Cannot use destination as argument to StrFormat");
        else if ((theArg < 0x10000) && (endptr[-1] == 's'))
            quit("!One of the string arguments supplied was not a string");
        else if (endptr[-1] == 's')
        {
            strncpy(spfbuffer, (const char*)theArg, STD_BUFFER_SIZE);
            spfbuffer[STD_BUFFER_SIZE - 1] = 0;
        }
        else 
            sprintf(spfbuffer, fmtstring, theArg);

        // use the formatted text
        buffer[bufidx] = 0;

        if (bufidx + strlen(spfbuffer) >= STD_BUFFER_SIZE)
            quitprintf("!String.Format: buffer overrun: maximum formatted string length %d chars, this string: %d chars", STD_BUFFER_SIZE, bufidx + strlen(spfbuffer));

        strcat(buffer, spfbuffer);
        bufidx += strlen(spfbuffer);
        curptr = endptr;
    }
    buffer[bufidx] = 0;
}
