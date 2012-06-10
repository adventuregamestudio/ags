#ifndef __CROOM_INTERACTION_H
#define __CROOM_INTERACTION_H

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

    NewInteractionValue() {
        valType = VALTYPE_LITERALINT;
        val = 0;
        extra = 0;
    }


#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        fread(&valType, sizeof(char), 1, fp);
        char pad[3]; fread(pad, sizeof(char), 3, fp);
        val = getw(fp);
        extra = getw(fp);
    }
    void WriteToFile(FILE *fp)
    {
        fwrite(&valType, sizeof(char), 1, fp);
        char pad[3]; fwrite(pad, sizeof(char), 3, fp);
        putw(val, fp);
        putw(extra, fp);
    }
#endif
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

    NewInteractionCommand() {
        type = 0;
        children = NULL;
        parent = NULL;
    }
    NewInteractionCommandList *get_child_list() {
        return (NewInteractionCommandList*)children;
    }
    void remove();

    void reset() { remove(); }

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        getw(fp); // skip the vtbl ptr
        type = getw(fp);
        for (int i = 0; i < MAX_ACTION_ARGS; ++i)
        {
            data[i].ReadFromFile(fp);
        }
        // all that matters is whether or not these are null...
        children = (NewInteractionAction *) getw(fp);
        parent = (NewInteractionCommandList *) getw(fp);
    }
    void WriteToFile(FILE *fp)
    {
        putw(0, fp); // write dummy vtbl ptr 
        putw(type, fp);
        for (int i = 0; i < MAX_ACTION_ARGS; ++i)
        {
            data[i].WriteToFile(fp);
        }
        putw((int)children, fp);
        putw((int)parent, fp);
    }
#endif
};

struct NewInteractionCommandList : public NewInteractionAction {
    int32 numCommands;
    NewInteractionCommand command[MAX_COMMANDS_PER_LIST];
    int32 timesRun;   // used by engine to track score changes

    NewInteractionCommandList () {
        numCommands = 0;
        timesRun = 0;
    }
    void reset();
};

struct NewInteraction {
    int numEvents;
    // the first few event types depend on the item - ID's of 100+ are
    // custom events (used for subroutines)
    int eventTypes[MAX_NEWINTERACTION_EVENTS];
    int timesRun[MAX_NEWINTERACTION_EVENTS];
    NewInteractionCommandList *response[MAX_NEWINTERACTION_EVENTS];


    NewInteraction() { 
        numEvents = 0;
        // NULL all the pointers
        memset (response, 0, sizeof(NewInteractionCommandList*) * MAX_NEWINTERACTION_EVENTS);
        memset (&timesRun[0], 0, sizeof(int) * MAX_NEWINTERACTION_EVENTS);
    }


    void copy_timesrun_from (NewInteraction *nifrom) {
        memcpy (&timesRun[0], &nifrom->timesRun[0], sizeof(int) * MAX_NEWINTERACTION_EVENTS);
    }
    void reset() {
        for (int i = 0; i < numEvents; i++) {
            if (response[i] != NULL) {
                response[i]->reset();
                delete response[i];
                response[i] = NULL;
            }
        }
        numEvents = 0;
    }
    ~NewInteraction() {
        reset();
    }

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        // it's all ints!
        fread(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
    }
    void WriteToFile(FILE *fp)
    {
        fwrite(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
    }
#endif
};


struct InteractionVariable {
    char name[23];
    char type;
    int  value;

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        fread(name, sizeof(char), 23, fp);
        type = getc(fp);
        value = getw(fp);
    }
#endif
};
extern InteractionVariable globalvars[];
extern int numGlobalVars;




struct InteractionScripts {
    int numEvents;
    char *scriptFuncNames[MAX_NEWINTERACTION_EVENTS];

    InteractionScripts() {
        numEvents = 0;
    }

    ~InteractionScripts() {
        for (int i = 0; i < numEvents; i++)
            delete scriptFuncNames[i];
    }
};



#endif // __CROOM_INTERACTION_H