#ifndef __AC_MOUSECURSOR_H
#define __AC_MOUSECURSOR_H

#define MCF_ANIMMOVE 1
#define MCF_DISABLED 2
#define MCF_STANDARD 4
#define MCF_HOTSPOT  8  // only animate when over hotspot
// this struct is also in the plugin header file
struct MouseCursor {
    int   pic;
    short hotx, hoty;
    short view;
    char  name[10];
    char  flags;
    MouseCursor();

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp);
#endif
};

#endif // __AC_MOUSECURSOR_H