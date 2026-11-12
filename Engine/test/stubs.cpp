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
//
// Do-nothing replacements of the engine functions that the tested sources
// call, for a test program that compiles these sources alone and does not link
// the engine library (Engine.App.Test in Solutions/Tests.App).
// A test program that links the engine (CMake's engine_test) must not have
// this file: the library has the real functions.
//
//=============================================================================
#include "debug/debug_log.h"
#include "script/cc_common.h"

using namespace AGS::Common;

void cc_error(const char *, ...)
{
    // do nothing
}

void debug_script_event(MessageType /*mt*/, const String & /*msg*/)
{
    // do nothing
}
