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
#include "ac/dynobj/cc_agsdynamicobject.h"
#include <string.h> // memcpy
#include "util/memorystream.h"

using namespace AGS::Common;

int CCBasicObject::Dispose(void* /*address*/, bool /*force*/)
{
    return 0; // cannot be removed from memory
}

int CCBasicObject::Serialize(void* /*address*/, uint8_t* /*buffer*/, int /*bufsize*/)
{
    return 0; // does not save data
}

void *CCBasicObject::GetFieldPtr(void *address, intptr_t offset)
{
    return static_cast<uint8_t*>(address) + offset;
}

void CCBasicObject::Read(void *address, intptr_t offset, uint8_t *dest, size_t size)
{
    memcpy(dest, static_cast<uint8_t*>(address) + offset, size);
}

uint8_t CCBasicObject::ReadInt8(void *address, intptr_t offset)
{
    return *(uint8_t*)(static_cast<uint8_t*>(address) + offset);
}

int16_t CCBasicObject::ReadInt16(void *address, intptr_t offset)
{
    return *(int16_t*)(static_cast<uint8_t*>(address) + offset);
}

int32_t CCBasicObject::ReadInt32(void *address, intptr_t offset)
{
    return *(int32_t*)(static_cast<uint8_t*>(address) + offset);
}

float CCBasicObject::ReadFloat(void *address, intptr_t offset)
{
    return *(float*)(static_cast<uint8_t*>(address) + offset);
}

void CCBasicObject::Write(void *address, intptr_t offset, const uint8_t *src, size_t size)
{
    memcpy(static_cast<uint8_t*>(address) + offset, src, size);
}

void CCBasicObject::WriteInt8(void *address, intptr_t offset, uint8_t val)
{
    *(uint8_t*)(static_cast<uint8_t*>(address) + offset) = val;
}

void CCBasicObject::WriteInt16(void *address, intptr_t offset, int16_t val)
{
    *(int16_t*)(static_cast<uint8_t*>(address) + offset) = val;
}

void CCBasicObject::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    *(int32_t*)(static_cast<uint8_t*>(address) + offset) = val;
}

void CCBasicObject::WriteFloat(void *address, intptr_t offset, float val)
{
    *(float*)(static_cast<uint8_t*>(address) + offset) = val;
}


int AGSCCDynamicObject::Serialize(void *address, uint8_t *buffer, int bufsize) {
    // If the required space is larger than the provided buffer,
    // then return negated required space, notifying the caller that a larger buffer is necessary
    size_t req_size = CalcSerializeSize(address);
    assert(req_size <= INT32_MAX); // dynamic object API does not support size > int32
    if (bufsize < 0 || req_size > static_cast<size_t>(bufsize))
        return -(static_cast<int32_t>(req_size));

    MemoryStream mems(reinterpret_cast<uint8_t*>(buffer), bufsize, kStream_Write);
    Serialize(address, &mems);
    return static_cast<int32_t>(mems.GetPosition());
}


AGSCCStaticObject GlobalStaticManager;
