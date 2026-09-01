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
#ifndef __AGS_TOOL_SPRITEIMPORT__COMMANDS_H
#define __AGS_TOOL_SPRITEIMPORT__COMMANDS_H

#include "data/sprite_utils.h"

namespace SpriteImport
{
    using String = AGS::Common::String;
    using SpriteStorage = AGS::Common::SpriteStorage;
    using SpriteCompression = AGS::Common::SpriteCompression;

    struct CommandOptions
    {
        bool OutputToSpritePak = false;
        String IndexFile;
        String ImageFilePattern;
        SpriteStorage StorageFlags = AGS::Common::kSprStore_OptimizeForSize;
        SpriteCompression Compress = AGS::Common::kSprCompress_Deflate;
        String RoomDirectory;
    };

    void Init();
    int Command_Import(const String &src_agf, const String &dst_path, const CommandOptions &opts, bool verbose);
}

#endif // __AGS_TOOL_SPRITEIMPORT__COMMANDS_H
