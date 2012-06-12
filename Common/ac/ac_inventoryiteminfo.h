#ifndef __AC_INVENTORYITEMINFO_H
#define __AC_INVENTORYITEMINFO_H

#define IFLG_STARTWITH 1
struct InventoryItemInfo {
    char name[25];
    int  pic;
    int  cursorPic, hotx, hoty;
    int  reserved[5];
    char flags;

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp);
#endif
};

#endif // __AC_INVENTORYITEMINFO_H