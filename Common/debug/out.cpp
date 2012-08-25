
//=============================================================================
//
// AGS logging system
//
//-----------------------------------------------------------------------------
// TODO:
// Use advanced utility classes instead of C-style strings and arrays.      
//
//=============================================================================

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "debug/out.h"
#include "debug/outputtarget.h"

#define STD_BUFFER_SIZE 3000

namespace AGS
{
namespace Common
{

namespace out
{
    struct OUTPUT_TARGET
    {
        IOutputTarget   *delegate_object;
        // target-specific verbosity setting
        int             verbosity;
        // TODO: use shared ptrs instead of remembering that ptr is shared
        bool            shared;
        bool            suppressed;

        //char            buffer[STD_BUFFER_SIZE];

        OUTPUT_TARGET()
        {
            delegate_object = NULL;
            verbosity       = 0;
            shared          = false;
            suppressed      = false;
        }

        ~OUTPUT_TARGET()
        {
            if (!shared) {
                delete delegate_object;
            }
        }
    };

    struct INTERNAL_DATA
    {
        // general verbosity setting
        int             verbosity;
        OUTPUT_TARGET   targets[MAX_TARGETS];

        INTERNAL_DATA()
        {
            verbosity = 0;
        }
    };

    // Internal va_list version of out()
    void vout (int reason, const char *sz_msg, va_list argptr);
    // Internal va_list version of out with additional function name string
    void voutfn (int reason, const char *sz_fnname, const char *sz_msg, va_list argptr);
    // Internal va_list wrapper over print()
    void vprint(int reason, const char *sz_msg, va_list argptr);
    // Does actual print
    void print (int reason, const char *sz_msg);

    // Check verbosity settings to check if it is permitted to
    // print a message for given reason
    inline bool get_output_permission (int verbosity, int reason = REASON_WHATEVER);

}   // namespace out

}   // namespace Common
}   // namespace AGS

using namespace AGS;
using namespace AGS::Common;

out::INTERNAL_DATA idata;

//-----------------------------------------------------------------------------
// System management
//-----------------------------------------------------------------------------
void out::init (int cmdarg_count, char **cmdargs)
{
    // TODO: check cmdargs to set up the settings
    //
    //
    idata.verbosity = REASON_WHATEVER; /*play.debug_mode != 0 ? REASON_WHATEVER : 0;*/

    //-------------------------------------------
    //
}

void out::add_output_target(int target_id, out::IOutputTarget *output_target, int verbosity, bool shared_ptr)
{
    // TODO: use array class instead
    if (target_id < 0 || target_id >= MAX_TARGETS) {
        out::warn("Output system: unable to add target output, because id %d is out of range", target_id);
        return;
    }

    if (idata.targets[target_id].shared) {
        delete idata.targets[target_id].delegate_object;
        idata.targets[target_id].delegate_object = NULL;
    }

    idata.targets[target_id].shared         = shared_ptr;
    idata.targets[target_id].verbosity      = verbosity;
    idata.targets[target_id].delegate_object= output_target;
}

void out::shutdown ()
{
    // release any memory, resources etc here

    // TODO: this should be handled by shared pointers!
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (!idata.targets[i].shared) {
            delete idata.targets[i].delegate_object;
            idata.targets[i].delegate_object = NULL;
        }
    }
}

//-----------------------------------------------------------------------------
// Verbosity setting
//-----------------------------------------------------------------------------
inline bool out::get_output_permission (int verbosity, int reason /* = REASON_WHATEVER */)
{
    // verbosity is tested against VERBOSE_DO_LOG separately,
    // since reason is not required to has that bit set
    return (verbosity & VERBOSE_DO_LOG) != 0 && (verbosity & reason) != 0;
}

//-----------------------------------------------------------------------------
// Convenience functions, with regard to verbosity settings
//-----------------------------------------------------------------------------
void out::debug (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vout(REASON_DEBUG, sz_msg, argptr);
    va_end(argptr);
}

void out::debug (const char *sz_fnname, const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::voutfn(REASON_DEBUG, sz_fnname, sz_msg, argptr);
    va_end(argptr);
}

