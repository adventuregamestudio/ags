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

#ifndef __AC_INTERACTION_H
#define __AC_INTERACTION_H

#include "ac/common_defines.h" // macros, typedef
#include "util/array.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

/* THE WAY THIS WORKS:
*
* NewInteraction (Hotspot 1)
*  |
*  +-- eventTypes [NUM_EVENTS]
*  +-- NewInteractionCommandList [NUM_EVENTS]   (Look at hotspot)
*        |
*        +-- NewInteractionCommand [NUM_COMMANDS]   (Display message)
*             |
*             +-- NewInteractionValue [NUM_ARGUMENTS]   (5)
*/

#define LOCAL_VARIABLE_OFFSET 10000
#define MAX_GLOBAL_VARIABLES 100
#define MAX_ACTION_ARGS 5
#define MAX_NEWINTERACTION_EVENTS 30
#define MAX_COMMANDS_PER_LIST 40
#define VALTYPE_LITERALINT 1
#define VALTYPE_VARIABLE   2
#define VALTYPE_BOOLEAN    3
#define VALTYPE_CHARNUM    4

struct NewInteractionValue {
    unsigned char valType;
    int  val;
    int  extra;

    NewInteractionValue();

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
};

//-----------------------------------------------------------------------------
// [IKM] 2013-05-08:
// It looks like this NewInteractionAction parent class is practically
// redundant. There's no pointer of its type that would be used as it is, the
// only instance being NewInteractionCommand::children pointer, which is always
// cast to NewInteractionCommandList before use.
//
// Since this construction was used only by older AGS versions and not likely
// to be developed any further, I removed NewInteractionAction completely, and
// changed NewInteractionCommand::children pointer to NewInteractionCommandList.
//
//---------------------------------------------------------
/*
struct NewInteractionAction {
    virtual void reset() = 0;
};*/

struct NewInteractionCommandList;

struct NewInteractionCommand /*: public NewInteractionAction*/ {
    int32 type;
    NewInteractionValue data[MAX_ACTION_ARGS];
    NewInteractionCommandList *children;
    NewInteractionCommandList *parent;

    NewInteractionCommand();
    ~NewInteractionCommand();
    /*NewInteractionCommandList *get_child_list();*/

    void assign(const NewInteractionCommand &nic, NewInteractionCommandList *new_parent);
    void reset();

    void ReadFromFile_v321(Common::Stream *in);
    void WriteToFile_v321(Common::Stream *out);
    void ReadNewInteractionValues_Aligned(Common::Stream *in);
    void WriteNewInteractionValues_Aligned(Common::Stream *out);
};

struct NewInteractionCommandList /*: public NewInteractionAction*/ {
    int32 numCommands;
    NewInteractionCommand command[MAX_COMMANDS_PER_LIST];
    int32 timesRun;   // used by engine to track score changes

    NewInteractionCommandList();
    NewInteractionCommandList(const NewInteractionCommandList &nicmd_list);
    ~NewInteractionCommandList();
    void reset();

    void ReadInteractionCommands_Aligned(Common::Stream *in);
    void WriteInteractionCommands_Aligned(Common::Stream *out);
};

struct NewInteraction {
    int numEvents;
    // the first few event types depend on the item - ID's of 100+ are
    // custom events (used for subroutines)
    int eventTypes[MAX_NEWINTERACTION_EVENTS];
    int timesRun[MAX_NEWINTERACTION_EVENTS];
    NewInteractionCommandList *response[MAX_NEWINTERACTION_EVENTS];

    NewInteraction();
    NewInteraction(const NewInteraction &ni);
    ~NewInteraction();

    void copy_timesrun_from (NewInteraction *nifrom);
    void reset();

    void ReadFromFile(Common::Stream *in, bool ignore_pointers = false);
    void WriteToFile(Common::Stream *out);

    NewInteraction &operator=(const NewInteraction &ni);
};


struct InteractionVariable {
    char name[23];
    char type;
    int  value;

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
};
extern InteractionVariable globalvars[];
extern int numGlobalVars;




struct InteractionScripts {
    int numEvents;
    Common::ObjectArray<Common::String> scriptFuncNames; //[MAX_NEWINTERACTION_EVENTS]

    InteractionScripts();
    InteractionScripts(const InteractionScripts &is);
    ~InteractionScripts();
};


// EventHandlers is a pair of old- and newstyle event handler objects;
// engine assumes that only one of the objects is valid at any given time.
// InteractionScripts has higher priority when looking for valid handler.
struct EventHandler
{
    // Old-style graphically set interactions
    NewInteraction      *Interaction;
    // Script event-handling functions
    InteractionScripts  *ScriptFnRef;

    EventHandler();
    EventHandler(const EventHandler &eh);
    ~EventHandler();
    void Free();
};


extern InteractionVariable globalvars[MAX_GLOBAL_VARIABLES];
extern int numGlobalVars;

extern void serialize_command_list (NewInteractionCommandList *nicl, Common::Stream *out);
extern void serialize_new_interaction (NewInteraction *nint, Common::Stream *out);

extern NewInteractionCommandList *deserialize_command_list (Common::Stream *out);
extern NewInteraction *deserialize_new_interaction (Common::Stream *in);

extern NewInteraction *nitemp;
extern void deserialize_interaction_scripts(Common::Stream *in, InteractionScripts *scripts);


#endif // __AC_INTERACTION_H