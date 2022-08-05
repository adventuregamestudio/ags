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
#include "util/string_compat.h"
#include "util/cmdlineopts.h"

namespace AGS
{
namespace Common
{
namespace CmdLineOpts
{

ParseResult CmdLineOptsParser::Parse(
    int argc,
    const char *const argv[],
    const std::set<String>& opt_params_with_1_values,
    const std::set<String>& opt_params_with_2_values,
    const std::set<String>& opt_params_with_3_values,
    const std::set<String>& opt_params_with_4_values
)
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

        const bool is_opt_with_1_value = opt_params_with_1_values.find(arg) != opt_params_with_1_values.end();
        const bool is_opt_with_2_values = opt_params_with_2_values.find(arg) != opt_params_with_2_values.end();
        const bool is_opt_with_3_values = opt_params_with_3_values.find(arg) != opt_params_with_3_values.end();
        const bool is_opt_with_4_values = opt_params_with_4_values.find(arg) != opt_params_with_4_values.end();

        const bool is_simple_flag = (!is_opt_with_1_value && !is_opt_with_2_values && !is_opt_with_3_values && !is_opt_with_4_values);

        if(is_simple_flag)
        {
            bool cont = false;
            // check if it's written like `--optVal` instead of `--opt Val`
            for(const auto& opt_param_with_value : opt_params_with_1_values)
            {
                if(arg.StartsWith(opt_param_with_value))
                {
                    String val = arg;
                    val.ClipLeft(opt_param_with_value.GetLength());
                    parseResult.OptWith1Value.emplace_back(opt_param_with_value, val);
                    cont = true;
                    break;
                }
            }
            if(cont) continue;

            // check if it's written like `--optVal1 Val2` instead of `--opt Val1 Val2`
            for (const auto& opt_param_with_value : opt_params_with_2_values)
            {
                if (arg.StartsWith(opt_param_with_value))
                {
                    if (i >= args.size()) {
                        parseResult.IsBadlyFormed = true;
                        parseResult.BadlyFormedOption = opt_param_with_value;
                        return parseResult;
                    }

                    String val = arg;
                    val.ClipLeft(opt_param_with_value.GetLength());

                    auto values = std::vector<String>();
                    values.emplace_back(val);
                    values.emplace_back(args[i + 1]);
                    parseResult.OptWith2Values.emplace_back(opt_param_with_value, values);
                    ++i;
                    cont = true;
                    break;
                }
            }
            if (cont) continue;

            for (const auto& opt_param_with_value : opt_params_with_3_values)
            {
                if (arg.StartsWith(opt_param_with_value))
                {
                    if (i+1 >= args.size()) {
                        parseResult.IsBadlyFormed = true;
                        parseResult.BadlyFormedOption = opt_param_with_value;
                        return parseResult;
                    }

                    String val = arg;
                    val.ClipLeft(opt_param_with_value.GetLength());

                    auto values = std::vector<String>();
                    values.emplace_back(val);
                    values.emplace_back(args[i + 1]);
                    values.emplace_back(args[i + 2]);
                    parseResult.OptWith3Values.emplace_back(opt_param_with_value, values);
                    i+=2;
                    cont = true;
                    break;
                }
            }
            if (cont) continue;

            for (const auto& opt_param_with_value : opt_params_with_4_values)
            {
                if (arg.StartsWith(opt_param_with_value))
                {
                    if (i+2 >= args.size()) {
                        parseResult.IsBadlyFormed = true;
                        parseResult.BadlyFormedOption = opt_param_with_value;
                        return parseResult;
                    }

                    String val = arg;
                    val.ClipLeft(opt_param_with_value.GetLength());

                    auto values = std::vector<String>();
                    values.emplace_back(val);
                    values.emplace_back(args[i + 1]);
                    values.emplace_back(args[i + 2]);
                    values.emplace_back(args[i + 3]);
                    parseResult.OptWith4Values.emplace_back(opt_param_with_value, values);
                    i += 3;
                    cont = true;
                    break;
                }
            }
            if (cont) continue;

            // it's just a regular `--opt` that has no values
            parseResult.Opt.emplace(arg);
            continue;
        }

        // picks up pattern `--opt Val`
        if (is_opt_with_1_value) {
            if (i+1 >= args.size()) {
                parseResult.IsBadlyFormed = true;
                parseResult.BadlyFormedOption = arg;
                return parseResult;
            }

            parseResult.OptWith1Value.emplace_back(arg, args[i + 1]);
            ++i; // consume next value
            continue;
        }

        // picks up pattern `--opt Val1 Val2`
        if (is_opt_with_2_values) {
            if (i + 2 >= args.size()) {
                parseResult.IsBadlyFormed = true;
                parseResult.BadlyFormedOption = arg;
                return parseResult;
            }

            auto values = std::vector<String>();
            values.emplace_back(args[i + 1]);
            values.emplace_back(args[i + 2]);
            parseResult.OptWith2Values.emplace_back(arg, values);
            i += 2; // consume 2 subsequent values
            continue;
        }

        // picks up pattern `--opt Val1 Val2 Val3`
        if (is_opt_with_3_values) {
            if (i + 3 >= args.size()) {
                parseResult.IsBadlyFormed = true;
                parseResult.BadlyFormedOption = arg;
                return parseResult;
            }

            auto values = std::vector<String>();
            values.emplace_back(args[i + 1]);
            values.emplace_back(args[i + 2]);
            values.emplace_back(args[i + 3]);
            parseResult.OptWith2Values.emplace_back(arg, values);
            i += 3; // consume 3 subsequent values
            continue;
        }

        // picks up pattern `--opt Val1 Val2 Val3 Val4`
        if (is_opt_with_4_values) {
            if (i + 4 >= args.size()) {
                parseResult.IsBadlyFormed = true;
                parseResult.BadlyFormedOption = arg;
                return parseResult;
            }

            auto values = std::vector<String>();
            values.emplace_back(args[i + 1]);
            values.emplace_back(args[i + 2]);
            values.emplace_back(args[i + 3]);
            values.emplace_back(args[i + 4]);
            parseResult.OptWith2Values.emplace_back(arg, values);
            i += 4; // consume 4 subsequent values
            continue;
        }
    }

    return parseResult;
}

} // namespace CmdLineOpts

} // namespace Common
} // namespace AGS
