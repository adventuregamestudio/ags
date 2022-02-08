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

		void NativeMethods::CompileScript(Script ^script, cli::array<String^> ^preProcessedScripts, Game ^game)
		{
			if (script->CompiledData != nullptr)
			{
				//script->CompiledData->Dispose();
				script->CompiledData = nullptr;
			}

      ccRemoveDefaultHeaders();

			  CompileMessage ^exceptionToThrow = nullptr;

        char **scriptHeaders = new char*[preProcessedScripts->Length - 1];
			  char *mainScript;
			  char *mainScriptName;
        ccScript *scrpt = NULL;
			  int headerCount = 0;

			  for each (String^ header in preProcessedScripts) 
			  {
          if (headerCount < preProcessedScripts->Length - 1)
          {
				    scriptHeaders[headerCount] = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(header).ToPointer();

            if (ccAddDefaultHeader(scriptHeaders[headerCount], "Header")) 
            {
              exceptionToThrow = gcnew CompileError("Too many scripts in game");
            }
				    headerCount++;
          }
			  }
			  mainScript = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(preProcessedScripts[preProcessedScripts->Length - 1]).ToPointer();
			  mainScriptName = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(script->FileName).ToPointer();

			  ccSetSoftwareVersion(editorVersionNumber.GetCStr());

			  ccSetOption(SCOPT_EXPORTALL, 1);
			  ccSetOption(SCOPT_LINENUMBERS, 1);
			  ccSetOption(SCOPT_LEFTTORIGHT, game->Settings->LeftToRightPrecedence);
			  ccSetOption(SCOPT_OLDSTRINGS, !game->Settings->EnforceNewStrings);

        if (exceptionToThrow == nullptr)
        {
			    scrpt = ccCompileText(mainScript, mainScriptName);
 			    if ((scrpt == NULL) || (ccError != 0))
			    {
				    exceptionToThrow = gcnew CompileError(tcv.ConvertAny(ccErrorString), tcv.ConvertASCII(ccCurScriptName), ccErrorLine);
			    }
        }
			  
			  System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(mainScript));
			  System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(mainScriptName));

        for (int i = 0; i < preProcessedScripts->Length - 1; i++) 
			  {
				  System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(scriptHeaders[i]));
			  }
			  delete scriptHeaders;

			  if (exceptionToThrow != nullptr) 
			  {
				  delete scrpt;
				  throw exceptionToThrow;
			  }

			  script->CompiledData = gcnew CompiledScript(PScript(scrpt));
		}

		void NativeMethods::UpdateFileIcon(String ^fileToUpdate, String ^iconName)
		{
			if (System::Environment::OSVersion->Platform == System::PlatformID::Win32NT) 
			{
				char iconNameChars[MAX_PATH];
                tcv.ConvertASCIIFilename(iconName, iconNameChars, MAX_PATH);

				char fileNameChars[MAX_PATH];
                tcv.ConvertASCIIFilename(fileToUpdate, fileNameChars, MAX_PATH);

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
			tcv.ConvertASCIIFilename(fileToUpdate, fileNameChars, MAX_PATH);

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
                tcv.ConvertASCIIFilename(fileToUpdate, fileNameChars, MAX_PATH);

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