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

#include <map>

#include "ac/common.h"
#include "debug/out.h"
#include "game/savegame_components.h"

namespace AGS
{
namespace Engine
{

using namespace Common;

namespace SavegameComponents
{

void WriteFormatTag(PStream out, const String &tag, bool open)
{
    String full_tag = String::FromFormat(open ? "<%s>" : "</%s>", tag.GetCStr());
    out->Write(full_tag, full_tag.GetLength());
}

bool ReadFormatTag(PStream in, String &tag, bool open)
{
    if (in->ReadByte() != '<')
        return false;
    if (!open && in->ReadByte() != '/')
        return false;
    tag.Empty();
    while (!in->EOS())
    {
        char c = in->ReadByte();
        if (c == '>')
            return true;
        tag.AppendChar(c);
    }
    return false; // reached EOS before closing symbol
}

bool AssertFormatTag(PStream in, const String &tag, bool open)
{
    String read_tag;
    if (!ReadFormatTag(in, read_tag, open))
        return false;
    return read_tag.Compare(tag) == 0;
}


// Description of a supported game state serialization component
struct ComponentHandler
{
    String             Name;
    int32_t            Version;
    SavegameError      (*Serialize)  (PStream);
    SavegameError      (*Unserialize)(PStream, int32_t cmp_ver, const PreservedParams&, RestoredData&);
};

// Array of supported components
ComponentHandler ComponentHandlers[] =
{
    { NULL, 0, NULL, NULL } // end of array
};


typedef std::map<String, ComponentHandler> HandlersMap;
void GenerateHandlersMap(HandlersMap &map)
{
    map.clear();
    for (int i = 0; !ComponentHandlers[i].Name.IsEmpty(); ++i)
        map[ComponentHandlers[i].Name] = ComponentHandlers[i];
}

// A helper struct to pass to (de)serialization handlers
struct SvgCmpReadHelper
{
    SavegameVersion       Version;  // general savegame version
    const PreservedParams &PP;      // previous game state kept for reference
    RestoredData          &RData;   // temporary storage for loaded data, that
                                    // will be applied after loading is done
    // The map of serialization handlers, one per supported component type ID
    HandlersMap            Handlers;

    SvgCmpReadHelper(SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data)
        : Version(svg_version)
        , PP(pp)
        , RData(r_data)
    {
    }
};

// The basic information about deserialized component, used for debugging purposes
struct ComponentInfo
{
    String  Name;
    int32_t Version;
    size_t  Offset;

    ComponentInfo() : Version(-1), Offset(0) {}
};

SavegameError ReadComponent(PStream in, SvgCmpReadHelper &hlp, ComponentInfo &info)
{
    info = ComponentInfo(); // reset in case of early error
    info.Offset = in->GetPosition();
    if (!ReadFormatTag(in, info.Name, true))
        return kSvgErr_ComponentOpeningTagMismatch;
    info.Version = in->ReadInt32();

    const ComponentHandler *handler = NULL;
    std::map<String, ComponentHandler>::const_iterator it_hdr = hlp.Handlers.find(info.Name);
    if (it_hdr != hlp.Handlers.end())
        handler = &it_hdr->second;

    if (!handler || !handler->Unserialize)
        return kSvgErr_UnsupportedComponent;
    if (info.Version > handler->Version)
        return kSvgErr_UnsupportedComponentVersion;
    SavegameError err = handler->Unserialize(in, info.Version, hlp.PP, hlp.RData);
    if (err != kSvgErr_NoError)
        if (!AssertFormatTag(in, info.Name, false))
            err = kSvgErr_ComponentClosingTagMismatch;
    return err;
}

SavegameError ReadAll(PStream in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data)
{
    // Prepare a helper struct we will be passing to the block reading proc
    SvgCmpReadHelper hlp(svg_version, pp, r_data);
    GenerateHandlersMap(hlp.Handlers);

    while (!in->EOS())
    {
        ComponentInfo info;
        SavegameError err = ReadComponent(in, hlp, info);
        if (err != kSvgErr_NoError)
        {
            Debug::Printf(kDbgMsg_Error, "ERROR: failed to read save block: type = %s, version = %i, at offset = %u",
                info.Name.GetCStr(), info.Version, info.Offset);
            return err;
        }
        update_polled_stuff_if_runtime();
    }
    return kSvgErr_NoError;
}

SavegameError WriteComponent(PStream out, ComponentHandler &hdlr)
{
    WriteFormatTag(out, hdlr.Name, true);
    out->WriteInt32(hdlr.Version);
    SavegameError err = hdlr.Serialize(out);
    if (err != kSvgErr_NoError)
        WriteFormatTag(out, hdlr.Name, false);
    return err;
}

SavegameError WriteAllCommon(PStream out)
{
    SavegameError svg_err;
    for (int type = 0; !ComponentHandlers[type].Name.IsEmpty(); ++type)
    {
        SavegameError err = WriteComponent(out, ComponentHandlers[type]);
        if (svg_err != kSvgErr_NoError)
        {
            Debug::Printf(kDbgMsg_Error, "ERROR: failed to write save block: type = %s", ComponentHandlers[type].Name.GetCStr());
            return svg_err;
        }
        update_polled_stuff_if_runtime();
    }
    return kSvgErr_NoError;
}

} // namespace SavegameBlocks
} // namespace Engine
} // namespace AGS
