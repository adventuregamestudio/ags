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
#include "util/deflatestream.h"
#include <miniz.h>
#include <stdexcept>

namespace AGS
{
namespace Common
{

struct DeflateStream::ags_z_stream : ::z_stream {};

DeflateStream::DeflateStream(std::unique_ptr<IStreamBase> &&base_stream, StreamMode mode)
    : TransformStream(std::move(base_stream), mode)
{
    if (GetMode() == kStream_Read)
        OpenUnTransform();
    else
        OpenTransform();
}

DeflateStream::DeflateStream(std::unique_ptr<IStreamBase> &&base_stream, soff_t begin_pos, soff_t end_pos)
    : TransformStream(std::move(base_stream), begin_pos, end_pos)
{
    // Only in read mode
    OpenUnTransform();
}

DeflateStream::~DeflateStream()
{
    CloseTransform();
}

void DeflateStream::OpenTransform()
{
    auto *zs = new ags_z_stream();
    memset(zs, 0, sizeof(ags_z_stream));
    int ret = deflateInit(zs, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        throw std::runtime_error("DeflateStream: inflate initialization failed.");
    _zstream.reset(zs);
    _isDeflate = true;
}

void DeflateStream::OpenUnTransform()
{
    auto *zs = new ags_z_stream();
    memset(zs, 0, sizeof(ags_z_stream));
    int ret = inflateInit(zs);
    if (ret != Z_OK)
        throw std::runtime_error("DeflateStream: deflate initialization failed.");
    _zstream.reset(zs);
    _isDeflate = false;
}

void DeflateStream::CloseTransform()
{
    if (_zstream)
    {
        if (_isDeflate)
            deflateEnd(_zstream.get());
        else
            inflateEnd(_zstream.get());
        _zstream = nullptr;
    }
}

DeflateStream::TransformResult
DeflateStream::Transform(const uint8_t *input, size_t in_sz, uint8_t *output, size_t out_sz, bool finalize,
                              size_t &in_read, size_t &out_wrote)
{
    assert(_zstream);
    _zstream->next_in = input;
    _zstream->avail_in = in_sz;
    _zstream->next_out = output;
    _zstream->avail_out = out_sz;
    const int flush = finalize ? Z_FINISH : Z_NO_FLUSH;

    int ret = deflate(_zstream.get(), flush);
    assert(ret != Z_STREAM_ERROR);

    in_read = in_sz - _zstream->avail_in;
    out_wrote = out_sz - _zstream->avail_out;

    switch (ret)
    {
    case Z_OK: return TransformResult::OK;
    case Z_STREAM_END: return TransformResult::End;
    case Z_BUF_ERROR: return TransformResult::Buffer;
    default: return TransformResult::Error;
    }
}

DeflateStream::TransformResult
DeflateStream::UnTransform(const uint8_t *input, size_t in_sz, uint8_t *output, size_t out_sz, bool finalize,
                                size_t &in_read, size_t &out_wrote)
{
    assert(_zstream);
    _zstream->next_in = input;
    _zstream->avail_in = in_sz;
    _zstream->next_out = output;
    _zstream->avail_out = out_sz;
    // CHECKME: miniz does not like it when Z_FINISH is called with not enough output space
    const int flush = Z_NO_FLUSH;// finalize ? Z_FINISH : Z_NO_FLUSH;

    int ret = inflate(_zstream.get(), flush);
    assert(ret != Z_STREAM_ERROR);

    in_read = in_sz - _zstream->avail_in;
    out_wrote = out_sz - _zstream->avail_out;

    switch (ret)
    {
    case Z_OK: return TransformResult::OK;
    case Z_STREAM_END: return TransformResult::End;
    case Z_BUF_ERROR: return TransformResult::Buffer;
    default: return TransformResult::Error;
    }
}

} // namespace Common
} // namespace AGS
