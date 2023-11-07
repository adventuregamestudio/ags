/*
// 'C'-style script compiler
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __CS_COMPILER2_H
#define __CS_COMPILER2_H

#include <string>
#include <vector>
#include "script/cc_script.h"
#include "cs_message_handler.h"

// Get a list of compiler extensions.
extern void ccGetExtensions2(std::vector<std::string> &exts);
// compile the script supplied, returns nullptr on failure
// cc_error() gets called.
extern ccScript *ccCompileText2(std::string const &script, std::string const &scriptName, uint64_t options, MessageHandler &mh);

#endif // __CS_COMPILER2_H
