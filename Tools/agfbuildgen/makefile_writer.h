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
#ifndef AGS_TOOL_AGFBUILDGEN__MAKEFILE_WRITER_H
#define AGS_TOOL_AGFBUILDGEN__MAKEFILE_WRITER_H

#include <string>
#include <vector>
#include "util/string.h"
#include "util/filestream.h"
#include "text_writer.h"

using namespace AGS::Common;

class MakefileWriter: public Writer {
public:
    explicit MakefileWriter(const String &filename);
    void Comment(const String& text);
    void Variable(const String& key, const String& value);
    void Rule(const String& target, const std::vector<String>& dependencies, const String& command);
    void Phony(const String& target);
};

#endif // AGS_TOOL_AGFBUILDGEN__MAKEFILE_WRITER_H
