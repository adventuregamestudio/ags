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
//
// Interaction structs: they define engine's reaction to player interaction
// with various game objects.
//
// The contemporary interaction system is represented by InteractionEvents:
// it is defined as a indexed list of script function names, where index is a
// internal index of a interaction type or event (object-specific),
// and function name tells which function to run, either in a global script
// or room script (again, object-specific).
//
//=============================================================================
#ifndef __AGS_CN_GAME__INTEREACTIONS_H
#define __AGS_CN_GAME__INTEREACTIONS_H

#include <memory>
#include <vector>
#include "util/error.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

class Stream;

//-----------------------------------------------------------------------------
//
// InteractionEvents (modern interaction system).
// A indexed list of script functions for all the supported events.
// Indexes are object-specific.
//
//-----------------------------------------------------------------------------

enum InteractionEventsVersion
{
    kInterEvents_Initial = 0,
    kInterEvents_v362    = 3060200,
};

// A indexed list of function links for all the supported events.
struct InteractionEvents
{
    // An optional name of a script module to run functions in
    String ScriptModule;
    // Script function names, corresponding to the event's index,
    // paired with Enabled flag to tell if this event handler has to be processed
    struct EventHandler
    {
        String FunctionName;
        // At runtime we may want to receive function's call result and update
        // Enabled status, but result may be delayed, so we have to use a shared memory object for safety.
        // TODO: have this in runtime-only struct, when we have a clear separation
        std::shared_ptr<bool> Enabled;

        inline bool IsEnabled() const { return Enabled && *Enabled; }

        EventHandler(const String &fn_name)
            : FunctionName(fn_name), Enabled(new bool(!fn_name.IsEmpty())) {}
    };
    std::vector<EventHandler> Events;

    // Read and create pre-3.6.2 version of the InteractionEvents
    static std::unique_ptr<InteractionEvents> CreateFromStream_v361(Stream *in);
    // Read and create 3.6.2+ version of the InteractionEvents
    static std::unique_ptr<InteractionEvents> CreateFromStream_v362(Stream *in);
    void Read_v361(Stream *in);
    HError Read_v362(Stream *in);
    void Write_v361(Stream *out) const;
    void Write_v362(Stream *out) const;
};

typedef std::unique_ptr<InteractionEvents> UInteractionEvents;

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__INTEREACTIONS_H
