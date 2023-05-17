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
#include "ac/dynobj/cc_agsdynamicobject.h"
#include <string.h> // memcpy
#include "util/memorystream.h"

using namespace AGS::Common;

int CCBasicObject::Dispose(const char* /*address*/, bool /*force*/)
{
    return 0; // cannot be removed from memory
}

int CCBasicObject::Serialize(const char* /*address*/, char* /*buffer*/, int /*bufsize*/)
{
    return 0; // does not save data
}

const char* CCBasicObject::GetFieldPtr(const char *address, intptr_t offset)
{
    return address + offset;
}

void CCBasicObject::Read(const char *address, intptr_t offset, void *dest, int size)
{
    memcpy(dest, address + offset, size);
}

uint8_t CCBasicObject::ReadInt8(const char *address, intptr_t offset)
{
    return *(uint8_t*)(address + offset);
}

int16_t CCBasicObject::ReadInt16(const char *address, intptr_t offset)
{
    return *(int16_t*)(address + offset);
}

int32_t CCBasicObject::ReadInt32(const char *address, intptr_t offset)
{
    return *(int32_t*)(address + offset);
}

float CCBasicObject::ReadFloat(const char *address, intptr_t offset)
{
    return *(float*)(address + offset);
}

void CCBasicObject::Write(const char *address, intptr_t offset, void *src, int size)
{
    memcpy((void*)(address + offset), src, size);
}

void CCBasicObject::WriteInt8(const char *address, intptr_t offset, uint8_t val)
{
    *(uint8_t*)(address + offset) = val;
}

void CCBasicObject::WriteInt16(const char *address, intptr_t offset, int16_t val)
{
    *(int16_t*)(address + offset) = val;
}

void CCBasicObject::WriteInt32(const char *address, intptr_t offset, int32_t val)
{
    *(int32_t*)(address + offset) = val;
}

void CCBasicObject::WriteFloat(const char *address, intptr_t offset, float val)
{
    *(float*)(address + offset) = val;
}


int AGSCCDynamicObject::Serialize(const char *address, char *buffer, int bufsize) {
    // If the required space is larger than the provided buffer,
    // then return negated required space, notifying the caller that a larger buffer is necessary
    size_t req_size = CalcSerializeSize(address);
    if (bufsize < 0 || req_size > static_cast<size_t>(bufsize))
        return -(static_cast<int32_t>(req_size));

    MemoryStream mems(reinterpret_cast<uint8_t*>(buffer), bufsize, kStream_Write);
    Serialize(address, &mems);
    return static_cast<int32_t>(mems.GetPosition());
}


AGSCCStaticObject GlobalStaticManager;
