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
// MessageBuffer, the IOutputHandler implementation that stores debug messages
// in a vector. Could be handy if you need to temporarily buffer debug log
// while specifying how to actually print it.
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__MESSAGEBUFFER_H
#define __AGS_CN_DEBUG__MESSAGEBUFFER_H

#include <vector>
#include "debug/outputhandler.h"

namespace AGS
{
namespace Common
{

class MessageBuffer : public IOutputHandler
{
public:
    MessageBuffer(size_t buffer_limit = 1024u)
        : _bufferLimit(buffer_limit)
    {}

    void OnRegister() override
    {
        // do nothing
    }

    void PrintMessage(const DebugMessage &msg) override
    {
        if (_buffer.size() < _bufferLimit)
            _buffer.push_back(msg);
        else
            _msgLost++;
    }

    // Clears buffer
    void Clear()
    {
        _buffer.clear();
    }
    // Grants read access to the buffered messages
    const std::vector<DebugMessage> GetBuffer() const
    {
        return _buffer;
    }
    // Tells how many messages were lost, exceeding the buffer limit
    size_t GetMessagesLost() const
    {
        return _msgLost;
    }

private:
    const size_t    _bufferLimit;
    std::vector<DebugMessage> _buffer;
    size_t          _msgLost = 0u;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_DEBUG__MESSAGEBUFFER_H
