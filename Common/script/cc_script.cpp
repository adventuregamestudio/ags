//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdlib.h>
#include <string.h>
#include "script/cc_common.h"
#include "script/cc_internal.h"
#include "script/cc_script.h"
#include "util/stream.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;

// currently executed line
int currentline;
// script file format signature
const char scfilesig[5] = "SCOM";


ccScript *ccScript::CreateFromStream(Stream *in)
{
    ccScript *scri = new ccScript();
    if (!scri->Read(in))
    {
        delete scri;
        return nullptr;
    }
    return scri;
}

ccScript::ccScript(const ccScript &src)
{
    globaldata = src.globaldata;
    code = src.code;
    strings = src.strings;
    fixuptypes = src.fixuptypes;
    fixups = src.fixups;

    imports = src.imports;
    exports = src.exports;
    export_addr = src.export_addr;

    sectionNames = src.sectionNames;
    sectionOffsets = src.sectionOffsets;

    instances = 0; // don't copy reference count, since it's a new object
}

void ccScript::Write(Stream *out)
{
    out->Write(scfilesig,4);
    out->WriteInt32(SCOM_VERSION_CURRENT);
    out->WriteInt32(globaldata.size());
    out->WriteInt32(code.size());
    out->WriteInt32(strings.size());
    if (globaldata.size() > 0)
        out->Write(&globaldata.front(), globaldata.size());
    if (code.size() > 0)
        out->WriteArrayOfInt32(&code.front(), code.size());
    if (strings.size() > 0)
        out->Write(&strings.front(), strings.size());
    out->WriteInt32(fixups.size());
    if (fixups.size() > 0)
    {
        out->Write(&fixuptypes.front(), fixups.size());
        out->WriteArrayOfInt32(&fixups.front(), fixups.size());
    }
    out->WriteInt32(imports.size());
    for (size_t n = 0; n < imports.size(); ++n)
        StrUtil::WriteCStr(imports[n].c_str(), out);
    out->WriteInt32(exports.size());
    for (size_t n = 0; n < exports.size(); ++n)
    {
        StrUtil::WriteCStr(exports[n].c_str(), out);
        out->WriteInt32(export_addr[n]);
    }
    out->WriteInt32(sectionNames.size());
    for (size_t n = 0; n < sectionNames.size(); ++n)
    {
        StrUtil::WriteCStr(sectionNames[n].c_str(), out);
        out->WriteInt32(sectionOffsets[n]);
    }
    out->WriteInt32(ENDFILESIG);
}

bool ccScript::Read(Stream *in)
{
    instances = 0;
    currentline = -1;

    char gotsig[5]{};
    in->Read(gotsig, 4);

    int fileVer = in->ReadInt32();
    if ((strcmp(gotsig, scfilesig) != 0) || (fileVer > SCOM_VERSION_CURRENT))
    {
        cc_error("file was not written by ccScript::Write or seek position is incorrect");
        return false;
    }

    const uint32_t globaldatasize = in->ReadInt32();
    const uint32_t codesize = in->ReadInt32();
    const uint32_t stringssize = in->ReadInt32();

    globaldata.resize(globaldatasize);
    if (globaldatasize > 0)
    {
        in->Read(&globaldata.front(), globaldata.size());
    }

    code.resize(codesize);
    if (codesize > 0)
    {
        in->ReadArrayOfInt32(&code.front(), code.size());
    }

    strings.resize(stringssize);
    if (strings.size() > 0)
    {
        in->Read(&strings.front(), strings.size());
    }

    const uint32_t numfixups = in->ReadInt32();
    fixuptypes.resize(numfixups);
    fixups.resize(numfixups);
    if (numfixups > 0)
    {
        in->Read(&fixuptypes.front(), numfixups);
        in->ReadArrayOfInt32(&fixups.front(), numfixups);
    }

    const uint32_t numimports = in->ReadInt32();
    imports.resize(numimports);
    for (uint32_t n = 0; n < numimports; ++n)
    {
        imports[n] = StrUtil::ReadCStrAsStdString(in);
    }

    const uint32_t numexports = in->ReadInt32();
    exports.resize(numexports);
    export_addr.resize(numexports);
    for (uint32_t n = 0; n < numexports; ++n)
    {
        exports[n] = StrUtil::ReadCStrAsStdString(in);
        export_addr[n] = in->ReadInt32();
    }

    if (fileVer >= SCOM_VERSION_SECTIONS)
    {
        // read in the Sections
        const uint32_t numsections = in->ReadInt32();
        sectionNames.resize(numsections);
        sectionOffsets.resize(numsections);
        for (uint32_t n = 0; n < numsections; ++n)
        {
            sectionNames[n] = StrUtil::ReadCStrAsStdString(in);
            sectionOffsets[n] = in->ReadInt32();
        }
    }

    if (static_cast<uint32_t>(in->ReadInt32()) != ENDFILESIG)
    {
        cc_error("!internal error reading script: end file signature not found");
        return false;
    }
    return true;
}

const char* ccScript::GetSectionName(int32_t offs) const
{
    size_t sect_idx = 0;
    for (; sect_idx < sectionOffsets.size(); ++sect_idx)
    {
        if (sectionOffsets[sect_idx] < offs)
            continue;
        break;
    }

    // if no sections in script, return unknown
    if (sect_idx == 0)
        return "(unknown section)";

    return sectionNames[sect_idx - 1].c_str();
}
