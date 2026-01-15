//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __CS_COMPILER2_H
#define __CS_COMPILER2_H

#include <string>
#include <vector>
#include "script/cc_script.h"
#include "cs_message_handler.h"

// Get a list of compiler extensions.
extern void ccGetExtensions2(std::vector<std::string> &exts);
// Compile the script supplied, returns nullptr on failure.
// All compiler's errors and warnings are returned as a MessageHandler collection.
extern ccScript *ccCompileText2(std::string const &script, std::string const &scriptName, uint64_t options, MessageHandler &mh);

#endif // __CS_COMPILER2_H
