#ifndef __AC_STRINGS_H
#define __AC_STRINGS_H

void split_lines_rightleft (char *todis, int wii, int fonnt);
char *reverse_text(char *text);
void wouttext_reverseifnecessary(int x, int y, int font, char *text);
void break_up_text_into_lines(int wii,int fonnt,char*todis);
void check_strlen(char*ptt);

void my_strncpy(char *dest, const char *src, int len);
void _sc_strcat(char*s1,char*s2);
void _sc_strcpy(char*s1,char*s2);
int StrContains (const char *s1, const char *s2);;

void _sc_strlower (char *desbuf);;
void _sc_strupper (char *desbuf);;
void my_sprintf(char *buffer, const char *fmt, va_list ap);;
void _sc_AbortGame(char*texx, ...);;
void _sc_sprintf(char*destt,char*texx, ...);;

#ifdef WINDOWS_VERSION
#define strlwr _strlwr
#define strupr _strupr
#endif


extern int MAXSTRLEN;

#endif // __AC_STRINGS_H

