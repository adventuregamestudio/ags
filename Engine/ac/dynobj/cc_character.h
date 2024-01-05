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
//
// Wrapper around script "Character" struct, managing access to its variables.
// Assumes object data contains CharacterInfo object.
//
//=============================================================================
#ifndef __AC_CCCHARACTER_H
#define __AC_CCCHARACTER_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct CCCharacter final : AGSCCDynamicObject
{
public:
    // return the type name of the object
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    uint8_t ReadInt8(void *address, intptr_t offset) override;
    int16_t ReadInt16(void *address, intptr_t offset) override;
    int32_t ReadInt32(void *address, intptr_t offset) override;
    void WriteInt8(void *address, intptr_t offset, uint8_t val) override;
    void WriteInt16(void *address, intptr_t offset, int16_t val) override;
    void WriteInt32(void *address, intptr_t offset, int32_t val) override;

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

#endif // __AC_CCCHARACTER_H