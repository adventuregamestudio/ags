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
#pragma once
#include "script/cc_script.h"
#include "script/cc_internal.h"
#include "util/file.h"
#include "util/stream.h"

namespace AGS
{
namespace Native
{

using namespace System;
using namespace AGS::Types;

public ref class CompiledScript : ICompiledScript
{
private:
	PScript* _compiledScript;
public:
	CompiledScript(PScript script) 
	{
		_compiledScript = new PScript();
		*_compiledScript = script;
	}

	property PScript Data
	{
		PScript get() { return *_compiledScript; }
		void set(PScript newScript) { *_compiledScript = newScript; }
	}

	~CompiledScript() 
	{
		this->!CompiledScript();
	}

	!CompiledScript()
	{
		_compiledScript->reset();
		delete _compiledScript;
		_compiledScript = NULL;
	}

    virtual void __clrcall Write(System::IO::FileStream ^ostream, System::String ^scriptFileName)
    {
        if (*_compiledScript == NULL)
        {
            throw gcnew AGS::Types::CompileError(gcnew System::String("Script has not been compiled: ") + scriptFileName);
        }
                
        // NOTE: if using temp files will prove inefficient,
        // we might switch to writing into a memory buffer instead,
        // or switching between file and buffer depending on script's size.
        String ^tempFile = System::IO::Path::GetTempFileName();
                
        std::unique_ptr<AGS::Common::Stream> out(
            AGS::Common::File::CreateFile(TextHelper::ConvertUTF8(tempFile)));
        (*_compiledScript)->Write(out.get());
        out.reset();
                
        Utilities::CopyFileContents(tempFile, ostream);

        System::IO::File::Delete(tempFile);
    }
};

} // namespace Native
} // namespace AGS
