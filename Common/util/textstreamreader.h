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
// Class for reading plain text from the stream
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TEXTSTREAMREADER_H
#define __AGS_CN_UTIL__TEXTSTREAMREADER_H

#include <memory>
#include "util/stream.h"
#include "util/textreader.h"

namespace AGS
{
namespace Common
{

class TextStreamReader : public TextReader
{
public:
    // TODO: use shared ptr
    TextStreamReader(std::unique_ptr<Stream> &&stream)
        : _stream(std::move(stream)) {}
    ~TextStreamReader() override = default;

    std::unique_ptr<Stream> ReleaseStream() { return std::move(_stream); }

    bool    IsValid() const override;
    bool    EOS() const;

    // Read single character
    char    ReadChar() override;
    // Read defined number of characters
    String  ReadString(size_t length) override;
    // Read till line break
    String  ReadLine() override;
    // Read till end of available data
    String  ReadAll() override;

private:
    std::unique_ptr<Stream> _stream;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAMREADER_H
