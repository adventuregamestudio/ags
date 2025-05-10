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
//
// DeflateStream implements data (un)compression using Deflate algorithm.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DEFLATESTREAM_H
#define __AGS_CN_UTIL__DEFLATESTREAM_H

#include "util/transformstream.h"

namespace AGS
{
namespace Common
{

class DeflateStream final : public TransformStream
{
public:
    DeflateStream(std::unique_ptr<IStreamBase> &&base_stream, StreamMode mode);
    // Opens deflate stream in read mode, limiting amount of data it may read out
    DeflateStream(std::unique_ptr<IStreamBase> &&base_stream, soff_t begin_pos, soff_t end_pos);
    ~DeflateStream();

protected:
    void OpenTransform() override;
    void OpenUnTransform() override;
    void CloseTransform() override;

    TransformResult Transform(const uint8_t *input, size_t in_sz, uint8_t *output, size_t out_sz, bool finalize,
                    size_t &in_read, size_t &out_wrote) override;
    TransformResult UnTransform(const uint8_t *input, size_t in_sz, uint8_t *output, size_t out_sz, bool finalize,
                    size_t &in_read, size_t &out_wrote) override;

private:
    struct ags_z_stream;
    std::unique_ptr<ags_z_stream> _zstream;
    bool _isDeflate = true;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DEFLATESTREAM_H
