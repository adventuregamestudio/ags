//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// 'C'-style script compiler
//
//=============================================================================

#ifndef __CC_SCRIPT_H
#define __CC_SCRIPT_H

#include "util/file.h"

namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

struct ccScript
{
public:
    char *globaldata;
    long globaldatasize;
    long *code;
    long codesize;
    char *strings;
    long stringssize;
    char *fixuptypes;             // global data/string area/ etc
    long *fixups;                 // code array index to fixup (in longs)
    int numfixups;
    int importsCapacity;
    char **imports;
    int numimports;
    int exportsCapacity;
    char **exports;   // names of exports
    long *export_addr;        // high byte is type; low 24-bits are offset
    int numexports;
    int instances;
    // 'sections' allow the interpreter to find out which bit
    // of the code came from header files, and which from the main file
    char **sectionNames;
    long *sectionOffsets;
    int numSections;
    int capacitySections;

    static ccScript *CreateFromStream(Common::DataStream *in);

    ccScript();
    virtual ~ccScript(); // there are few derived classes, so dtor should be virtual
    // write the script to disk (after compiling)
    void        Write(Common::DataStream *out);
    // read back a script written with Write
    bool        Read(Common::DataStream *in);
    const char* GetSectionName(long offset);

protected:
    // free the memory occupied by the script - do NOT attempt to run the
    // script after calling this function
    void        Free();
};

#endif // __CC_SCRIPT_H
