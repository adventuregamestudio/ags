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
// Ninja Syntax for build generators.
// based on https://github.com/ninja-build/ninja/blob/master/misc/ninja_syntax.py
//
//=============================================================================
#ifndef AGS_TOOL_AGFBUILDGEN__NINJA_WRITER_H
#define AGS_TOOL_AGFBUILDGEN__NINJA_WRITER_H

#include <vector>
#include <memory>
#include "util/string.h"
#include "util/filestream.h"
#include "text_writer.h"

using namespace AGS::Common;

class NinjaWriter: public Writer {
private:
    int _width;
    String EscapePath(const String& word);
    std::vector<String> EscapePaths(const std::vector<String>& words);
public:
    // fix to use OutputFileWriter somehow
    explicit NinjaWriter(const String &filename, int width = 78);
    virtual ~NinjaWriter() = default;
    void Comment(const String& text);
    void Variable(const String& key, const String& value, int indent = 0);
    void Pool(const String& name, int depth);

    void Rule(const String& name, const String& command, const String& description, const String& depfile,
              bool generator = false, const String& pool = "", bool restat = false, const String& rspfile = "",
              const String& rspfile_content = "", const String& deps = "");

    void Rule(const String& name, const String& command, const String& description);
    void Rule(const String& name, const String& command);

    void Build(const std::vector<String>& outputs, const String& rule, const std::vector<String>& inputs,
               const std::vector<String>& imp_deps, const std::vector<String>& order_only,
               const std::vector<std::pair<String, String>>& variables,
               const std::vector<String>& implicit_outputs = {},
               const String& pool = "", const String& dyndep = "");

    void Build(const std::vector<String>& outputs, const String& rule, const std::vector<String>& inputs);
    void Build(const std::vector<String>& outputs, const String& rule);

    void Include(const String& path);
    void Subninja(const String& path);
    void Default(const std::vector<String>& paths);
};

#endif // AGS_TOOL_AGFBUILDGEN__NINJA_WRITER_H
