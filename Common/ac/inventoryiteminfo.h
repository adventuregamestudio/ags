#ifndef __AC_INVENTORYITEMINFO_H
#define __AC_INVENTORYITEMINFO_H

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

#define IFLG_STARTWITH 1
struct InventoryItemInfo {
    char name[25];
    int  pic;
    int  cursorPic, hotx, hoty;
    int  reserved[5];
    char flags;

    void ReadFromFile(Common::CDataStream *in);
    void WriteToFile(Common::CDataStream *out);
};

#endif // __AC_INVENTORYITEMINFO_H