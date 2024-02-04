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
#include "util/stream.h"
#include <algorithm>

namespace AGS
{
namespace Common
{

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

soff_t CopyStream(Stream *in, Stream *out, soff_t length)
{
    char buf[4096];
    soff_t wrote_num = 0;
    while (length > 0)
    {
        size_t to_read = (size_t)std::min((soff_t)sizeof(buf), length);
        size_t was_read = in->Read(buf, to_read);
        if (was_read == 0)
            return wrote_num;
        length -= was_read;
        size_t to_write = was_read;
        while (to_write > 0)
        {
            size_t wrote = out->Write(buf + was_read - to_write, to_write);
            if (wrote == 0)
                return wrote_num;
            to_write -= wrote;
            wrote_num += wrote;
        }
    }
    return wrote_num;
}

} // namespace Common
} // namespace AGS
