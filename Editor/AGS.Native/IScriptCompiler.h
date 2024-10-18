//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#pragma once
#include "CompiledScript.h"

namespace AGS
{
namespace Native 
{

using namespace System;
using namespace System::Collections::Generic;
using namespace AGS::Types;

// TODO: move IScriptCompiler interface out of AGS.Native into one of the C# libs.
// There's an existing IScriptCompiler in AGS.CScript.Compiler, but modifying it
// will affect many other things, so I decided to keep this one separated for now.

[Flags]
public enum class ScriptCompilerOptions
{
    AutoExportFunctions = 0x0001,   // export all functions automatically
    // 0x0002, // [UNUSED] was "show warnings in console", but never implemented properly
    LineNumbers         = 0x0004,   // generate line numbers in compiled code
    // 0x0008, // [UNUSED] was runtime only option
    // 0x0010, // [UNUSED] was runtime only option
    // TODO: this flag might have to be deprecated as it makes inconsistent rules for distinct scripts
    NoImportOverride    = 0x0020,   // do not allow an import to be re-declared
    // 0x0040, // [DEPRECATED] was: left-to-right operator precedence 
    OldStrings          = 0x0080,   // allow old-style strings
    UTF8                = 0x0100,   // UTF-8 text mode
    RTTI                = 0x0200,   // generate and export RTTI
    RTTIOps             = 0x0400,   // enable syntax & opcodes that require RTTI to work
    ScriptTOC           = 0x0800,   // generate and export ScriptTOC
    NoAutoPtrImport     = 0x1000    // object pointers in imports must be declared explicitly
};

public interface class IScriptCompiler
{
public:
    // Gets this compiler's identifying name
    virtual String^ GetName() = 0;
    // Gets this compiler's human-readable name
    virtual String^ GetDescription() = 0;
    // Gets this compiler's list of supported extensions
    virtual List<String^>^ GetExtensions() = 0;
    // Gets if this compiler supports building several scripts in parallel
    virtual bool DoesSupportParallelBuilds() = 0;
    // Compiles the given script.
    // The caller must guarantee that the text does not contain any preprocessor commands,
    // and have only pure AGS script.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, String^ scriptText, ScriptCompilerOptions options, CompileMessages^ messages) = 0;
    // Compiles the given script text array, treating it as split parts of the same script.
    // scriptTexts represents headers and a script body, they are supposed to be merged
    // by the compiler itself prior to compilation.
    // The caller must guarantee that the text does not contain any preprocessor commands,
    // and have only pure AGS script.
    // NOTE: this variant is required for the legacy compiler only, as normally compiler
    // should detect new sections using a predefined string literal in text.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, cli::array<String^>^ scriptTexts, ScriptCompilerOptions options, CompileMessages^ messages) = 0;
};

} // namespace Native 
} // namespace AGS
