
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
// 1. Use advanced utility classes instead of C-style strings and arrays.    
//
//=============================================================================
#ifndef __AGS_CN_DEBUG__OUT_H
#define __AGS_CN_DEBUG__OUT_H

#include "debug/outputtarget.h"

namespace AGS
{
namespace Common
{

namespace out
{
    // Verbosity
    enum
    {
        VERBOSE_DO_LOG          = 0x0001, // do log (otherwise do not, C.O.)

        // Debug reason is for random information about current game state,
        // like outputting object values, or telling that execution has
        // passed certain point in the function
        REASON_DEBUG            = 0x0002,
        // Notifications are made when the program is passing important
        // checkpoints, like initializing or shutting engine down, or
        // when program decides to make a workaround to solve some problem
        REASON_NOTIFICATION     = 0x0004,
        // Warnings are made when unexpected or generally strange behavior
        // is detected in program, which does not necessarily mean error,
        // but still may be a symptom of a bigger problem
        REASON_WARNING          = 0x0008,
        // Handled errors are ones that were 'fixed' by the program on run;
        // There's high chance that program will continue running as normal,
        // but message should be maid to bring user or dev's attention
        REASON_HANDLED_ERROR    = 0x0010,
        // Unhandled errors are errors that program was not able to fix;
        // Program *may* try to continue, but there's no guarantee it will
        // work as expected
        REASON_UNHANDLED_ERROR  = 0x0020,
        // Fatal errors make program abort immediately
        REASON_FATAL_ERROR      = 0x0040,

        // Convenience aliases
        REASON_FUNCTION_ENTER   = REASON_DEBUG,
        REASON_FUNCTION_EXIT    = REASON_DEBUG,

        REASON_NOT_DEBUG        = 0x007D,
        REASON_WARN_ERRORS      = 0x0079,
        // Just print out the damned message!
        REASON_WHATEVER         = 0x00FF
    };

    const int MAX_TARGETS = 10; // TODO: remove this when array classes are implemented

    // System management
    void init (int cmdarg_count, char **cmdargs);
    // TODO: use shared ptrs instead of telling that ptr is shared
    void add_output_target(int target_id, out::IOutputTarget *output_target, int verbosity, bool shared_ptr);
    void shutdown ();

    // Convenience functions, with regard to verbosity settings
    void debug          (const char *sz_msg, ...);
    void debug          (const char *sz_fnname, const char *sz_msg, ...);
    void notify         (const char *sz_msg, ...);
    void notify         (const char *sz_fnname, const char *sz_msg, ...);
    void warn           (const char *sz_msg, ...);
    void warn           (const char *sz_fnname, const char *sz_msg, ...);
    void handled_err    (const char *sz_msg, ...);
    void handled_err    (const char *sz_fnname, const char *sz_msg, ...);
    void unhandled_err  (const char *sz_msg, ...);
    void unhandled_err  (const char *sz_fnname, const char *sz_msg, ...);
    void fatal_err      (const char *sz_msg, ...);
    void fatal_err      (const char *sz_fnname, const char *sz_msg, ...);
    void fnin           (const char *sz_fnname);
    void fnout          (const char *sz_fnname);

    // Make an output with regard to verbosity settings
    void out   (int reason, const char *sz_msg, ...);

    // Force print: output message regardless of verbosity settings
    // This will make an output even if VERBOSE_DO_LOG is not set
    void fprint(const char *sz_msg, ...);

}   // namespace out

}   // namespace Common
}   // namespace AGS

#endif // __AGS_CN_DEBUG__OUT_H
