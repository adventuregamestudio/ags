/*
  The SCC API was formerly under NDA, but now it's on MSDN:
  http://msdn.microsoft.com/en-us/library/bb165429(v=vs.80).aspx
  so it must be de-classified!
*/
/* AGS Source Control Integration
Adventure Game Studio Editor Source Code
Copyright (c) 2006-2010 Chris Jones
------------------------------------------------------

The AGS Editor Source Code is provided under the Artistic License 2.0,
see the license.txt for details.
*/
#include "NativeMethods.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>

extern void ConvertStringToCharArray(System::String^ clrString, char *textBuffer);
extern void ConvertFileNameToCharArray(System::String^ clrString, char *textBuffer);

#define SCC_NAME_LEN            31      // lpSccName, SCCInitialize
#define SCC_AUXLABEL_LEN        31      // lpAuxPathLabel, SCCInitialize
#define SCC_USER_LEN            31
#define SCC_PRJPATH_LEN         300     // lpAuxProjPath, SCCGetProjPath
#define SCC_FILETYPE_AUTO       0
#define SCC_FILETYPE_TEXT       1
#define SCC_FILETYPE_BINARY     2
#define SCC_CAP_RENAME          0x0002
typedef long (__cdecl *LPTEXTOUTPROC) (LPCSTR, DWORD);
typedef LPVOID LPCMDOPTS;

#define SCC_KEEP_CHECKEDOUT     0x1000

#define SCC_OK                  0

#define SCC_I_SHARESUBPROJOK				7
#define SCC_I_FILEDIFFERS						6
#define SCC_I_RELOADFILE						5
#define SCC_I_FILENOTAFFECTED                   4
#define SCC_I_PROJECTCREATED                    3
#define SCC_I_OPERATIONCANCELED                 2
#define SCC_I_ADV_SUPPORT                       1

#define SCC_E_INITIALIZEFAILED                  -1
#define SCC_E_UNKNOWNPROJECT                    -2
#define SCC_E_COULDNOTCREATEPROJECT             -3
#define SCC_E_NOTCHECKEDOUT                     -4
#define SCC_E_ALREADYCHECKEDOUT                 -5
#define SCC_E_FILEISLOCKED                      -6
#define SCC_E_FILEOUTEXCLUSIVE                  -7
#define SCC_E_ACCESSFAILURE                     -8
#define SCC_E_CHECKINCONFLICT                   -9
#define SCC_E_FILEALREADYEXISTS                 -10
#define SCC_E_FILENOTCONTROLLED                 -11
#define SCC_E_FILEISCHECKEDOUT                  -12
#define SCC_E_NOSPECIFIEDVERSION                -13
#define SCC_E_OPNOTSUPPORTED                    -14
#define SCC_E_NONSPECIFICERROR                  -15
#define SCC_E_OPNOTPERFORMED                    -16
#define SCC_E_TYPENOTSUPPORTED                  -17
#define SCC_E_VERIFYMERGE                       -18
#define SCC_E_FIXMERGE                          -19
#define SCC_E_SHELLFAILURE                      -20
#define SCC_E_INVALIDUSER                       -21
#define SCC_E_PROJECTALREADYOPEN                -22
#define SCC_E_PROJSYNTAXERR                     -23
#define SCC_E_INVALIDFILEPATH                   -24
#define SCC_E_PROJNOTOPEN                       -25
#define SCC_E_NOTAUTHORIZED                     -26
#define SCC_E_FILESYNTAXERR                     -27
#define SCC_E_FILENOTEXIST                      -28
#define SCC_E_CONNECTIONFAILURE                 -29
#define SCC_E_UNKNOWNERROR                      -30
#define SCC_E_BACKGROUNDGETINPROGRESS           -31

char sourceControlProviderDescription[SCC_NAME_LEN + 1];
char sourceControlAuxPathBuffer[SCC_AUXLABEL_LEN + 1];
char sourceControlUserName[SCC_USER_LEN + 1];
char sourceControlProjectName[SCC_PRJPATH_LEN + 1];
char sourceControlLocalPath[MAX_PATH];
char sourceControlAuxPath[SCC_PRJPATH_LEN + 1];
char sourceControlDllName[MAX_PATH];
HMODULE sourceControlDllHandle = 0;
HWND mainAppHwnd = 0;
LONG sccCaps, sccCheckoutCommentLen, sccCommentLen;
bool projectOpen = false;

