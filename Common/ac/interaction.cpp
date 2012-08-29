
#include <stdio.h>              // NULL definition xD
#include <string.h>
#include "ac/interaction.h"
#include "ac/common.h"
#include "util/string_utils.h"      // fputstring, etc

InteractionVariable globalvars[MAX_GLOBAL_VARIABLES] = {{"Global 1", 0, 0}};
int numGlobalVars = 1;

NewInteractionValue::NewInteractionValue() {
    valType = VALTYPE_LITERALINT;
    val = 0;
    extra = 0;
}


void NewInteractionValue::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fread(&valType, sizeof(char), 1, fp);
    char pad[3]; fread(pad, sizeof(char), 3, fp);
    val = getw(fp);
    extra = getw(fp);
//#else
//    throw "NewInteractionValue::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void NewInteractionValue::WriteToFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fwrite(&valType, sizeof(char), 1, fp);
    char pad[3]; fwrite(pad, sizeof(char), 3, fp);
    putw(val, fp);
    putw(extra, fp);
//#else
//    throw "NewInteractionValue::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}


void InteractionVariable::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fread(name, sizeof(char), 23, fp);
    type = getc(fp);
    value = getw(fp);
//#else
//    throw "InteractionVariable::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}


NewInteractionCommand::NewInteractionCommand() {
    type = 0;
    children = NULL;
    parent = NULL;
}

NewInteractionCommandList *NewInteractionCommand::get_child_list() {
    return (NewInteractionCommandList*)children;
}

void NewInteractionCommand::remove () {
    if (children != NULL) {
        children->reset();
        delete children;
    }
    children = NULL;
    parent = NULL;
    type = 0;
}

void NewInteractionCommand::reset() { remove(); }

void NewInteractionCommand::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    getw(fp); // skip the vtbl ptr
    type = getw(fp);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].ReadFromFile(fp);
    }
    // all that matters is whether or not these are null...
    children = (NewInteractionAction *)(long)getw(fp);
    parent = (NewInteractionCommandList *)(long)getw(fp);
//#else
//    throw "NewInteractionCommand::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void NewInteractionCommand::WriteToFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    putw(0, fp); // write dummy vtbl ptr 
    putw(type, fp);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].WriteToFile(fp);
    }
    putw((long)children, fp);
    putw((long)parent, fp);
//#else
//    throw "NewInteractionCommand::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

NewInteractionCommandList::NewInteractionCommandList () {
    numCommands = 0;
    timesRun = 0;
}

void NewInteractionCommandList::reset () {
  int j;
  for (j = 0; j < numCommands; j++) {
    if (command[j].children != NULL) {
      // using this Reset crashes it for some reason
      //command[j].reset ();
      command[j].get_child_list()->reset();
      delete command[j].children;
      command[j].children = NULL;
    }
    command[j].remove();
  }
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

void NewInteraction::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    // it's all ints!
    fread(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
//#else
//    throw "NewInteraction::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
void NewInteraction::WriteToFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    fwrite(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
//#else
//    throw "NewInteraction::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}


InteractionScripts::InteractionScripts() {
    numEvents = 0;
}

InteractionScripts::~InteractionScripts() {
    for (int i = 0; i < numEvents; i++)
        delete scriptFuncNames[i];
}


void serialize_command_list (NewInteractionCommandList *nicl, FILE*ooo) {
  if (nicl == NULL)
    return;
  putw (nicl->numCommands, ooo);
  putw (nicl->timesRun, ooo);
//#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
      nicl->command[iteratorCount].WriteToFile(ooo);
  }
//#else
//  fwrite (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands, ooo);  
//#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL)
      serialize_command_list (nicl->command[k].get_child_list(), ooo);
  }
}

void serialize_new_interaction (NewInteraction *nint, FILE*ooo) {
  int a;

  putw (1, ooo);  // Version
  putw (nint->numEvents, ooo);
  fwrite (&nint->eventTypes[0], sizeof(int), nint->numEvents, ooo);

  // 64 bit: The pointer is only checked against NULL to determine whether the event exists
  for (a = 0; a < nint->numEvents; a++)
    putw ((long)nint->response[a], ooo);

  for (a = 0; a < nint->numEvents; a++) {
    if (nint->response[a] != NULL)
      serialize_command_list (nint->response[a], ooo);
  }
}

NewInteractionCommandList *deserialize_command_list (FILE *ooo) {
  NewInteractionCommandList *nicl = new NewInteractionCommandList;
  nicl->numCommands = getw(ooo);
  nicl->timesRun = getw(ooo);
//#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
      nicl->command[iteratorCount].ReadFromFile(ooo);
  }
//#else
//  fread (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands, ooo);  
//#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL) {
      nicl->command[k].children = deserialize_command_list (ooo);
    }
    nicl->command[k].parent = nicl;
  }
  return nicl;
}

NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (FILE *ooo) {
  int a;

  if (getw(ooo) != 1)
    return NULL;
  nitemp = new NewInteraction;
  nitemp->numEvents = getw(ooo);
  if (nitemp->numEvents > MAX_NEWINTERACTION_EVENTS) {
    quit("Error: this interaction was saved with a newer version of AGS");
    return NULL;
  }
  fread (&nitemp->eventTypes[0], sizeof(int), nitemp->numEvents, ooo);
  //fread (&nitemp->response[0], sizeof(void*), nitemp->numEvents, ooo);

  // 64 bit: The pointer is only checked against NULL to determine whether the event exists
  for (a = 0; a < nitemp->numEvents; a++)
    nitemp->response[a] = (NewInteractionCommandList*)getw(ooo);

  for (a = 0; a < nitemp->numEvents; a++) {
    if (nitemp->response[a] != NULL)
      nitemp->response[a] = deserialize_command_list (ooo);
    nitemp->timesRun[a] = 0;
  }
  return nitemp;
}

void deserialize_interaction_scripts(FILE *iii, InteractionScripts *scripts)
{
  int numEvents = getw(iii);
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
    quit("Too many interaction script events");
  scripts->numEvents = numEvents;

  char buffer[200];
  for (int i = 0; i < numEvents; i++)
  {
    fgetstring_limit(buffer, iii, sizeof(buffer));
    scripts->scriptFuncNames[i] = new char[strlen(buffer) + 1];
    strcpy(scripts->scriptFuncNames[i], buffer);
  }
}
