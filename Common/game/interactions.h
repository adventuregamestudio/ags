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
// Interaction structs: they define engine's reaction to player interaction
// with various game objects.
//
// There are two kinds of interaction systems: the modern and legacy ones.
// The new one, represented by InteractionEvents struct, is very simple:
// it is defined as a indexed list of script function names, where index is a
// internal index of a interaction type or event (object-specific),
// and function name tells which function to run, either in a global script
// or room script (again, object-specific).
//
//-----------------------------------------------------------------------------
//
// Legacy system was used prior the proper scripting was introduced in AGS.
// This sytem was removed from AGS Editor completely in generation 3.0,
// so it's here strictly for backwards compatibility.
//
// Legacy system is represented by Interaction struct, and is defined by a
// tree-like collection of events, conditions and actions.
//
/* THE WAY THIS WORKS:
*
* Interaction (Hotspot 1)
*  |
*  +-- eventTypes [NUM_EVENTS]
*  +-- InteractionCommandList [NUM_EVENTS]   (Look at hotspot)
*        |
*        +-- InteractionCommand [NUM_COMMANDS]   (Display message)
*             |
*             +-- InteractionValue [NUM_ARGUMENTS]   (5)
*/
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
    // paired with Enabled flag to tell if this event handler has to be processed,
    // and Checked flag that tells whether the handler test in script was performed.
    struct EventHandler
    {
        String FunctionName;
        
        bool Enabled = false;
        bool Checked = false;

        inline bool IsEnabled() const { return Enabled; }
        inline bool IsChecked() const { return Checked; }
        inline void SetChecked(bool enabled) { Checked = true; Enabled = enabled; }

        EventHandler(const String &fn_name)
            // If no function name is assigned, then we disable and mark as checked right away
            : FunctionName(fn_name), Enabled(!fn_name.IsEmpty()), Checked(fn_name.IsEmpty()) {}
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


//-----------------------------------------------------------------------------
//
// Interactions (legacy interaction system).
//
//-----------------------------------------------------------------------------

#define MAX_ACTION_ARGS             5
#define MAX_NEWINTERACTION_EVENTS   30
#define MAX_COMMANDS_PER_LIST       40

enum InterValType : int8_t
{
    kInterValLiteralInt = 1,
    kInterValVariable   = 2,
    kInterValBoolean    = 3,
    kInterValCharnum    = 4
};

enum InteractionVersion
{
    kInteractionVersion_Initial = 1
};

// InteractionValue represents an argument of interaction command
struct InteractionValue
{
    InterValType Type;  // value type
    int          Value; // value definition
    int          Extra;

    InteractionValue();

    void Read(Stream *in);
    void Write(Stream *out) const;
};


struct InteractionCommandList;
typedef std::unique_ptr<InteractionCommandList> UInterCmdList;

// InteractionCommand represents a single command (action), an item of Command List
struct InteractionCommand
{
    int                     Type;       // type of action
    InteractionValue        Data[MAX_ACTION_ARGS]; // action arguments
    UInterCmdList           Children;   // list of sub-actions
    InteractionCommandList *Parent;     // action parent (optional)

    InteractionCommand();
    InteractionCommand(const InteractionCommand &ic);

    void Assign(const InteractionCommand &ic, InteractionCommandList *parent);
    void Reset();

    void Read(Stream *in, bool &has_children);
    void Write(Stream *out) const;

    InteractionCommand &operator = (const InteractionCommand &ic);

private:
    void ReadValues(Stream *in);
    void WriteValues(Stream *out) const;
};


typedef std::vector<InteractionCommand> InterCmdVector;
// InteractionCommandList represents a list of commands (actions) that need to be
// performed on particular game event
struct InteractionCommandList
{
    InterCmdVector  Cmds;     // actions to run
    int             TimesRun; // used by engine to track score changes

    InteractionCommandList();
    InteractionCommandList(const InteractionCommandList &icmd_list);

    void Reset();

    void Read(Stream *in);
    void Write(Stream *out) const;

protected:
    void ReadCommands(Common::Stream *in, std::vector<bool> &cmd_children);
    void WriteCommands(Common::Stream *out) const;
};


// InteractionEvent is a single event with a list of commands to performed
struct InteractionEvent
{
    int           Type;     // type of event
    int           TimesRun; // used by engine to track score changes
    UInterCmdList Response; // list of commands to run

    InteractionEvent();
    InteractionEvent(const InteractionEvent &ie);

    InteractionEvent &operator = (const InteractionEvent &ic);
};

typedef std::vector<InteractionEvent> InterEvtVector;
// Interaction is the list of events and responses for a game or game object
struct Interaction
{
    // The first few event types depend on the item - ID's of 100+ are
    // custom events (used for subroutines)
    InterEvtVector Events;

    Interaction();
    Interaction(const Interaction &inter);

    // Copy information on number of times events of this interaction were fired
    void CopyTimesRun(const Interaction &inter);
    void Reset();

    // Game static data (de)serialization
    static std::unique_ptr<Interaction> CreateFromStream(Stream *in);
    void Write(Stream *out) const;

    Interaction &operator =(const Interaction &inter);
};


// Legacy pre-3.0 kind of global and local room variables
struct InteractionVariable
{
    String Name {};
    char   Type {'\0'};
    int    Value {0};

    InteractionVariable();
    InteractionVariable(const String &name, char type, int val);

    void Read(Stream *in);
    void Write(Stream *out) const;
};


typedef std::unique_ptr<Interaction> UInteraction;

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__INTEREACTIONS_H