typedef long __cdecl SccInitializeTypedef(LPVOID * ppContext, HWND hWnd, LPCSTR lpCallerName, LPSTR lpSccName, 
                        LPLONG lpSccCaps, LPSTR lpAuxPathLabel, LPLONG pnCheckoutCommentLen, LPLONG pnCommentLen);
typedef long __cdecl SccUnInitializeTypedef(LPVOID pContext);
typedef long __cdecl SccGetProjPathTypedef(LPVOID pvContext, HWND hWnd, LPSTR lpUser, LPSTR lpProjName,
						LPSTR lpLocalPath, LPSTR lpAuxProjPath, BOOL bAllowChangePath, LPBOOL pbNew);
typedef long __cdecl SccOpenProjectTypedef(LPVOID pContext, HWND hWnd, LPSTR lpUser, LPSTR lpProjName,
                        LPCSTR lpLocalProjPath, LPSTR lpAuxProjPath, LPCSTR lpComment, LPTEXTOUTPROC lpTextOutProc, LONG dwFlags);
typedef long __cdecl SccCloseProjectTypedef(LPVOID pContext);
typedef long __cdecl SccQueryInfoTypedef(LPVOID pContext, LONG nFiles, LPCSTR* lpFileNames, LPLONG lpStatus);
typedef long __cdecl SccAddTypedef(LPVOID pvContext, HWND hWnd, LONG nFiles, LPCSTR* lpFileNames, LPCSTR lpComment, LONG* pfOptions, LPCMDOPTS pvOptions);
typedef long __cdecl SccCheckinTypedef(LPVOID pvContext, HWND hWnd, LONG nFiles, LPSTR* lpFileNames, LPCSTR lpComment, LONG fOptions, LPCMDOPTS pvOptions);
typedef long __cdecl SccCheckoutTypedef(LPVOID pvContext, HWND hWnd, LONG nFiles, LPCSTR* lpFileNames, LPCSTR lpComment, LONG fOptions, LPCMDOPTS pvOptions);
typedef long __cdecl SccRenameTypedef(LPVOID pvContext, HWND hWnd, LPCSTR oldFileName, LPCSTR newFileName);
typedef long __cdecl SccRemoveTypedef(LPVOID pvContext, HWND hWnd, LONG nFiles, LPCSTR* lpFileNames, LPCSTR lpComment, LONG fOptions, LPCMDOPTS pvOptions);

long (__cdecl *SccGetVersion)(void);
SccInitializeTypedef *SccInitialize;
SccUnInitializeTypedef *SccUnInitialize;
SccGetProjPathTypedef *SccGetProjPath;
SccOpenProjectTypedef *SccOpenProject;
SccCloseProjectTypedef *SccCloseProject;
SccQueryInfoTypedef *SccQueryInfo;
SccAddTypedef *SccAdd;
SccCheckinTypedef *SccCheckin;
SccCheckoutTypedef *SccCheckout;
SccRenameTypedef *SccRename;
SccRemoveTypedef *SccRemove;
void *SccContext;


namespace AGS
{
	namespace Native
	{
		SourceCodeControl::SourceCodeControl(void)
		{
		}

		bool SourceCodeControl::Initialize(System::String^ dllName, int mainWindowHwnd)
		{
			mainAppHwnd = (HWND)mainWindowHwnd;
			ConvertFileNameToCharArray(dllName, sourceControlDllName);
			sourceControlDllHandle = LoadLibrary(sourceControlDllName);
			if (sourceControlDllHandle == NULL)
			{
				throw gcnew FileNotFoundException(String::Format("The DLL file {0} could not be loaded.", dllName));
			}
			SccGetVersion = (long (__cdecl *)())GetProcAddress(sourceControlDllHandle, "SccGetVersion");
			SccInitialize = (SccInitializeTypedef*)GetProcAddress(sourceControlDllHandle, "SccInitialize");
			SccUnInitialize = (SccUnInitializeTypedef*)GetProcAddress(sourceControlDllHandle, "SccUninitialize");
			SccGetProjPath = (SccGetProjPathTypedef*)GetProcAddress(sourceControlDllHandle, "SccGetProjPath");
			SccOpenProject = (SccOpenProjectTypedef*)GetProcAddress(sourceControlDllHandle, "SccOpenProject");
			SccCloseProject = (SccCloseProjectTypedef*)GetProcAddress(sourceControlDllHandle, "SccCloseProject");
			SccQueryInfo = (SccQueryInfoTypedef*)GetProcAddress(sourceControlDllHandle, "SccQueryInfo");
			SccAdd = (SccAddTypedef*)GetProcAddress(sourceControlDllHandle, "SccAdd");
			SccCheckin = (SccCheckinTypedef*)GetProcAddress(sourceControlDllHandle, "SccCheckin");
			SccCheckout = (SccCheckoutTypedef*)GetProcAddress(sourceControlDllHandle, "SccCheckout");
			SccRename = (SccRenameTypedef*)GetProcAddress(sourceControlDllHandle, "SccRename");
			SccRemove = (SccRemoveTypedef*)GetProcAddress(sourceControlDllHandle, "SccRemove");
			LONG version = SccGetVersion();
			if (version < 0x00010001)
			{
				FreeLibrary(sourceControlDllHandle);
				sourceControlDllHandle = 0;
				return false;
			}
			if (SccInitialize(&SccContext, mainAppHwnd, "Adventure Game Studio", 
				sourceControlProviderDescription, &sccCaps, sourceControlAuxPathBuffer,
				&sccCheckoutCommentLen, &sccCommentLen) != SCC_OK)
			{
				FreeLibrary(sourceControlDllHandle);
				sourceControlDllHandle = 0;
				return false;
			}
			return true;
		}

