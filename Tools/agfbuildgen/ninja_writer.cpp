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
#include "ninja_writer.h"

using namespace AGS::Common;
using namespace WriterUtils;

NinjaWriter::NinjaWriter(const String &filename, int width)  : Writer(filename), _width(width)
{
    _indent_char = ' ';
    _indent_multiplier = 2;
}

String NinjaWriter::EscapePath(const String& word)
{
    String escaped = word;
    escaped.Replace("$ ", "$$ ");
    escaped.Replace(" ", "$ ");
    escaped.Replace(":", "$:");
    return escaped;
}

std::vector<String> NinjaWriter::EscapePaths(const std::vector<String>& words)
{
    std::vector<String> escaped_paths;
    escaped_paths.reserve(words.size());
    for (const auto& word : words)
    {
        escaped_paths.push_back(EscapePath(word));
    }
    return escaped_paths;
}

void NinjaWriter::Comment(const String& text)
{
    Write("# ");
    Write(text);
    Newline();
}

void NinjaWriter::Variable(const String& key, const String& value, int indent)
{
    if (!value.IsNullOrSpace())
    {
        Line(String::FromFormat("%s = %s", key.GetCStr(), value.GetCStr()), indent);
    }
}

void NinjaWriter::Pool(const String& name, int depth)
{
    Line(String::FromFormat("pool %s", name.GetCStr()));
    Variable("depth", String::FromFormat("%d", depth), 1);
}

void NinjaWriter::Rule(const String& name, const String& command, const String& description, const String& depfile,
                       bool generator, const String& pool, bool restat, const String& rspfile, const String& rspfile_content, const String& deps) {
    Line(String::FromFormat("rule %s", name.GetCStr()));
    Variable("command", command, 1);
    if (!description.IsNullOrSpace())
        Variable("description", description, 1);
    if (!depfile.IsNullOrSpace())
        Variable("depfile", depfile, 1);
    if (generator)
        Variable("generator", "1", 1);
    if (!pool.IsNullOrSpace())
        Variable("pool", pool, 1);
    if (restat)
        Variable("restat", "1", 1);
    if (!rspfile.IsNullOrSpace())
        Variable("rspfile", rspfile, 1);
    if (!rspfile_content.IsNullOrSpace())
        Variable("rspfile_content", rspfile_content, 1);
    if (!deps.IsNullOrSpace())
        Variable("deps", deps, 1);
}

void NinjaWriter::Rule(const String& name, const String& command, const String& description)
{
    Rule(name, command, description,
    nullptr, false, nullptr,
    false, nullptr, nullptr, nullptr
    );
}

void NinjaWriter::Rule(const String& name, const String& command)
{
    Rule(name, command, nullptr);
}

void NinjaWriter::Build(const std::vector<String>& outputs, const String& rule, const std::vector<String>& inputs,
                        const std::vector<String>& imp_deps, const std::vector<String>& order_only,
                        const std::vector<std::pair<String, String>>& variables, const std::vector<String>& implicit_outputs,
                        const String& pool, const String& dyndep)
{
    auto out_outputs = EscapePaths(outputs);
    auto all_inputs = EscapePaths(inputs);

    if (!imp_deps.empty())
    {
        auto implicits = EscapePaths(imp_deps);
        all_inputs.push_back("|");
        all_inputs.insert(all_inputs.end(), implicits.begin(), implicits.end());
    }
    if (!order_only.empty())
    {
        auto order_onlys = EscapePaths(order_only);
        all_inputs.push_back("||");
        all_inputs.insert(all_inputs.end(), order_onlys.begin(), order_onlys.end());
    }
    if (!implicit_outputs.empty())
    {
        auto implicit_outs = EscapePaths(implicit_outputs);
        out_outputs.push_back("|");
        out_outputs.insert(out_outputs.end(), implicit_outs.begin(), implicit_outs.end());
    }

    Line(String::FromFormat("build %s: %s", Join(" ", out_outputs).GetCStr(), Join(" ", rule, all_inputs).GetCStr()));

    if (!pool.IsNullOrSpace())
        Line(String::FromFormat("  pool = %s", pool.GetCStr()));
    if (!dyndep.IsNullOrSpace())
        Line(String::FromFormat("  dyndep = %s", dyndep.GetCStr()));

    for (const auto& var : variables)
    {
        Variable(var.first, var.second, 1);
    }
}

void NinjaWriter::Build(const std::vector<String>& outputs, const String& rule, const std::vector<String>& inputs) {
    Build(outputs, rule, inputs,
          {}, {},
          {}, {},
          nullptr, nullptr
    );
}

void NinjaWriter::Build(const std::vector<String>& outputs, const String& rule)
{
    Build(outputs, rule, {});
}


void NinjaWriter::Include(const String& path)
{
    Line(String::FromFormat("include %s", path.GetCStr()));
}

void NinjaWriter::Subninja(const String& path)
{
    Line(String::FromFormat("subninja %s", path.GetCStr()));
}

void NinjaWriter::Default(const std::vector<String>& paths)
{
    Line(String::FromFormat("default %s", Join(" ", paths).GetCStr()));
}
