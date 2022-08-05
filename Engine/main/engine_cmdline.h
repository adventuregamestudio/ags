//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__CMDLINE_H
#define __AGS_EE_MAIN__CMDLINE_H

#include "util/cmdlineopts.h"
#include "util/ini_util.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;

namespace AGS
{
namespace Engine
{
namespace CmdLineOpts
{

    // Container struct to store command line options for the Engine
    struct EngineParsedOptions {

        // Options that get stored in a config tree
        AGS::Common::ConfigTree cfg;

        String CmdGameDataPath;

        //Misc options (maybe should be moved to the config tree?)
        int DebugFlags = 0;
        int OverrideStartRoom = 0;
        String LoadSaveGameOnStartup;
        String EditorDebuggerInstanceToken;
        bool EditorDebuggingEnabled = false;
        std::set<String> TellInfoKeys;
        bool AttachToParentConsole = false;
        bool HideMessageBoxes = false;

        //User setup options
        bool   UserSetup_DisableExceptionHandling = false;
        String UserSetup_ConfPath;
        String UserSetup_LocalUserConfPath;
        String UserSetup_UserConfPath;

        // Implicit logic inferred from the actual options
        bool JustDisplayHelp = false;
        bool JustDisplayVersion = false;
        bool JustRunSetup = false;
        bool Exit = false;
        int ErrorCode = 0;

        EngineParsedOptions() = default;
        explicit EngineParsedOptions(int error_code) { Exit = true; ErrorCode = error_code; }
    };

	class EngineCmdLineOpts
	{
    public:

        // Converts a generic set of command line options to AGS engine-specific options
        static void Convert(EngineParsedOptions& options, ParseResult& cmdLineOpts);

        // Returns the help text to display in a terminal for the AGS engine
        static const char* GetHelpText();

    private:
        static const char* _helpText;

	};

} // namespace CmdLineOpts
} // namespace Common
} // namespace AGS

#endif  // __AGS_EE_MAIN__CMDLINE_H

