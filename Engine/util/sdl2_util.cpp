//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/sdl2_util.h"
#include "debug/assert.h"

using namespace AGS::Common;

namespace AGS
{
namespace Engine
{
namespace SDL2Util
{

static const Uint32 RWopsType = 0xF0001000;

/* These functions should not be used except from pointers in an SDL_RWops */
static Sint64 rwops_size(SDL_RWops *context)
{
    return static_cast<Stream*>(context->hidden.unknown.data1)->GetLength();
}

static StreamSeek RWopsSeekToAGS(int whence)
{
    switch (whence)
    {
    case RW_SEEK_SET: return kSeekBegin;
    case RW_SEEK_CUR: return kSeekCurrent;
    case RW_SEEK_END: return kSeekEnd;
    default: assert(false); return kSeekBegin;
    }
}

static Sint64 rwops_seek(SDL_RWops *context, Sint64 offset, int whence)
{
    auto *s = static_cast<Stream*>(context->hidden.unknown.data1);
    if (s->Seek(offset, RWopsSeekToAGS(whence)))
        return s->GetPosition();
    return -1;
}

static size_t rwops_read(SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
{
    return (static_cast<Stream*>(context->hidden.unknown.data1)->Read(ptr, size * maxnum) / size);
}

static size_t rwops_write(SDL_RWops *context, const void *ptr, size_t size, size_t num)
{
    return (static_cast<Stream*>(context->hidden.unknown.data1)->Write(ptr, size * num) / size);
}

static int rwops_close(SDL_RWops *context)
{
    if (context->type != RWopsType) {
        return SDL_SetError("Wrong kind of SDL_RWops for rwops_close(): got %u, expected %u", context->type, RWopsType);
    }

    delete static_cast<Stream*>(context->hidden.unknown.data1);
    SDL_FreeRW(context);
    return 0;
}

SDL_RWops *OpenRWops(std::unique_ptr<Stream> ags_stream)
{
    SDL_RWops *c = SDL_AllocRW();
    if (!c) return nullptr;
    c->size = rwops_size;
    c->seek = rwops_seek;
    c->read = rwops_read;
    c->write = rwops_write;
    c->close = rwops_close;
    c->type = RWopsType;
    c->hidden.unknown.data1 = ags_stream.release();
    return c;
}

} // namespace SDL2Util
} // namespace Engine
} // namespace AGS
