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
//
// TransformStream is a parent class for the data transformation streams,
// where the contents and length of input and output data are different.
// One example is data compression.
// 
// Technically, TransformStream is a wrapper over a IStreamBase object,
// which does the actual reading or writing from or to a device (file, mem).
// TransformStream transforms on writing and un-transforms on reading:
// 1. In the "read" mode, it will read data from the base stream,
//    untransform, and return normal data to the user.
// 2. In the "write" mode, it will receive normal data from the user,
//    transform, and write transformed data into the base stream.
// 
// Because there's no direct correspondence between data lengths and positions,
// TransformStream does not support Seek()* and GetLength().
// When you are reading - read data out until EOS() returns positive, or Read()
// returns 0. When you are writing - you should know when to stop yourself.
// (*) NOTE: for compatibility with the old code transform stream does
// support forward seeking, in which case it reads out a number of bytes.
// This forward seeking uses *untransformed* bytes as offset units.
// 
// Stream position is reported: that would be position in "untransformed"
// bytes, either put into the stream or received from the stream, depending
// on the working mode.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TRANSFORMSTREAM_H
#define __AGS_CN_UTIL__TRANSFORMSTREAM_H

#include <vector>
#include "util/stream.h"

namespace AGS
{
namespace Common
{

class TransformStream : public StreamBase
{
public:
    // CHECKME: needs tuning depending on the platform? or a way to configure.
    static const size_t BufferSize = 1024u * 64;

    TransformStream(std::unique_ptr<IStreamBase> &&base_stream, StreamMode mode);
    // Opens a transform stream in read mode, limiting amount of data it may read out
    TransformStream(std::unique_ptr<IStreamBase> &&base_stream, soff_t begin_pos, soff_t end_pos);
    ~TransformStream();

    IStreamBase *GetStreamBase() { return _base.get(); }
    std::unique_ptr<IStreamBase> ReleaseStreamBase();

    const char *GetPath() const override { return _base ? _base->GetPath() : ""; }
    StreamMode GetMode() const override { return _mode; }
    bool    GetError() const override { return _base ? _base->GetError() : false; }

    // Is end of stream
    bool    EOS() const override;
    // Total length of stream is not supported for transform streams
    soff_t  GetLength() const override { assert(false); return 0; }
    // Current position is not supported for transform streams
    soff_t  GetPosition() const override { return _inputProcessed; }

    void    Close() override;
    // NOTE: flush does not work reliably in transform streams,
    // as transformation algorithm might require to know whether the
    // input is complete in order to write pending result.
    // Consider using Finalize() instead.
    bool    Flush() override;

    size_t  Read(void *buffer, size_t size) override;
    int32_t ReadByte() override;
    size_t  Write(const void *buffer, size_t size) override;
    int32_t WriteByte(uint8_t val) override;

    // Seek operation is formally not supported for transform streams.
    // For compatibility with the old code we still allow "forward seeking",
    // which is implemented as reading out and discarding a number of bytes.
    // This forward seeking uses *untransformed* bytes as offset units.
    soff_t  Seek(soff_t offset, StreamSeek origin) override;

    soff_t  GetProcessedInput() const { return _inputProcessed; }
    // Finalizes transformation in the write mode.
    // This is similar to Flush() in regular streams, and guarantees that
    // any pending transformation will be forced to complete.
    // No write operation is expected to work after this.
    void    Finalize();

protected:
    enum class TransformResult
    {
        // generic error (invalid stream state, or invalid data)
        Error,
        // generic success, returned when any progress was made
        OK,
        // transformation completed, no more input will be accepted
        End,
        // more buffer space required (input or output)
        Buffer
    };

    virtual void OpenTransform() = 0;
    virtual void OpenUnTransform() = 0;
    virtual void CloseTransform() = 0;

    virtual TransformResult Transform(const uint8_t *input, size_t in_sz, uint8_t *output, size_t out_sz, bool finalize,
                           size_t &in_read, size_t &out_wrote) = 0;
    virtual TransformResult UnTransform(const uint8_t *input, size_t in_sz, uint8_t *output, size_t out_sz, bool finalize,
                           size_t &in_read, size_t &out_wrote) = 0;

private:
    void Open(std::unique_ptr<IStreamBase> &&base_stream, StreamMode mode);
    void ReadBuffer();
    void WriteBuffer(bool finalize);
    void CloseOnDisposal();

    // TODO: store IStreamBase as a shader_ptr instead? will let use it after wrapper is disposed
    std::unique_ptr<IStreamBase> _base;
    StreamMode _mode = kStream_None;
    soff_t _baseBeginPos = 0;
    soff_t _baseEndPos = 0;
    // Input buffer: where the original data is being read to
    // before passing into the transform algorithm.
    std::vector<uint8_t> _inBuffer;
    size_t _inBufPos  = 0u;
    size_t _inBufEnd = 0u;
    // Output buffer: where the (un)transformed data is stored.
    std::vector<uint8_t> _outBuffer;
    size_t _outBufEnd = 0u;
    size_t _outBufPos = 0u;
    TransformResult _lastResult = TransformResult::OK;
    // Total untransformed bytes processed (read or written)
    soff_t _inputProcessed = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TRANSFORMSTREAM_H