void out::notify (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vout(REASON_NOTIFICATION, sz_msg, argptr);
    va_end(argptr);
}

void out::notify (const char *sz_fnname, const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::voutfn(REASON_NOTIFICATION, sz_fnname, sz_msg, argptr);
    va_end(argptr);
}

void out::warn (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vout(REASON_WARNING, sz_msg, argptr);
    va_end(argptr);
}

void out::warn (const char *sz_fnname, const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::voutfn(REASON_WARNING, sz_fnname, sz_msg, argptr);
    va_end(argptr);
}

void out::handled_err (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vout(REASON_HANDLED_ERROR, sz_msg, argptr);
    va_end(argptr);
}

void out::handled_err (const char *sz_fnname, const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::voutfn(REASON_HANDLED_ERROR, sz_fnname, sz_msg, argptr);
    va_end(argptr);
}

void out::unhandled_err (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vout(REASON_UNHANDLED_ERROR, sz_msg, argptr);
    va_end(argptr);
}

void out::unhandled_err (const char *sz_fnname, const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::voutfn(REASON_UNHANDLED_ERROR, sz_fnname, sz_msg, argptr);
    va_end(argptr);
}

void out::fatal_err (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vout(REASON_FATAL_ERROR, sz_msg, argptr);
    va_end(argptr);
}

void out::fatal_err (const char *sz_fnname, const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::voutfn(REASON_FATAL_ERROR, sz_fnname, sz_msg, argptr);
    va_end(argptr);
}

void out::fnin (const char *sz_fnname)
{
    out::out(REASON_FUNCTION_ENTER, ">>> function: %s", sz_fnname);
}

void out::fnout (const char *sz_fnname)
{
    out::out(REASON_FUNCTION_EXIT, "<<< function: %s", sz_fnname);
}

//-----------------------------------------------------------------------------
// Make an output with regard to verbosity settings
//-----------------------------------------------------------------------------
void out::out (int reason, const char *sz_msg, ...)
{
    if (!get_output_permission(idata.verbosity, reason)) {
        return;
    }

    va_list argptr;
    va_start(argptr, sz_msg);
    out::vprint(reason, sz_msg, argptr);
    va_end(argptr);
}

void out::vout (int reason, const char *sz_msg, va_list argptr)
{
    if (!get_output_permission(idata.verbosity, reason)) {
        return;
    }

    out::vprint(reason, sz_msg, argptr);
}

void out::voutfn (int reason, const char *sz_fnname, const char *sz_msg, va_list argptr)
{
    if (!get_output_permission(idata.verbosity, reason)) {
        return;
    }

    char buffer[STD_BUFFER_SIZE];
    int fnname_length = strlen(sz_fnname);
    sprintf(buffer, "@%s: ", sz_fnname);
    vsprintf(buffer + fnname_length + 3, sz_msg, argptr);

    out::print(reason, buffer);
}

//-----------------------------------------------------------------------------
// Force print: make an output regardless of verbosity settings
//-----------------------------------------------------------------------------
void out::fprint (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    out::vprint(REASON_WHATEVER, sz_msg, argptr);
    va_end(argptr);
}

//-----------------------------------------------------------------------------
// Do actual print
//-----------------------------------------------------------------------------
void out::vprint (int reason, const char *sz_msg, va_list argptr)
{
    // Make a formatted string
    char buffer[STD_BUFFER_SIZE];
    vsprintf(buffer, sz_msg, argptr);

    // TODO: Optionally add information here (room index, etc) (?)
    //
    //
    // Send final message to targets
    print(reason, buffer);
}

void out::print (int reason, const char *sz_msg)
{    
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        if (idata.targets[i].delegate_object) {
            if (idata.targets[i].suppressed) {
                continue;
            }
            if (!get_output_permission(idata.targets[i].verbosity, reason)) {
                continue;
            }

            // We suppress current target here  so that if it makes a call
            // to output system itself, message would not print to the
            // same target
            idata.targets[i].suppressed = true;
            idata.targets[i].delegate_object->out(sz_msg);
            idata.targets[i].suppressed = false;
        }
    }
} 
