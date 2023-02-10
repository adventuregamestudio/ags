/* AGS Script Compiler interface to .NET

Adventure Game Studio Editor Source Code
Copyright (c) 2006-2010 Chris Jones
------------------------------------------------------

The AGS Editor Source Code is provided under the Artistic License 2.0,
see the license.txt for details.
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "NativeMethods.h"
#include "NativeUtils.h"
#include "scripting.h"
#include "script/cs_compiler.h"
#include "script2/cs_compiler.h"
#include "script/cc_common.h"

extern void ReplaceIconFromFile(const char *iconName, const char *exeName);
extern void ReplaceResourceInEXE(const char *exeName, const char *resourceName, const unsigned char *data, int dataLength, const char *resourceType);

using namespace System::IO;

namespace AGS
{
	namespace Native
	{
    void NativeMethods::CompileScript(Script ^script, cli::array<String^> ^preProcessedScripts,
        Game ^game, CompileMessages ^messages)
    {
        if (script->CompiledData != nullptr)
            script->CompiledData = nullptr; // clear up previous data, if present

        std::unique_ptr<ccScript> cc_script;
        // Prepare to convert script texts to native strings, in UTF-8 format
        TextConverter^ tcv = NativeMethods::GetGameTextConverter();
        // Set up compiler options
        uint64_t const cc_options =
                SCOPT_EXPORTALL |
                SCOPT_LINENUMBERS |
                SCOPT_OLDSTRINGS * (!game->Settings->EnforceNewStrings) |
                SCOPT_UTF8 * (game->UnicodeMode) |
                false;

        if (game->Settings->ExtendedCompiler)
        {
            // Concatenate the whole thing together
            String^ all_the_script = "";
            for each (String^ header in preProcessedScripts)
                all_the_script += header;

            AGS::MessageHandler mh;

            std::string mainScript = tcv->ConvertToStd(all_the_script);
            std::string mainScriptName = tcv->ConvertToStd(script->FileName);
            cc_script.reset(ccCompileText2(mainScript, mainScriptName, cc_options, mh));

            auto compiler_messages = mh.GetMessages();
            for (auto msg = compiler_messages.begin(); msg != compiler_messages.end(); msg++)
                if (msg->Severity >= mh.kSV_UserError)
                    messages->Add(gcnew CompileError(
                        gcnew String (msg->Message.c_str()),
                        gcnew String (msg->Section.c_str()),
                        static_cast<int>(msg->Lineno)));
                else
                    messages->Add(gcnew CompileWarning(
                        gcnew String(msg->Message.c_str()),
                        gcnew String(msg->Section.c_str()),
                        static_cast<int>(msg->Lineno)));

            if (mh.HasError())
            {
                return;
            }
        }
        else
        {
            ccRemoveDefaultHeaders();

            CompileMessage ^compile_error = nullptr;

            std::vector<AGSString> scriptHeaders;
            scriptHeaders.resize(preProcessedScripts->Length - 1);
            AGSString mainScript;
            AGSString mainScriptName;
            int headerCount = 0;

            for each (String^ header in preProcessedScripts)
            {
                if (headerCount < preProcessedScripts->Length - 1)
                {
                    scriptHeaders[headerCount] = tcv->Convert(header);

                    if (ccAddDefaultHeader(scriptHeaders[headerCount].GetCStr(), "Header")) 
                    {
                        compile_error = gcnew CompileError("Too many scripts in game");
                        break;
                    }
                    headerCount++;
                }
            }

            ccSetSoftwareVersion(editorVersionNumber.GetCStr());
            for (uint64_t opt_bit = 1; opt_bit <= SCOPT_HIGHEST; opt_bit <<= 1)
                ccSetOption(opt_bit, (cc_options & opt_bit) != 0);

            if (compile_error == nullptr)
            {
                mainScript = tcv->Convert(preProcessedScripts[preProcessedScripts->Length - 1]);
                mainScriptName = tcv->Convert(script->FileName);
                cc_script.reset(ccCompileText(mainScript.GetCStr(), mainScriptName.GetCStr()));
                if (!cc_script || cc_has_error())
                {
                    auto &error = cc_get_error();
                    compile_error = gcnew CompileError(tcv->Convert(error.ErrorString), TextHelper::ConvertASCII(ccCurScriptName), error.Line);
                }
            }

            if (compile_error != nullptr)
            {
                messages->Add(compile_error);
            }
        }

        // Success, create new CompiledData
        script->CompiledData = gcnew CompiledScript(PScript(cc_script.release()));
    }

		void NativeMethods::UpdateFileIcon(String ^fileToUpdate, String ^iconName)
		{
			if (System::Environment::OSVersion->Platform == System::PlatformID::Win32NT) 
			{
				char iconNameChars[MAX_PATH];
				TextHelper::ConvertASCIIFilename(iconName, iconNameChars, MAX_PATH);

				char fileNameChars[MAX_PATH];
				TextHelper::ConvertASCIIFilename(fileToUpdate, fileNameChars, MAX_PATH);

				ReplaceIconFromFile(iconNameChars, fileNameChars);
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
			char fileNameChars[MAX_PATH];
			TextHelper::ConvertASCIIFilename(fileToUpdate, fileNameChars, MAX_PATH);

      HMODULE module = LoadLibraryEx(fileNameChars, NULL, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
      if (module == NULL)
      {
        throw gcnew AGSEditorException("LoadLibrary failed");
      }
      HRSRC handle = FindResource(module, MAKEINTRESOURCE(1), RT_VERSION);
      if (handle == NULL)
      {
        FreeLibrary(module);
        throw gcnew AGSEditorException("FindResource failed");
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

      ReplaceResourceInEXE(fileNameChars, MAKEINTRESOURCE(1), dataCopy, dataSize, MAKEINTRESOURCE(RT_VERSION));
      free(dataCopy);
    }

    } // namespace Native
} // namespace AGS

