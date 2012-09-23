
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

namespace Out
{
    struct COutputTargetSlot
    {
    public:
        IOutputTarget   *DelegateObject;
        // target-specific verbosity setting
        OutputVerbosity Verbosity;
        // TODO: use safer technique instead of remembering that ptr is shared
        bool            IsShared;
        bool            IsSuppressed;

        COutputTargetSlot(IOutputTarget *output_target, OutputVerbosity verbosity, bool shared_object)
        {
            DelegateObject  = output_target;
            Verbosity       = verbosity;
            IsShared        = shared_object;
            IsSuppressed    = false;
        }

        ~COutputTargetSlot()
        {
            if (!IsShared) {
                delete DelegateObject;
            }
        }
    };

    struct CInternalData
    {
    public:
        // general verbosity setting
        OutputVerbosity     Verbosity;
        COutputTargetSlot   *Targets[MAX_TARGETS];

        CInternalData()
        {
            Verbosity = kVerbose_Never;
            memset(Targets, NULL, sizeof(Targets));
        }
    };

    // Internal va_list version of Out()
    void VOut  (OutputVerbosity reason, const char *sz_msg, va_list argptr);
    // Internal va_list wrapper over print()
    void VPrint(OutputVerbosity reason, const char *sz_msg, va_list argptr);
    // Does actual print
    void Print (OutputVerbosity reason, const char *sz_msg);

    // Check verbosity settings to check if it is permitted to
    // print a message for given reason
    inline bool GetOutputPermission (OutputVerbosity verbosity, OutputVerbosity reason = kVerbose_Always);

}   // namespace Out

}   // namespace Common
}   // namespace AGS

namespace Out = AGS::Common::Out;

Out::CInternalData IData;

//-----------------------------------------------------------------------------
// System management
//-----------------------------------------------------------------------------
void Out::Init (int cmdarg_count, char **cmdargs)
{
    // TODO: check cmdargs to set up the settings
    //
    //
    IData.Verbosity = kVerbose_Always; /*play.debug_mode != 0 ? kVerbose_Always : 0;*/

    //-------------------------------------------
    //
}

void Out::AddOutputTarget(int target_id, Out::IOutputTarget *output_target, OutputVerbosity verbosity, bool shared_object)
{
    // TODO: use array class instead
    if (target_id < 0 || target_id >= MAX_TARGETS) {
        Out::Warn("Output system: unable to add target output, because id %d is Out of range", target_id);
        return;
    }

    delete IData.Targets[target_id];
    IData.Targets[target_id] = new COutputTargetSlot(output_target, verbosity, shared_object);
}

void Out::Shutdown ()
{
    // release any memory, resources etc here

    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        delete IData.Targets[i];
        IData.Targets[i] = NULL;
    }
}

//-----------------------------------------------------------------------------
// Verbosity setting
//-----------------------------------------------------------------------------
inline bool Out::GetOutputPermission (OutputVerbosity verbosity, OutputVerbosity reason)
{
    // verbosity is tested against kVerbose_DoLog separately,
    // since reason is not required to has that bit set
    return (verbosity & kVerbose_DoLog) != 0 && (verbosity & reason) != 0;
}

//-----------------------------------------------------------------------------
// Convenience functions, with regard to verbosity settings
//-----------------------------------------------------------------------------
void Out::Debug (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VOut(kVerbose_Debug, sz_msg, argptr);
    va_end(argptr);
}

void Out::Notify (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VOut(kVerbose_Notification, sz_msg, argptr);
    va_end(argptr);
}

void Out::Warn (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VOut(kVerbose_Warning, sz_msg, argptr);
    va_end(argptr);
}

void Out::HandledError (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VOut(kVerbose_HandledError, sz_msg, argptr);
    va_end(argptr);
}

void Out::UnhandledError (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VOut(kVerbose_UnhandledError, sz_msg, argptr);
    va_end(argptr);
}

void Out::FatalError(const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VOut(kVerbose_FatalError, sz_msg, argptr);
    va_end(argptr);
}

//-----------------------------------------------------------------------------
// Make an output with regard to verbosity settings
//-----------------------------------------------------------------------------
void Out::Out (OutputVerbosity reason, const char *sz_msg, ...)
{
    if (!GetOutputPermission(IData.Verbosity, reason)) {
        return;
    }

    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VPrint(reason, sz_msg, argptr);
    va_end(argptr);
}

void Out::VOut (OutputVerbosity reason, const char *sz_msg, va_list argptr)
{
    if (!GetOutputPermission(IData.Verbosity, reason)) {
        return;
    }

    Out::VPrint(reason, sz_msg, argptr);
}

//-----------------------------------------------------------------------------
// Force print: make an output regardless of verbosity settings
//-----------------------------------------------------------------------------
void Out::FPrint (const char *sz_msg, ...)
{
    va_list argptr;
    va_start(argptr, sz_msg);
    Out::VPrint(kVerbose_Always, sz_msg, argptr);
    va_end(argptr);
}

//-----------------------------------------------------------------------------
// Do actual print
//-----------------------------------------------------------------------------
void Out::VPrint (OutputVerbosity reason, const char *sz_msg, va_list argptr)
{
    // Make a formatted string
    char buffer[STD_BUFFER_SIZE];
    vsprintf(buffer, sz_msg, argptr);

    // TODO: Optionally add information here (room index, etc) (?)
    //
    //
    // Send final message to Targets
    Print(reason, buffer);
}

void Out::Print (OutputVerbosity reason, const char *sz_msg)
{    
    for (int i = 0; i < MAX_TARGETS; ++i)
    {
        COutputTargetSlot *target = IData.Targets[i];
        if (!target || !target->DelegateObject)
        {
            continue;
        }

        if (target->IsSuppressed) {
            continue;
        }
        if (!GetOutputPermission(target->Verbosity, reason)) {
            continue;
        }

        // We suppress current target here  so that if it makes a call
        // to output system itself, message would not print to the
        // same target
        target->IsSuppressed = true;
        target->DelegateObject->Out(sz_msg);
        target->IsSuppressed = false;
    }
} 
