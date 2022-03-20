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
//#include "cscomp.h"
#include "script/cs_compiler.h"
#include "script2/cs_compiler.h"
#include "script/cc_options.h"
#include "script/cc_error.h"

extern void ReplaceIconFromFile(const char *iconName, const char *exeName);
extern void ReplaceResourceInEXE(const char *exeName, const char *resourceName, const unsigned char *data, int dataLength, const char *resourceType);
static const char *GAME_DEFINITION_FILE_RESOURCE = "__GDF_XML";
static const char *GAME_DEFINITION_THUMBNAIL_RESOURCE = "__GDF_THUMBNAIL";

using namespace System::IO;

namespace AGS
{
	namespace Native
	{

    void NativeMethods::CompileScript(Script ^script, cli::array<String^> ^preProcessedScripts, Game ^game, CompileMessages ^messages)
    {
			TextConverter^ tcv = NativeMethods::GetGameTextConverter();

        if (script->CompiledData != nullptr)
            script->CompiledData = nullptr;

        if (game->Settings->ExtendedCompiler)
        {
            ccScript *scrpt = nullptr;

            // Concatenate the whole thing together
            String^ all_the_script = "";
            for each (String^ header in preProcessedScripts)
                all_the_script += header;

            long const options =
                SCOPT_EXPORTALL |
                SCOPT_LINENUMBERS |
                SCOPT_OLDSTRINGS * (!game->Settings->EnforceNewStrings) |
                false;

            AGS::MessageHandler mh;

            char *mainScript = (char *) System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(all_the_script).ToPointer();
            char *mainScriptName = (char *) System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(script->FileName).ToPointer();
            scrpt = ccCompileText2(mainScript, mainScriptName, options, mh);
            System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(mainScript));
            System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(mainScriptName));

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
                delete scrpt;
                return;
            }
            script->CompiledData = gcnew CompiledScript(PScript(scrpt));
        }
        else
        {
            ccRemoveDefaultHeaders();

            CompileMessage ^compile_error = nullptr;

            std::vector<AGSString> scriptHeaders;
            scriptHeaders.resize(preProcessedScripts->Length - 1);
            AGSString mainScript;
            AGSString mainScriptName;
            ccScript *scrpt = NULL;
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
            ccSetOption(SCOPT_EXPORTALL, 1);
            ccSetOption(SCOPT_LINENUMBERS, 1);
            ccSetOption(SCOPT_OLDSTRINGS, !game->Settings->EnforceNewStrings);
			  ccSetOption(SCOPT_UTF8, game->UnicodeMode);

            if (compile_error == nullptr)
            {
                mainScript = tcv->Convert(preProcessedScripts[preProcessedScripts->Length - 1]);
                mainScriptName = tcv->Convert(script->FileName);
                scrpt = ccCompileText(mainScript.GetCStr(), mainScriptName.GetCStr());
                if ((scrpt == NULL) || (ccError != 0))
                {
                    compile_error = gcnew CompileError(tcv->Convert(ccErrorString), TextHelper::ConvertASCII(ccCurScriptName), ccErrorLine);
                }
            }

            if (compile_error != nullptr)
            {
                delete scrpt;
                messages->Add(compile_error);
                return;
            }

            script->CompiledData = gcnew CompiledScript(PScript(scrpt));
        }
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

    void NativeMethods::UpdateGameExplorerThumbnail(String ^fileToUpdate, cli::array<System::Byte> ^newData)
    {
      this->UpdateResourceInFile(fileToUpdate, GAME_DEFINITION_THUMBNAIL_RESOURCE, newData);
    }

    void NativeMethods::UpdateGameExplorerXML(String ^fileToUpdate, cli::array<System::Byte> ^newData)
		{
      this->UpdateResourceInFile(fileToUpdate, GAME_DEFINITION_FILE_RESOURCE, newData);
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

      HMODULE module = LoadLibrary(fileNameChars);
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

    void NativeMethods::UpdateResourceInFile(String ^fileToUpdate, const char *resourceName, cli::array<System::Byte> ^newData)
    {
			if (System::Environment::OSVersion->Platform == System::PlatformID::Win32NT) 
			{
				char fileNameChars[MAX_PATH];
				TextHelper::ConvertASCIIFilename(fileToUpdate, fileNameChars, MAX_PATH);

        if (newData == nullptr) 
        {
          ReplaceResourceInEXE(fileNameChars, resourceName, NULL, 0, "DATA");
        }
        else
        {
          unsigned char *data = new unsigned char[newData->Length];
          for (int i = 0; i < newData->Length; i++)
          {
            data[i] = newData[i];
          }

				  ReplaceResourceInEXE(fileNameChars, resourceName, data, newData->Length, "DATA");

          delete data;
        }
			}
		}

	}
}
