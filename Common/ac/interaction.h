#ifndef __AC_INTERACTION_H
#define __AC_INTERACTION_H

#include "ac/common_defines.h" // macros, typedef
#include "platform/file.h"

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

    void ReadFromFile(FILE *fp);
    void WriteToFile(FILE *fp);
};

struct NewInteractionAction {
    virtual void reset() = 0;
};
struct NewInteractionCommandList;

struct NewInteractionCommand: public NewInteractionAction {
    int32 type;
    NewInteractionValue data[MAX_ACTION_ARGS];
    NewInteractionAction * children;
    NewInteractionCommandList *parent;

    NewInteractionCommand();
    NewInteractionCommandList *get_child_list();
    void remove();

    void reset();

    void ReadFromFile(FILE *fp);
    void WriteToFile(FILE *fp);
};

struct NewInteractionCommandList : public NewInteractionAction {
    int32 numCommands;
    NewInteractionCommand command[MAX_COMMANDS_PER_LIST];
    int32 timesRun;   // used by engine to track score changes

    NewInteractionCommandList ();
    void reset();
};

struct NewInteraction {
    int numEvents;
    // the first few event types depend on the item - ID's of 100+ are
    // custom events (used for subroutines)
    int eventTypes[MAX_NEWINTERACTION_EVENTS];
    int timesRun[MAX_NEWINTERACTION_EVENTS];
    NewInteractionCommandList *response[MAX_NEWINTERACTION_EVENTS];


    NewInteraction();


    void copy_timesrun_from (NewInteraction *nifrom);
    void reset();
    ~NewInteraction();

    void ReadFromFile(FILE *fp);
    void WriteToFile(FILE *fp);
};


struct InteractionVariable {
    char name[23];
    char type;
    int  value;

    void ReadFromFile(FILE *fp);
};
extern InteractionVariable globalvars[];
extern int numGlobalVars;




struct InteractionScripts {
    int numEvents;
    char *scriptFuncNames[MAX_NEWINTERACTION_EVENTS];

    InteractionScripts();
    ~InteractionScripts();
};


extern InteractionVariable globalvars[MAX_GLOBAL_VARIABLES];
extern int numGlobalVars;

extern void serialize_command_list (NewInteractionCommandList *nicl, FILE*ooo);
extern void serialize_new_interaction (NewInteraction *nint, FILE*ooo);
extern NewInteractionCommandList *deserialize_command_list (FILE *ooo);

extern NewInteraction *nitemp;
extern NewInteraction *deserialize_new_interaction (FILE *ooo);

extern void deserialize_interaction_scripts(FILE *iii, InteractionScripts *scripts);


#endif // __AC_INTERACTION_H