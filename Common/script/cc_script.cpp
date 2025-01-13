//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
#include "util/data_ext.h"
#include "util/stream.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;

// script file format signature
const char scfilesig[5] = "SCOM";

const std::string ccScript::noname = "";
// FIXME: this constant should not be in ccScript, but in whoever uses its section names?
const std::string ccScript::unknownSectionName = "(unknown section)";

// ScriptExtReader reads script data's extension blocks
class ScriptExtReader : public DataExtReader
{
public:
    ScriptExtReader(ccScript &script, std::unique_ptr<Stream> &&in)
        : DataExtReader(std::move(in), kDataExt_NumID8 | kDataExt_File64)
        , _script(script)
        {}

protected:
    HError ReadBlock(Stream *in, int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override
    {
        read_next = true;
        if (ext_id.CompareNoCase("rtti") == 0)
        {
            _script.rtti.reset(new RTTI(std::move(RTTISerializer::Read(in))));
            return HError::None();
        }
        else if (ext_id.CompareNoCase("sctoc") == 0)
        {
            _script.sctoc.reset(new ScriptTOC(std::move(ScriptTOCSerializer::Read(in, _script.rtti.get() /* may be null */))));
            return HError::None();
        }
        return new Error(String::FromFormat("Unknown script extension: %s (%d)", ext_id.GetCStr(), block_id));
    }

    ccScript &_script;
};



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

ccScript *ccScript::CreateFromStream(const std::string &name, Common::Stream *in)
{
    ccScript *scri = new ccScript(name);
    if (!scri->Read(in))
    {
        delete scri;
        return nullptr;
    }
    return scri;
}

ccScript::ccScript(const std::string &name)
{
    scriptname = name;
}

ccScript::ccScript(const ccScript &src)
{
    *this = src;
}

ccScript &ccScript::operator =(const ccScript &src)
{
    scriptname = src.scriptname;
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
    rtti.reset(new RTTI(*src.rtti));
    return *this;
}

void ccScript::Write(Stream *out)
{
    out->Write(scfilesig,4);
    out->WriteInt32(SCOM_VERSION_CURRENT);
    out->WriteInt32(globaldata.size());
    out->WriteInt32(code.size());
    out->WriteInt32(strings.size());
    if (globaldata.size() > 0)
        out->Write(globaldata.data(), globaldata.size());
    if (code.size() > 0)
        out->WriteArrayOfInt32(code.data(), code.size());
    if (strings.size() > 0)
        out->Write(strings.data(), strings.size());
    out->WriteInt32(fixups.size());
    if (fixups.size() > 0)
    {
        out->Write(fixuptypes.data(), fixups.size());
        out->WriteArrayOfInt32(fixups.data(), fixups.size());
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

    //-------------------------------------------------------------------------
    // Extended data
    //-------------------------------------------------------------------------
    const auto *rtti = ccScript::rtti.get();
    if (rtti && !rtti->IsEmpty())
    {
        WriteExtBlock("rtti", [rtti](Stream *out){ RTTISerializer::Write(*rtti, out); },
                      kDataExt_NumID8 | kDataExt_File64, out);
    }
    const auto *sctoc = ccScript::sctoc.get();
    if (sctoc && !sctoc->IsEmpty())
    {
        WriteExtBlock("sctoc", [sctoc](Stream *out){ ScriptTOCSerializer::Write(*sctoc, out); },
                      kDataExt_NumID8 | kDataExt_File64, out);
    }
    // Write ending
    out->WriteInt8(static_cast<uint8_t>(0xFF));

    out->WriteInt32(ENDFILESIG);
}

bool ccScript::Read(Stream *in)
{
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
        in->Read(globaldata.data(), globaldata.size());
    }

    code.resize(codesize);
    if (codesize > 0)
    {
        in->ReadArrayOfInt32(code.data(), code.size());
    }

    strings.resize(stringssize);
    if (strings.size() > 0)
    {
        in->Read(strings.data(), strings.size());
    }

    const uint32_t numfixups = in->ReadInt32();
    fixuptypes.resize(numfixups);
    fixups.resize(numfixups);
    if (numfixups > 0)
    {
        in->Read(fixuptypes.data(), numfixups);
        in->ReadArrayOfInt32(fixups.data(), numfixups);
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

    //-------------------------------------------------------------------------
    // Extended data
    //-------------------------------------------------------------------------
    if (fileVer >= SCOM_VERSION_EXT) {
        ScriptExtReader reader(*this, std::unique_ptr<Stream>(in));
        HError err = reader.Read();
        reader.ReleaseStream().release(); // FIXME: this double release is ugly
        if (!err) {
            cc_error("!internal error reading script extensions: %s", err->FullMessage().GetCStr());
            return false;
        }
    }

    if (static_cast<uint32_t>(in->ReadInt32()) != ENDFILESIG)
    {
        cc_error("!internal error reading script: end file signature not found");
        return false;
    }
    return true;
}

const std::string &ccScript::GetScriptName() const
{
    if (!scriptname.empty())
        return scriptname;
    // In a regular script sections contain an optional list of headers in an order
    // they were included, and the script body's own name as the last element.
    if (sectionNames.size() > 0)
        return sectionNames.back();
    return noname;
}

const std::string &ccScript::GetSectionName(int32_t offs) const
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
        return unknownSectionName;

    return sectionNames[sect_idx - 1];
}

void ccScript::SetScriptName(const std::string &name)
{
    scriptname = name;
}
