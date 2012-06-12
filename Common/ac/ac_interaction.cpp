
#include <stdio.h>              // NULL definition xD
#include <string.h>
#include "ac_interaction.h"

NewInteractionValue::NewInteractionValue() {
    valType = VALTYPE_LITERALINT;
    val = 0;
    extra = 0;
}

#ifdef ALLEGRO_BIG_ENDIAN
void NewInteractionValue::ReadFromFile(FILE *fp)
{
    fread(&valType, sizeof(char), 1, fp);
    char pad[3]; fread(pad, sizeof(char), 3, fp);
    val = getw(fp);
    extra = getw(fp);
}
void NewInteractionValue::WriteToFile(FILE *fp)
{
    fwrite(&valType, sizeof(char), 1, fp);
    char pad[3]; fwrite(pad, sizeof(char), 3, fp);
    putw(val, fp);
    putw(extra, fp);
}
#endif



NewInteractionCommand::NewInteractionCommand() {
    type = 0;
    children = NULL;
    parent = NULL;
}
NewInteractionCommandList *NewInteractionCommand::get_child_list() {
    return (NewInteractionCommandList*)children;
}

void NewInteractionCommand::reset() { remove(); }

#ifdef ALLEGRO_BIG_ENDIAN
void NewInteractionCommand::ReadFromFile(FILE *fp)
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
void NewInteractionCommand::WriteToFile(FILE *fp)
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

NewInteractionCommandList::NewInteractionCommandList () {
    numCommands = 0;
    timesRun = 0;
}

NewInteraction::NewInteraction() { 
    numEvents = 0;
    // NULL all the pointers
    memset (response, 0, sizeof(NewInteractionCommandList*) * MAX_NEWINTERACTION_EVENTS);
    memset (&timesRun[0], 0, sizeof(int) * MAX_NEWINTERACTION_EVENTS);
}


void NewInteraction::copy_timesrun_from (NewInteraction *nifrom) {
    memcpy (&timesRun[0], &nifrom->timesRun[0], sizeof(int) * MAX_NEWINTERACTION_EVENTS);
}
void NewInteraction::reset() {
    for (int i = 0; i < numEvents; i++) {
        if (response[i] != NULL) {
            response[i]->reset();
            delete response[i];
            response[i] = NULL;
        }
    }
    numEvents = 0;
}
NewInteraction::~NewInteraction() {
    reset();
}

#ifdef ALLEGRO_BIG_ENDIAN
void NewInteraction::ReadFromFile(FILE *fp)
{
    // it's all ints!
    fread(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
}
void NewInteraction::WriteToFile(FILE *fp)
{
    fwrite(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
}
#endif


InteractionScripts::InteractionScripts() {
    numEvents = 0;
}

InteractionScripts::~InteractionScripts() {
    for (int i = 0; i < numEvents; i++)
        delete scriptFuncNames[i];
}