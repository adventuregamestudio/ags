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

#include "util/stream.h"

namespace AGS
{
namespace Common
{

size_t Stream::ReadArrayOfIntPtr32(intptr_t *buffer, size_t count)
{
    if (!CanRead())
    {
        return 0;
    }

    int32_t *buf_ptr = sizeof(int32_t) == sizeof(intptr_t) ?
        (int32_t*)buffer : new int32_t[count];
    if (!buf_ptr)
    {
        return 0;
    }

    count = ReadArrayOfInt32(buf_ptr, count);

    if (sizeof(int32_t) != sizeof(intptr_t))
    {
        for (size_t i = 0; i < count; ++i)
        {
            buffer[i] = buf_ptr[i];
        }
        delete [] buf_ptr;
    }
    return count;
}

size_t Stream::WriteArrayOfIntPtr32(const intptr_t *buffer, size_t count)
{
    if (!CanWrite())
    {
        return 0;
    }

    size_t elem;
    for (elem = 0; elem < count && !EOS(); ++elem, ++buffer)
    {
        int32_t val = (int32_t)*buffer;
        if (WriteInt32(val) < sizeof(int32_t))
        {
            break;
        }
    }
    return elem;
}

size_t Stream::WriteByteCount(uint8_t b, size_t count)
{
    if (!CanWrite())
        return 0;
    size_t size = 0;
    for (; count > 0; --count, ++size)
    {
        if (WriteByte(b) < 0)
            break;
    }
    return size;
}

} // namespace Common
} // namespace AGS
