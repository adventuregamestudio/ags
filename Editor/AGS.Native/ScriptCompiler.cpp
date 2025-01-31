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
// AGS Script Compiler interface to .NET
//
//=============================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "NativeMethods.h"
#include "NativeUtils.h"
#include "CompiledScript.h"
#include "IScriptCompiler.h"
#include "script/cs_compiler.h"
#include "script2/cs_compiler.h"
#include "script/cc_common.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_utils.h"

extern void ReplaceIconFromFile(const AGSString &iconName, const AGSString &exeName);
extern void ReplaceResourceInEXE(const AGSString &exeName, const char *resourceName, const unsigned char *data, int dataLength, const char *resourceType);

using namespace System::IO;

namespace AGS
{
namespace Native
{

//
// IScriptCompiler implementation for legacy AGS3 compiler
//
public ref class AGS3ScriptCompiler : public IScriptCompiler
{
public:
    AGS3ScriptCompiler()
    {
    }

    // Gets this compiler's identifying name
    virtual String^ GetName()
    {
        return "AGS SCOM 3 EXT";
    }
    // Gets this compiler's human-readable name
    virtual String^ GetDescription()
    {
        return "AGS v3-ext Script Compiler";
    }
    // Gets this compiler's list of supported extensions
    virtual List<String^>^ GetExtensions()
    {
        std::vector<std::string> cc_exts;
        ccGetExtensions(cc_exts);
        List<String^>^ exts = gcnew List<String^>();
        for (const auto &s : cc_exts)
            exts->Add(gcnew String(s.c_str()));
        return exts;
    }
    // Gets if this compiler supports building several scripts in parallel
    virtual bool DoesSupportParallelBuilds()
    {
        return false; // still relies on global data, so - no
    }
    // Compiles the given script.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, String^ scriptText, ScriptCompilerOptions options, CompileMessages^ messages)
    {
        cli::array<String^>^ scriptTexts = gcnew cli::array<String^>(1);
        scriptTexts[0] = scriptText;
        return CompileScript(scriptName, scriptTexts, options, messages);
    }
    // Compile the given script texts.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, cli::array<String^>^ scriptTexts, ScriptCompilerOptions options, CompileMessages^ messages)
    {
        // Prepare to convert script texts to native strings, in UTF-8 format
        TextConverter^ tcv = NativeMethods::GetGameTextConverter();
        // Set up compiler options
        uint64_t const cc_options = (UInt64)options;

        ccRemoveDefaultHeaders();

        std::vector<AGSString> scriptHeaders;
        scriptHeaders.resize(scriptTexts->Length - 1);
        AGSString mainScript;
        AGSString mainScriptName;
        int headerCount = 0;

        // FIXME: compiler should scan for NEW_SCRIPT_MARKER in a merged script text instead,
        // then we won't have to receive an array of texts, but a single merged text
        for each (String^ header in scriptTexts)
        {
            if (headerCount < scriptTexts->Length - 1)
            {
                scriptHeaders[headerCount] = tcv->Convert(header);

                if (ccAddDefaultHeader(scriptHeaders[headerCount].GetCStr(), "Header")) 
                {
                    messages->Add(gcnew CompileError("Too many scripts in game"));
                    return nullptr;
                }
                headerCount++;
            }
        }

        ccSetSoftwareVersion(editorVersionNumber.GetCStr());
        for (uint64_t opt_bit = 1; opt_bit <= SCOPT_HIGHEST; opt_bit <<= 1)
            ccSetOption(opt_bit, (cc_options & opt_bit) != 0);

        mainScript = tcv->Convert(scriptTexts[scriptTexts->Length - 1]);
        mainScriptName = TextHelper::ConvertASCII(scriptName);
        std::unique_ptr<ccScript> cc_script(
            ccCompileText(mainScript.GetCStr(), mainScriptName.GetCStr()));
        if (!cc_script || cc_has_error())
        {
            auto &error = cc_get_error();
            messages->Add(gcnew CompileError(tcv->Convert(error.ErrorString), TextHelper::ConvertASCII(ccCurScriptName), error.Line));
            return nullptr;
        }

        // Success, create new CompiledData
        return gcnew CompiledScript(std::move(cc_script));
    }
};

//
// IScriptCompiler implementation for AGS4 compiler
//
public ref class AGS4ScriptCompiler : public IScriptCompiler
{
public:
    AGS4ScriptCompiler()
    {
    }

    // Gets this compiler's identifying name
    virtual String^ GetName()
    {
        return "AGS SCOM 4";
    }
    // Gets this compiler's human-readable name
    virtual String^ GetDescription()
    {
        return "AGS v4 Script Compiler";
    }
    // Gets this compiler's list of supported extensions
    virtual List<String^>^ GetExtensions()
    {
        std::vector<std::string> cc_exts;
        ccGetExtensions2(cc_exts);
        List<String^>^ exts = gcnew List<String^>();
        for (const auto &s : cc_exts)
            exts->Add(gcnew String(s.c_str()));
        return exts;
    }
    // Gets if this compiler supports building several scripts in parallel
    virtual bool DoesSupportParallelBuilds()
    {
        return true;
    }

    // Compiles the given script texts.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, cli::array<String^>^ scriptTexts, ScriptCompilerOptions options, CompileMessages^ messages)
    {
        // Concatenate the whole thing together
        String^ all_the_script = "";
        for each (String^ header in scriptTexts)
            all_the_script += header;

        return CompileScript(scriptName, all_the_script, options, messages);
    }

    // Compiles the given script.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, String^ scriptText, ScriptCompilerOptions options, CompileMessages^ messages)
    {
        // Prepare to convert script texts to native strings, in UTF-8 format
        TextConverter^ tcv = NativeMethods::GetGameTextConverter();
        // Set up compiler options
        uint64_t const cc_options = (UInt64)options;

        AGS::MessageHandler mh;
        std::string mainScript = tcv->ConvertToStd(scriptText);
        std::string mainScriptName = TextHelper::ConvertASCIIToStd(scriptName);
        std::unique_ptr<ccScript> cc_script(
            ccCompileText2(mainScript, mainScriptName, cc_options, mh));

        auto compiler_messages = mh.GetMessages();
        int message_index = 0;
        for (auto msg = compiler_messages.begin(); msg != compiler_messages.end(); msg++)
            if (msg->Severity >= mh.kSV_UserError)
                messages->Add(gcnew CompileError(
                    message_index++,
                    gcnew String (msg->Message.c_str()),
                    gcnew String (msg->Section.c_str()),
                    static_cast<int>(msg->Lineno)));
            else
                messages->Add(gcnew CompileWarning(
                    message_index++,
                    gcnew String(msg->Message.c_str()),
                    gcnew String(msg->Section.c_str()),
                    static_cast<int>(msg->Lineno)));

        if (mh.HasError())
        {
            return nullptr;
        }

        // Success, create new CompiledData
        return gcnew CompiledScript(std::move(cc_script));
    }
};


List<IScriptCompiler^>^ NativeMethods::GetEmbeddedScriptCompilers()
{
    // Gather compilers in order of priority
    List<IScriptCompiler^>^ compilers = gcnew List<IScriptCompiler^>();
    compilers->Add(gcnew AGS4ScriptCompiler());
    compilers->Add(gcnew AGS3ScriptCompiler());
    return compilers;
}

void NativeMethods::UpdateFileIcon(String ^fileToUpdate, String ^iconName)
{
	if (System::Environment::OSVersion->Platform == System::PlatformID::Win32NT) 
	{
		ReplaceIconFromFile(TextHelper::ConvertUTF8(iconName), TextHelper::ConvertUTF8(fileToUpdate));
	}
}

void NativeMethods::FindAndUpdateMemory(unsigned char *data, int dataLen, const unsigned char *searchFor, int searchForLen, const unsigned char *replaceWith)
{
    for (int i = 0; i < dataLen - searchForLen; i++)
    {
        if (memcmp(&data[i], searchFor, searchForLen) == 0)
        {
            memcpy(&data[i], replaceWith, searchForLen);
            return;
        }
    }

    throw gcnew AGSEditorException("Unable to find source string for replacement");
}

void NativeMethods::ReplaceStringInMemory(unsigned char *memory, int memorySize, const char *searchFor, const unsigned char *replaceWithData)
{
    WCHAR searchForUnicode[100];
    MultiByteToWideChar(CP_ACP, 0, searchFor, (int)strlen(searchFor) + 1, searchForUnicode, sizeof(searchForUnicode));
    FindAndUpdateMemory(memory, memorySize, (unsigned char*)searchForUnicode, (int)strlen(searchFor) * 2, replaceWithData);
}

void NativeMethods::UpdateFileVersionInfo(String ^fileToUpdate, cli::array<System::Byte> ^authorNameUnicode, cli::array<System::Byte> ^gameNameUnicode)
{
    AGSString abs_path = AGS::Common::Path::MakeAbsolutePath(TextHelper::ConvertUTF8(fileToUpdate));
    WCHAR wpath[MAX_PATH_SZ];
    MultiByteToWideChar(CP_UTF8, 0, abs_path.GetCStr(), -1, wpath, MAX_PATH_SZ);
    HMODULE module = LoadLibraryExW(wpath, NULL, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
    if (module == NULL)
    {
        throw gcnew AGSEditorException(WinAPIHelper::MakeErrorManaged("LoadLibrary failed."));
    }
    HRSRC handle = FindResource(module, MAKEINTRESOURCE(1), RT_VERSION);
    if (handle == NULL)
    {
        FreeLibrary(module);
        throw gcnew AGSEditorException(WinAPIHelper::MakeErrorManaged("FindResource failed."));
    }
    HGLOBAL hglobal = LoadResource(module, handle);
    int dataSize = SizeofResource(module, handle);
    unsigned char *dataCopy = (unsigned char*)malloc(dataSize);
    unsigned char *versionData = (unsigned char*)LockResource(hglobal);
    memcpy(dataCopy, versionData, dataSize);
    UnlockResource(versionData);
    FreeLibrary(module);

    const char *searchFor = "AGS Engine by Chris Jones et al.        ";
    pin_ptr<Byte> authorNameData = &authorNameUnicode[0];
    ReplaceStringInMemory(dataCopy, dataSize, searchFor, authorNameData);

    searchFor = "Adventure Game Studio run-time engine   ";
    pin_ptr<Byte> descriptionData = &gameNameUnicode[0];
    ReplaceStringInMemory(dataCopy, dataSize, searchFor, descriptionData);

    ReplaceResourceInEXE(abs_path, MAKEINTRESOURCE(1), dataCopy, dataSize, MAKEINTRESOURCE(RT_VERSION));
    free(dataCopy);
}

} // namespace Native
} // namespace AGS
