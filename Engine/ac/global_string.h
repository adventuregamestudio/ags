
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALSTRING_H
#define __AGS_EE_AC__GLOBALSTRING_H

int StrGetCharAt (char *strin, int posn);
void StrSetCharAt (char *strin, int posn, int nchar);
void _sc_strcat(char*s1,char*s2);
void _sc_strcpy(char*s1,char*s2);
void _sc_strlower (char *desbuf);
void _sc_strupper (char *desbuf);
void _sc_sprintf(char*destt,char*texx, ...);

#endif // __AGS_EE_AC__GLOBALSTRING_H