		void SourceCodeControl::Shutdown()
		{
			if (projectOpen)
			{
				projectOpen = false;
				SccCloseProject(SccContext);
			}
			SccUnInitialize(SccContext);
			FreeLibrary(sourceControlDllHandle);
			sourceControlDllHandle = 0;
		}

		SourceControlProject^ SourceCodeControl::AddToSourceControl()
		{
			_getcwd(sourceControlLocalPath, MAX_PATH);
			BOOL allowCreateNewProject = TRUE;
			if (SccGetProjPath(SccContext, mainAppHwnd, sourceControlUserName,
							sourceControlProjectName, sourceControlLocalPath,
							sourceControlAuxPath, FALSE, &allowCreateNewProject) == SCC_OK)
			{
				return gcnew SourceControlProject(gcnew System::String(sourceControlAuxPath), gcnew System::String(sourceControlProjectName));
			}
			return nullptr;
		}

		void SourceCodeControl::CloseProject()
		{
			if (projectOpen) 
			{
				SccCloseProject(SccContext);
				projectOpen = false;
			}
		}

		bool SourceCodeControl::OpenProject(SourceControlProject^ project)
		{
			this->CloseProject();

			_getcwd(sourceControlLocalPath, MAX_PATH);
			ConvertStringToCharArray(project->AuxPath, sourceControlAuxPath);
			ConvertStringToCharArray(project->ProjectName, sourceControlProjectName);

			if (SccOpenProject(SccContext, mainAppHwnd, sourceControlUserName,
							sourceControlProjectName, sourceControlLocalPath,
							sourceControlAuxPath, "", NULL, 0) == SCC_OK)
			{
				project->AuxPath = gcnew String(sourceControlAuxPath);
				projectOpen = true;
				return true;
			}
			return false;
		}

		cli::array<AGS::Types::SourceControlFileStatus>^ SourceCodeControl::GetFileStatuses(cli::array<System::String^> ^fileNames)
		{
			LPCSTR *fileNameList = new LPCSTR[fileNames->Length];
			LONG *statusCodes = new LONG[fileNames->Length];
			for (int i = 0; i < fileNames->Length; i++) 
			{
				fileNameList[i] = new char[fileNames[i]->Length + 1];
        ConvertFileNameToCharArray(fileNames[i], (char*)fileNameList[i]);
			}

      int errorCode;
			if ((errorCode = SccQueryInfo(SccContext, fileNames->Length, fileNameList, statusCodes)) != SCC_OK) 
			{
        throw gcnew AGS::Types::SourceControlException(errorCode, "SccQueryInfo");
			}

			cli::array<AGS::Types::SourceControlFileStatus> ^statuses = gcnew cli::array<AGS::Types::SourceControlFileStatus>(fileNames->Length);
			for (int i = 0; i < fileNames->Length; i++) 
			{
				statuses[i] = (AGS::Types::SourceControlFileStatus)statusCodes[i];
				delete fileNameList[i];
			}
			delete fileNameList;
			delete statusCodes;
			return statuses;
		}

