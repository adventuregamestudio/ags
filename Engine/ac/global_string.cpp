
#include <string.h>
#include "ac/common.h"
#include "ac/global_string.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"

extern int MAXSTRLEN;

int StrGetCharAt (char *strin, int posn) {
    if ((posn < 0) || (posn >= (int)strlen(strin)))
        return 0;
    return strin[posn];
}

void StrSetCharAt (char *strin, int posn, int nchar) {
    if ((posn < 0) || (posn > (int)strlen(strin)) || (posn >= MAX_MAXSTRLEN))
        quit("!StrSetCharAt: tried to write past end of string");

    if (posn == (int)strlen(strin))
        strin[posn+1] = 0;
    strin[posn] = nchar;
}

void _sc_strcat(char*s1,char*s2) {
    // make sure they don't try to append a char to the string
    VALIDATE_STRING (s2);
    check_strlen(s1);
    int mosttocopy=(MAXSTRLEN-strlen(s1))-1;
    //  int numbf=game.iface[4].numbuttons;
    my_strncpy(&s1[strlen(s1)], s2, mosttocopy);
}

void _sc_strcpy(char*s1,char*s2) {
    check_strlen(s1);
    my_strncpy(s1, s2, MAXSTRLEN - 1);
}

void _sc_strlower (char *desbuf) {
    VALIDATE_STRING(desbuf);
    check_strlen (desbuf);
    strlwr (desbuf);
}

void _sc_strupper (char *desbuf) {
    VALIDATE_STRING(desbuf);
    check_strlen (desbuf);
    strupr (desbuf);
}

/*int _sc_strcmp (char *s1, char *s2) {
return strcmp (get_translation (s1), get_translation(s2));
}

int _sc_stricmp (char *s1, char *s2) {
return stricmp (get_translation (s1), get_translation(s2));
}*/

void _sc_sprintf(char*destt,char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    VALIDATE_STRING(destt);
    check_strlen(destt);
    va_list ap;
    va_start(ap,texx);
    my_sprintf(displbuf, get_translation(texx), ap);
    va_end(ap);

    my_strncpy(destt, displbuf, MAXSTRLEN - 1);
}
