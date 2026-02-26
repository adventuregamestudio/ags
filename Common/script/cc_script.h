//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Compiled script object. A result of AGS script compilation,
// loaded and run by the engine. A single game may have multiple scripts.
//
//=============================================================================
#ifndef __CC_SCRIPT_H
#define __CC_SCRIPT_H

#include <memory>
#include <string>
#include <vector>
#include "platform/types.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct ccScript
{
public:
    static const std::string noname;
    static const std::string unknownSectionName;

    std::string scriptname;
    std::vector<char> globaldata;
    std::vector<int32_t> code;    // executable byte-code, 32-bit per op or arg
    std::vector<char> strings;
    std::vector<char> fixuptypes; // global data/string area/ etc
    std::vector<int32_t> fixups;  // code array index to fixup (in ints)
    std::vector<std::string> imports; // names of imports
    std::vector<std::string> exports; // names of exports
    std::vector<int32_t> export_addr; // export addresses: high byte is type; low 24-bits are offset
    // 'sections' allow the interpreter to find out which bit
    // of the code came from header files, and which from the main file
    std::vector<std::string> sectionNames;
    std::vector<int32_t> sectionOffsets;
    // Extended information

    int instances = 0; // reference count for this script object

    static ccScript *CreateFromStream(Common::Stream *in);
    static ccScript *CreateFromStream(const std::string &name, Common::Stream *in);

    ccScript() = default;
    ccScript(const std::string &name);
    ccScript(const ccScript &src);
    virtual ~ccScript() = default; // there are few derived classes, so dtor should be virtual

    ccScript &operator =(const ccScript&);

    const std::string &GetScriptName() const;
    const std::string &GetSectionName(int32_t offset) const;

    void        SetScriptName(const std::string &name);

    // write the script to disk (after compiling)
    void        Write(Common::Stream *out);
    // read back a script written with Write
    bool        Read(Common::Stream *in);
};

typedef std::shared_ptr<ccScript> PScript;

#endif // __CC_SCRIPT_H
