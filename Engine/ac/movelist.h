#ifndef __AC_MOVE_H
#define __AC_MOVE_H

// Forward declaration
namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

#define MAXNEEDSTAGES 40
struct MoveList {
    int   pos[MAXNEEDSTAGES];
    int   numstage;
    fixed xpermove[MAXNEEDSTAGES], ypermove[MAXNEEDSTAGES];
    int   fromx, fromy;
    int   onstage, onpart;
    int   lastx, lasty;
    char  doneflag;
    char  direct;  // MoveCharDirect was used or not

    void ReadFromFile(Common::CDataStream *in);
    void WriteToFile(Common::CDataStream *out);
};
#endif // __AC_MOVE_H