		void SourceCodeControl::AddFilesToSourceControl(cli::array<System::String^> ^fileNames, System::String ^comment)
		{
			LPCSTR commentAsLpcstr = new char[comment->Length + 1];
			LPCSTR *fileNameList = new LPCSTR[fileNames->Length];
			LONG *options = new LONG[fileNames->Length];
			for (int i = 0; i < fileNames->Length; i++) 
			{
				options[i] = SCC_FILETYPE_AUTO;
				fileNameList[i] = new char[fileNames[i]->Length + 1];
        ConvertFileNameToCharArray(fileNames[i], (char*)fileNameList[i]);
			}
			ConvertStringToCharArray(comment, (char*)commentAsLpcstr);

      int errorCode;
			if ((errorCode = SccAdd(SccContext, mainAppHwnd, fileNames->Length, fileNameList, commentAsLpcstr, options, 0)) != SCC_OK) 
			{
        throw gcnew AGS::Types::SourceControlException(errorCode, "SccAdd");
			}

			for (int i = 0; i < fileNames->Length; i++) 
			{
				delete fileNameList[i];
			}
			delete fileNameList;
			delete options;
			delete commentAsLpcstr;
		}

		void SourceCodeControl::CheckInFiles(cli::array<System::String^> ^fileNames, System::String ^comment)
		{
			LPCSTR commentAsLpcstr = new char[comment->Length + 1];
			LPSTR *fileNameList = new LPSTR[fileNames->Length];
			for (int i = 0; i < fileNames->Length; i++) 
			{
				fileNameList[i] = new char[fileNames[i]->Length + 1];
        ConvertFileNameToCharArray(fileNames[i], (char*)fileNameList[i]);
			}
			ConvertStringToCharArray(comment, (char*)commentAsLpcstr);

      int errorCode = SccCheckin(SccContext, mainAppHwnd, fileNames->Length, fileNameList, commentAsLpcstr, 0, 0);
			if (errorCode != SCC_OK) 
			{
        throw gcnew AGS::Types::SourceControlException(errorCode, "SccCheckin");
			}

			for (int i = 0; i < fileNames->Length; i++) 
			{
				delete fileNameList[i];
			}
			delete fileNameList;
			delete commentAsLpcstr;
		}

		void SourceCodeControl::CheckOutFiles(cli::array<System::String^> ^fileNames, System::String ^comment)
		{
			LPCSTR commentAsLpcstr = new char[comment->Length + 1];
			LPCSTR *fileNameList = new LPCSTR[fileNames->Length];
			for (int i = 0; i < fileNames->Length; i++) 
			{
				fileNameList[i] = new char[fileNames[i]->Length + 1];
        ConvertFileNameToCharArray(fileNames[i], (char*)fileNameList[i]);
			}
			ConvertStringToCharArray(comment, (char*)commentAsLpcstr);

      int errorCode = SccCheckout(SccContext, mainAppHwnd, fileNames->Length, fileNameList, commentAsLpcstr, 0, 0);
			if (errorCode != SCC_OK) 
			{
        throw gcnew AGS::Types::SourceControlException(errorCode, "SccCheckout");
			}

			for (int i = 0; i < fileNames->Length; i++) 
			{
				delete fileNameList[i];
			}
			delete fileNameList;
			delete commentAsLpcstr;
		}

		void SourceCodeControl::RenameFile(System::String^ currentPath, System::String ^newPath)
		{
			if ((sccCaps & SCC_CAP_RENAME) == 0)
			{
				throw gcnew AGS::Types::AGSEditorException("The source control provider does not support renaming files");
			}

			char currentName[MAX_PATH];
			char newName[MAX_PATH];
      ConvertFileNameToCharArray(currentPath, currentName);
			ConvertFileNameToCharArray(newPath, newName);

      int errorCode = SccRename(SccContext, mainAppHwnd, currentName, newName);
			if (errorCode != SCC_OK) 
			{
        throw gcnew AGS::Types::SourceControlException(errorCode, "SccRename");
			}

		}

		void SourceCodeControl::DeleteFiles(cli::array<System::String^> ^fileNames, System::String ^comment)
		{
			LPCSTR commentAsLpcstr = new char[comment->Length + 1];
			LPCSTR *fileNameList = new LPCSTR[fileNames->Length];
			for (int i = 0; i < fileNames->Length; i++) 
			{
				fileNameList[i] = new char[fileNames[i]->Length + 1];
				ConvertFileNameToCharArray(fileNames[i], (char*)fileNameList[i]);
			}
			ConvertStringToCharArray(comment, (char*)commentAsLpcstr);

      int errorCode = SccRemove(SccContext, mainAppHwnd, fileNames->Length, fileNameList, commentAsLpcstr, 0, 0);
			if (errorCode != SCC_OK) 
			{
        throw gcnew AGS::Types::SourceControlException(errorCode, "SccRemove");
			}

			for (int i = 0; i < fileNames->Length; i++) 
			{
				delete fileNameList[i];
			}
			delete fileNameList;
			delete commentAsLpcstr;
		}

	}

}