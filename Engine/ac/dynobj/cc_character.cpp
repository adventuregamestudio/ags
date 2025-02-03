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
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/characterinfo.h"
#include "ac/gamesetupstruct.h"
#include "script/cc_common.h" // cc_error
#include "script/cc_common.h"
#include "util/stream.h"

using namespace AGS::Common;
extern std::vector<int> StaticCharacterArray;

extern GameSetupStruct game;

// return the type name of the object
const char *CCCharacter::GetType()
{
    return "Character";
}

size_t CCCharacter::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void CCCharacter::Serialize(const void *address, Stream *out)
{
    const CharacterInfo *chaa = static_cast<const CharacterInfo*>(address);
    out->WriteInt32(chaa->index_id);
}

void CCCharacter::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    int num = in->ReadInt32();
    StaticCharacterArray[num] = ccRegisterUnserializedPersistentObject(index, &game.chars[num], this);
}

uint8_t CCCharacter::ReadInt8(const void *address, intptr_t offset)
{
    // The only supported variable remaining in 3.4.*
    const CharacterInfo *ci = static_cast<const CharacterInfo*>(address);
    const int on_offset = 28 * sizeof(int32_t) /* first var group */
        + 301 * sizeof(int16_t) /* inventory */ + sizeof(int16_t) * 2 /* two shorts */ + 40 /* name */ + 20 /* scrname */;
    if (offset == on_offset)
        return (ci->flags & CHF_ENABLED) != 0;
    cc_error("ScriptCharacter: unsupported 'char' variable offset %d", offset);
    return 0;
}

void CCCharacter::WriteInt8(void *address, intptr_t offset, uint8_t val)
{
    // The only supported variable remaining in 3.4.*
    CharacterInfo *ci = static_cast<CharacterInfo*>(address);
    const int on_offset = 28 * sizeof(int32_t) /* first var group */
        + 301 * sizeof(int16_t) /* inventory */ + sizeof(int16_t) * 2 /* two shorts */ + 40 /* name */ + 20 /* scrname */;
    if (offset == on_offset)
        ci->flags = (ci->flags & ~CHF_ENABLED) | ((CHF_ENABLED | CHF_VISIBLE) * val);
    else
        cc_error("ScriptCharacter: unsupported 'char' variable offset %d", offset);
}

int16_t CCCharacter::ReadInt16(const void *address, intptr_t offset)
{
    cc_error("ScriptCharacter: unsupported 'short' variable offset %d", offset);
    return 0;
}

void CCCharacter::WriteInt16(void *address, intptr_t offset, int16_t val)
{
    cc_error("ScriptCharacter: unsupported 'short' variable offset %d", offset);
}

int32_t CCCharacter::ReadInt32(const void *address, intptr_t offset)
{
    cc_error("ScriptCharacter: unsupported 'int' variable offset %d", offset);
    return 0;
}

void CCCharacter::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    cc_error("ScriptCharacter: unsupported 'int' variable offset %d", offset);
}
