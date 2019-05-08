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

#include "util/bufferedstream.h"

#include <stdexcept>

#include <cstring>
#include <algorithm>

#include "util/stdio_compat.h"
#include "util/string.h"

namespace AGS {
namespace Common {

BufferedStream::BufferedStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode, DataEndianess stream_endianess)
        : FileStream(file_name, open_mode, work_mode, stream_endianess), buffer_(BufferStreamSize), bufferPosition_(0), position_(0)
{
    if (FileStream::Seek(0, kSeekEnd) == false) {
        throw std::runtime_error("Error determining stream end.");
    }

    end_ = FileStream::GetPosition();
    if (end_ == -1) {
        throw std::runtime_error("Error determining stream end.");
    }

    if (FileStream::Seek(0, kSeekBegin) == false) {
        throw std::runtime_error("Error determining stream end.");
    }

    buffer_.resize(0);

}

void BufferedStream::FillBufferFromPosition(soff_t position) {
    FileStream::Seek(position, kSeekBegin);

    buffer_.resize(BufferStreamSize);
    auto sz = FileStream::Read(buffer_.data(), BufferStreamSize);
    buffer_.resize(sz);

	bufferPosition_ = position;
}

bool BufferedStream::EOS() const {
    return position_ == end_;
}

soff_t BufferedStream::GetPosition() const {
    return position_;
}

size_t BufferedStream::Read(void *toBuffer, size_t toSize) {
    auto to = static_cast<char *>(toBuffer);

    while(toSize > 0) {
        if (position_ < bufferPosition_ || position_ >= bufferPosition_ + buffer_.size()) {
            FillBufferFromPosition(position_);
            if (buffer_.size() <= 0) { break; } // reached EOS
        }

        auto bufferOffset = position_ - bufferPosition_;
        auto bytesLeft = buffer_.size() - bufferOffset;
        auto chunkSize = std::min<size_t>(bytesLeft, toSize);

        std::memcpy(to, buffer_.data()+bufferOffset, chunkSize);

        to += chunkSize;
        position_ += chunkSize;
        toSize -= chunkSize;
    }

    return to - (char*)toBuffer;
}

int32_t BufferedStream::ReadByte() {
    uint8_t ch;
    auto bytesRead = Read(&ch, 1);
    if (bytesRead != 1) { return EOF; }
    return ch;
}

size_t BufferedStream::Write(const void *buffer, size_t size) { 
    FileStream::Seek(position_, kSeekBegin);
    auto sz = FileStream::Write(buffer, size);
	position_ += sz;
	return sz;
}

int32_t BufferedStream::WriteByte(uint8_t val) {
	auto sz = Write(&val, 1);
	if (sz != 1) { return -1; }
	return sz;
}

bool BufferedStream::Seek(soff_t offset, StreamSeek origin) {
    switch(origin) {
        case StreamSeek::kSeekCurrent:  position_ = position_   + offset; break;
        case StreamSeek::kSeekBegin:    position_ = 0           + offset; break;
        case StreamSeek::kSeekEnd:      position_ = end_        + offset; break;
        break;
    }

    // clamp
    position_ = std::min(position_, end_);
    position_ = std::max(position_, (soff_t)0);

    return position_;
}

} // namespace Common
} // namespace AGS
