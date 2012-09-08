
#include <stdio.h>              // NULL definition xD
#include <string.h>
#include "ac/interaction.h"
#include "ac/common.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/datastream.h"

using AGS::Common::CDataStream;
using AGS::Common::CString;

InteractionVariable globalvars[MAX_GLOBAL_VARIABLES] = {{"Global 1", 0, 0}};
int numGlobalVars = 1;

NewInteractionValue::NewInteractionValue() {
    valType = VALTYPE_LITERALINT;
    val = 0;
    extra = 0;
}


void NewInteractionValue::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadArray(&valType, sizeof(char), 1);
    char pad[3]; in->ReadArray(pad, sizeof(char), 3);
    val = in->ReadInt32();
    extra = in->ReadInt32();
//#else
//    throw "NewInteractionValue::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void NewInteractionValue::WriteToFile(CDataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    out->WriteArray(&valType, sizeof(char), 1);
    char pad[3]; out->WriteArray(pad, sizeof(char), 3);
    out->WriteInt32(val);
    out->WriteInt32(extra);
//#else
//    throw "NewInteractionValue::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}


void InteractionVariable::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadArray(name, sizeof(char), 23);
    type = in->ReadInt8();
    value = in->ReadInt32();
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

void NewInteractionCommand::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadInt32(); // skip the vtbl ptr
    type = in->ReadInt32();
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].ReadFromFile(in);
    }
    // all that matters is whether or not these are null...
    children = (NewInteractionAction *) in->ReadInt32();
    parent = (NewInteractionCommandList *) in->ReadInt32();
//#else
//    throw "NewInteractionCommand::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void NewInteractionCommand::WriteToFile(CDataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    out->WriteInt32(0); // write dummy vtbl ptr 
    out->WriteInt32(type);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].WriteToFile(out);
    }
    out->WriteInt32((int)children);
    out->WriteInt32((int)parent);
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

void NewInteraction::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    // it's all ints!
    in->ReadArray(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int));
//#else
//    throw "NewInteraction::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
void NewInteraction::WriteToFile(CDataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    out->WriteArray(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int));
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


void serialize_command_list (NewInteractionCommandList *nicl, CDataStream *out) {
  if (nicl == NULL)
    return;
  out->WriteInt32 (nicl->numCommands);
  out->WriteInt32 (nicl->timesRun);
//#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
      nicl->command[iteratorCount].WriteToFile(out);
  }
//#else
//  out->WriteArray (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands);  
//#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL)
      serialize_command_list (nicl->command[k].get_child_list(), out);
  }
}

void serialize_new_interaction (NewInteraction *nint, CDataStream *out) {
  int a;

  out->WriteInt32 (1);  // Version
  out->WriteInt32 (nint->numEvents);
  out->WriteArray (&nint->eventTypes[0], sizeof(int), nint->numEvents);
  for (a = 0; a < nint->numEvents; a++)
    out->WriteInt32 ((int)nint->response[a]);

  for (a = 0; a < nint->numEvents; a++) {
    if (nint->response[a] != NULL)
      serialize_command_list (nint->response[a], out);
  }
}

NewInteractionCommandList *deserialize_command_list (CDataStream *in) {
  NewInteractionCommandList *nicl = new NewInteractionCommandList;
  nicl->numCommands = in->ReadInt32();
  nicl->timesRun = in->ReadInt32();
//#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < nicl->numCommands; ++iteratorCount)
  {
      nicl->command[iteratorCount].ReadFromFile(in);
  }
//#else
//  in->ReadArray (&nicl->command[0], sizeof(NewInteractionCommand), nicl->numCommands);  
//#endif  // ALLEGRO_BIG_ENDIAN
  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL) {
      nicl->command[k].children = deserialize_command_list (in);
    }
    nicl->command[k].parent = nicl;
  }
  return nicl;
}

NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (CDataStream *in) {
  int a;

  if (in->ReadInt32() != 1)
    return NULL;
  nitemp = new NewInteraction;
  nitemp->numEvents = in->ReadInt32();
  if (nitemp->numEvents > MAX_NEWINTERACTION_EVENTS) {
    quit("Error: this interaction was saved with a newer version of AGS");
    return NULL;
  }
  in->ReadArray (&nitemp->eventTypes[0], sizeof(int), nitemp->numEvents);
  //in->ReadArray (&nitemp->response[0], sizeof(void*), nitemp->numEvents);
  for (a = 0; a < nitemp->numEvents; a++)
    nitemp->response[a] = (NewInteractionCommandList*)in->ReadInt32();

  for (a = 0; a < nitemp->numEvents; a++) {
    if (nitemp->response[a] != NULL)
      nitemp->response[a] = deserialize_command_list (in);
    nitemp->timesRun[a] = 0;
  }
  return nitemp;
}

void deserialize_interaction_scripts(CDataStream *in, InteractionScripts *scripts)
{
  int numEvents = in->ReadInt32();
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
    quit("Too many interaction script events");
  scripts->numEvents = numEvents;

  CString buffer;
  for (int i = 0; i < numEvents; i++)
  {
    buffer = in->ReadString(200);
    scripts->scriptFuncNames[i] = new char[buffer.GetLength() + 1];
    strcpy(scripts->scriptFuncNames[i], buffer);
  }
}
