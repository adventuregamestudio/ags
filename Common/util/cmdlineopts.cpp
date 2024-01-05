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

#include "core/platform.h"
#include "util/string_compat.h"
#include "util/cmdlineopts.h"

namespace AGS
{
namespace Common
{

namespace CmdLineOpts
{

ParseResult Parse(int argc, const char *const argv[], const std::set<String> &opt_params_with_values)
{
    ParseResult parseResult;

    std::vector<String> args(argv + 1, argv + argc);

    // on windows, a person may use `/?` or `-?` when trying to ask for help to a command line program
    for (auto & arg : args)
    {
        if (arg == "-h" ||  arg == "--help" 
#if AGS_PLATFORM_OS_WINDOWS
            || arg == "/?" || arg == "-?"
#endif
            )
        {
            parseResult.HelpRequested = true;
            break; // we don't fully exit here in case this is used in a command line that requires `--console-attach`
        }
    }

    for (size_t i = 0; i < args.size(); i++)
    {
        String arg = args[i];

        if (arg[0] != '-')
        {
            parseResult.PosArgs.emplace_back(args[i]);
            continue;
        }

        const bool is_opt_with_value = opt_params_with_values.find(arg) != opt_params_with_values.end();
        if(!is_opt_with_value)
        {
            bool cont = false;
            // check if it's written like `--optVal` instead of `--opt Val`
            for(const auto& opt_param_with_value : opt_params_with_values)
            {
                if(arg.StartsWith(opt_param_with_value))
                {
                    String val = arg;
                    val.ClipLeft(opt_param_with_value.GetLength());
                    parseResult.OptWithValue.emplace_back(opt_param_with_value, val);
                    cont = true;
                    break;
                }
            }
            if(cont) continue;

            // it's just a regular `--opt` that has no values
            parseResult.Opt.emplace(arg);
            continue;
        }

        // picks up `--opt Val`
        parseResult.OptWithValue.emplace_back(arg, args[i + 1]);
        ++i; // skip next value, it is not a free parameter
    }

    return parseResult;
}

} // namespace CmdLineOpts

} // namespace Common
} // namespace AGS