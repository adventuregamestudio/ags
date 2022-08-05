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

#include "core/platform.h"
#include <set>
#include <stdio.h>
#include "ac/common.h"
#include "engine_cmdline.h"
#include <util/ini_util.h>


using namespace AGS::Common;

namespace AGS
{
namespace Engine
{
namespace CmdLineOpts
{
    const char* EngineCmdLineOpts::_helpText = R"EOS(
        "Usage: ags [OPTIONS] [GAMEFILE or DIRECTORY]\n\n"
        //--------------------------------------------------------------------------------|
        "Options:\n"
        "  --background                 Keeps game running in background\n"
        "                               (this does not work in exclusive fullscreen)\n"
        "  --clear-cache-on-room-change Clears sprite cache on every room change\n"
        "  --conf FILEPATH              Specify explicit config file to read on startup\n"
#if AGS_PLATFORM_OS_WINDOWS
        "  --console-attach             Write output to the parent process's console\n"
#endif
        "  --fps                        Display fps counter\n"
        "  --fullscreen                 Force display mode to fullscreen\n"
        "  --gfxdriver <id>             Request graphics driver. Available options:\n"
#if AGS_PLATFORM_OS_WINDOWS
        "                                 d3d9, ogl, software\n"
#else
        "                                 ogl, software\n"
#endif
        "  --gfxfilter FILTER [SCALING]\n"
        "                               Request graphics filter. Available options:\n"
        "                                 linear, none, stdscale\n"
        "                                 (support differs between graphic drivers);\n"
        "                                 scaling is specified by integer number\n"
        "  --help                       Print this help message and stop\n"
        "  --loadsavedgame FILEPATH     Load savegame on startup\n"
        "  --localuserconf              Read and write user config in the game's \n"
        "                               directory rather than using standard system path.\n"
        "                               Game directory must be writeable.\n"
        "  --log-OUTPUT=GROUP[:LEVEL][,GROUP[:LEVEL]][,...]\n"
        "  --log-OUTPUT=+GROUPLIST[:LEVEL]\n"
        "                               Setup logging to the chosen OUTPUT with given\n"
        "                               log groups and verbosity levels. Groups may\n"
        "                               be also defined by a LIST of one-letter IDs,\n"
        "                               preceded by '+', e.g. +ABCD:LEVEL. Verbosity may\n"
        "                               be also defined by a numberic ID.\n"
        "                               OUTPUTs are\n"
        "                                 stdout, file, console\n"
        "                               (where \"console\" is internal engine's console)\n"
        "                               GROUPs are:\n"
        "                                 all, main (m), game (g), manobj (o),\n"
        "                                 script (s), sprcache (c)\n"
        "                               LEVELs are:\n"
        "                                 all, alert (1), fatal (2), error (3), warn (4),\n"
        "                                 info (5), debug (6)\n"
        "                               Examples:\n"
        "                                 --log-stdout=+mgs:debug\n"
        "                                 --log-file=all:warn\n"
        "  --log-file-path=PATH         Define custom path for the log file\n"
        //--------------------------------------------------------------------------------|
#if AGS_PLATFORM_OS_WINDOWS
        "  --no-message-box             Disable alerts as modal message boxes\n"
#endif
        "  --no-translation             Use default game language on start\n"
        "  --noiface                    Don't draw game GUI\n"
        "  --noscript                   Don't run room scripts; *WARNING:* unreliable\n"
        "  --nospr                      Don't draw room objects and characters\n"
        "  --noupdate                   Don't run game update\n"
        "  --novideo                    Don't play game videos\n"
        "  --rotation <MODE>            Screen rotation preferences. MODEs are:\n"
        "                                 unlocked (0), portrait (1), landscape (2)\n"
        "  --sdl-log=LEVEL              Setup SDL backend logging level\n"
        "                               LEVELs are:\n"
        "                                 verbose (1), debug (2), info (3), warn (4),\n"
        "                                 error (5), critical (6)\n"
#if AGS_PLATFORM_OS_WINDOWS
        "  --setup                      Run setup application\n"
#endif
        "  --shared-data-dir DIR        Set the shared game data directory\n"
        "  --startr <room_number>       Start game by loading certain room.\n"
        "  --tell                       Print various information concerning engine\n"
        "                                 and the game; for selected output use:\n"
        "  --tell-config                Print contents of merged game config\n"
        "  --tell-configpath            Print paths to available config files\n"
        "  --tell-data                  Print information on game data and its location\n"
        "  --tell-gameproperties        Print information on game general settings\n"
        "  --tell-engine                Print engine name and version\n"
        "  --tell-filepath              Print all filepaths engine uses for the game\n"
        "  --tell-graphicdriver         Print list of supported graphic drivers\n"
        "\n"
        "  --test                       Run game in the test mode\n"
        "  --translation <name>         Select the given translation on start\n"
        "  --version                    Print engine's version and stop\n"
        "  --user-data-dir DIR          Set the save game directory\n"
        "  --windowed                   Force display mode to windowed\n"
        "\n"
        "Gamefile options:\n"
        "  /dir/path/game/              Launch the game in specified directory\n"
        "  /dir/path/game/penguin.exe   Launch penguin.exe\n"
        "  [nothing]                    Launch the game in the current directory\n"
        //--------------------------------------------------------------------------------|
    )EOS";

	void EngineCmdLineOpts::Convert(EngineParsedOptions& options, ParseResult& cmdLineOpts)
	{
		ParseResult result;

        //TODO : Make this replace main_process_cmdline
	}

	const char* EngineCmdLineOpts::GetHelpText()
	{
        return _helpText;
	}

} // namespace CmdLineOpts
} // namespace Common
} // namespace AGS
