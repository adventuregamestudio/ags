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
#ifndef __AGS_TOOL_SPRITEPAK__COMMANDS_H
#define __AGS_TOOL_SPRITEPAK__COMMANDS_H

#include "ac/spritefile.h"
#include "util/string.h"

namespace SpritePak
{
    using String = AGS::Common::String;
    using SpriteStorage = AGS::Common::SpriteStorage;
    using SpriteCompression = AGS::Common::SpriteCompression;

    struct CommandOptions
    {
        String IndexFile;
        String OutIndexFile;
        String ImageFilePattern;
        SpriteStorage StorageFlags = AGS::Common::kSprStore_OptimizeForSize;
        SpriteCompression Compress = AGS::Common::kSprCompress_Deflate;
    };

    String GetCompressionName(SpriteCompression compress);
    SpriteCompression CompressionFromName(const String &compress_name);

    void Init();
    int Command_Create(const String &src_dir, const String &dst_pak, const CommandOptions &opts, bool verbose);
    int Command_Export(const String &src_pak, const String &dst_dir, const CommandOptions &opts, bool verbose);
    int Command_Info(const String &src_pak, const CommandOptions &opts);
    int Command_List(const String &src_pak, const CommandOptions &opts);
    int Command_Copy(const String &src_pak, const String &dst_pak, const CommandOptions &opts, bool verbose);
}

#endif // __AGS_TOOL_SPRITEPAK__COMMANDS_H
