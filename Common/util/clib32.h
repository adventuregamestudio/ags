
#ifndef __CLIB32_H
#define __CLIB32_H

#include "util/file.h"

namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

extern "C"
{
    int csetlib(char *namm, char *passw);
    char *clibgetdatafile(char *fill);
    Common::DataStream *clibfopen(const char *filnamm,
        Common::FileOpenMode open_mode = Common::kFile_Open,
        Common::FileWorkMode work_mode = Common::kFile_Read);
    long cliboffset(const char *);
    long clibfilesize(const char *);
    extern long last_opened_size;

    int clibGetNumFiles();
    const char *clibGetFileName(int index);
    const char *clibgetoriginalfilename();

    extern int cfopenpriority;
};

#endif // __CLIB32_H
