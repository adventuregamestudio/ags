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
// AGS logging system
//
// [IKM] 2012-06-23
// Logging system provides a simple way to output debug information.
// It is being controlled internally by two general settings:
// Verbosity - which defines what types of information should be actually
// written, and what discarded, and
// Target    - which determines an output destination between
//      log file,
//      console,
//      editor debugger (via IAGSEditorDebugger interface),
//      in-game message system,
//      anything else whichever is implemented.
//
// The calling unit should not worry or try to know anything about how
// the output works. I should simply call one of its exported functions
// when needed.
//
//-----------------------------------------------------------------------------
// TODO:
// + Use advanced utility classes instead of C-style strings and arrays.    
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__OUT_H
#define __AGS_CN_DEBUG__OUT_H

#include "debug/outputtarget.h"

namespace AGS
{
namespace Common
{

namespace Out
{
    // Verbosity
    enum OutputVerbosity
    {
        kVerbose_Never          = 0,
        kVerbose_DoLog          = 0x0001, // do log (otherwise do not, C.O.)

        // Debug reason is for random information about current game state,
        // like outputting object values, or telling that execution has
        // passed certain point in the function
        kVerbose_Debug          = 0x0002,
        // Notifications are made when the program is passing important
        // checkpoints, like initializing or shutting engine down, or
        // when program decides to make a workaround to solve some problem
        kVerbose_Notification   = 0x0004,
        // Warnings are made when unexpected or generally strange behavior
        // is detected in program, which does not necessarily mean error,
        // but still may be a symptom of a bigger problem
        kVerbose_Warning        = 0x0008,
        // Handled errors are ones that were 'fixed' by the program on run;
        // There's high chance that program will continue running as normal,
        // but message should be maid to bring user or dev's attention
        kVerbose_HandledError   = 0x0010,
        // Unhandled errors are errors that program was not able to fix;
        // Program *may* try to continue, but there's no guarantee it will
        // work as expected
        kVerbose_UnhandledError = 0x0020,
        // Fatal errors make program abort immediately
        kVerbose_FatalError     = 0x0040,

        // Convenience aliases
        kVerbose_NoDebug        = 0x007D,
        kVerbose_WarnErrors     = 0x0079,
        // Just print out the damned message!
        kVerbose_Always         = 0x00FF
    };

    const int MAX_TARGETS = 10; // TODO: remove this when array classes are implemented

    // System management
    void Init (int cmdarg_count, char **cmdargs);
    // TODO: use safer technique like shared ptrs or reference counted objects
    // instead of telling that ptr is shared
    void AddOutputTarget(int target_id, IOutputTarget *output_target, OutputVerbosity verbosity, bool shared_object);
    void RemoveOutputTarget(int target_id);
    void Shutdown ();

    // Convenience functions, with regard to verbosity settings
    void Debug          (const char *sz_msg, ...);
    void Notify         (const char *sz_msg, ...);
    void Warn           (const char *sz_msg, ...);
    void HandledError   (const char *sz_msg, ...);
    void UnhandledError (const char *sz_msg, ...);
    void FatalError     (const char *sz_msg, ...);

    // Make an output with regard to verbosity settings
    void Out   (OutputVerbosity reason, const char *sz_msg, ...);

    // Force print: output message regardless of verbosity settings
    // This will make an output even if kVerbose_DoLog is not set
    void FPrint(const char *sz_msg, ...);

}   // namespace Out

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__OUT_H
