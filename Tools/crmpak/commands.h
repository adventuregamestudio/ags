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
#ifndef __AGS_TOOL_CRMPAK__COMMANDS_H
#define __AGS_TOOL_CRMPAK__COMMANDS_H

#include <vector>
#include "util/string.h"
#include "util/string_types.h"

namespace CRMPak
{
    using String = AGS::Common::String;

    enum ContentType
    {
        kContent_Undefined = 0,
        kContent_Background,
        kContent_Hotspot,
        kContent_Region,
        kContent_WalkArea,
        kContent_WalkBehind,
        kContent_ScriptCompiled3,
        kContent_ScriptText,
        kNumContentTypes
    };

    struct Content
    {
        ContentType Type = kContent_Undefined;
        int Index = -1;
        String FileName;

        Content() = default;
        Content(ContentType type, const String &filename)
            : Type(type), FileName(filename)
        {}
        Content(ContentType type, int index, const String &filename)
            : Type(type), Index(index), FileName(filename)
        {}
    };

    const AGS::Common::CstrArr<kNumContentTypes> &GetContentNames();

    void Init();
    int Command_Create(const String &dst_room, const std::vector<Content> &content, bool verbose);
    int Command_Cut(const String &src_room, const String &dst_room, const std::vector<Content> &content, bool verbose);
    int Command_Export(const String &src_room, const std::vector<Content> &content, bool verbose);
    int Command_Import(const String &src_room, const String &dst_room, const std::vector<Content> &content, bool verbose);
    int Command_List(const String &src_room);
}

#endif // __AGS_TOOL_CRMPAK__COMMANDS_H
