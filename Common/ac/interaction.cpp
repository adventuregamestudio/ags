
#include <stdio.h>              // NULL definition xD
#include <string.h>
#include "ac/interaction.h"
#include "ac/common.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/datastream.h"

using AGS::Common::DataStream;
using AGS::Common::String;

InteractionVariable globalvars[MAX_GLOBAL_VARIABLES] = {{"Global 1", 0, 0}};
int numGlobalVars = 1;

NewInteractionValue::NewInteractionValue() {
    valType = VALTYPE_LITERALINT;
    val = 0;
    extra = 0;
}


void NewInteractionValue::ReadFromFile(DataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->Read(&valType, 1);
    char pad[3];
    in->Read(pad, 3);
    val = in->ReadInt32();
    extra = in->ReadInt32();
//#else
//    throw "NewInteractionValue::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void NewInteractionValue::WriteToFile(DataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    out->Write(&valType, 1);
    char pad[3];
    out->Write(pad, 3);
    out->WriteInt32(val);
    out->WriteInt32(extra);
//#else
//    throw "NewInteractionValue::WriteToFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}


void InteractionVariable::ReadFromFile(DataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->Read(name, 23);
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

void NewInteractionCommand::ReadFromFile(DataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    in->ReadInt32(); // skip the vtbl ptr
    type = in->ReadInt32();
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].ReadFromFile(in);
    }
    // all that matters is whether or not these are null...
    children = (NewInteractionAction *) (long)in->ReadInt32();
    parent = (NewInteractionCommandList *) (long)in->ReadInt32();
//#else
//    throw "NewInteractionCommand::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void NewInteractionCommand::WriteToFile(DataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    out->WriteInt32(0); // write dummy vtbl ptr 
    out->WriteInt32(type);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].WriteToFile(out);
    }
    out->WriteInt32((long)children);
    out->WriteInt32((long)parent);
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

void NewInteraction::ReadFromFile(DataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    // it's all ints! <- JJS: No, it's not! There are pointer too.

  numEvents = in->ReadInt32();
  in->ReadArray(&eventTypes, sizeof(*eventTypes), MAX_NEWINTERACTION_EVENTS);
  in->ReadArray(&timesRun, sizeof(*timesRun), MAX_NEWINTERACTION_EVENTS);

  for (int i = 0; i < MAX_NEWINTERACTION_EVENTS; i++)
    response[i] = (NewInteractionCommandList*)in->ReadInt32();

//    in->ReadArray(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int));
//#else
//    throw "NewInteraction::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}
void NewInteraction::WriteToFile(DataStream *out)
{
//#ifdef ALLEGRO_BIG_ENDIAN

  out->WriteInt32(numEvents);
  out->WriteArray(&eventTypes, sizeof(*eventTypes), MAX_NEWINTERACTION_EVENTS);
  out->WriteArray(&timesRun, sizeof(*timesRun), MAX_NEWINTERACTION_EVENTS);

  for (int i = 0; i < MAX_NEWINTERACTION_EVENTS; i++)
    out->WriteInt32((int)(response[i] != NULL));

//    fwrite(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
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


void serialize_command_list (NewInteractionCommandList *nicl, DataStream *out) {
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

void serialize_new_interaction (NewInteraction *nint, DataStream *out) {
  int a;

  out->WriteInt32 (1);  // Version
  out->WriteInt32 (nint->numEvents);
  out->WriteArrayOfInt32 (&nint->eventTypes[0], nint->numEvents);

  // 64 bit: The pointer is only checked against NULL to determine whether the event exists
  for (a = 0; a < nint->numEvents; a++)
    out->WriteInt32 ((long)nint->response[a]);

  for (a = 0; a < nint->numEvents; a++) {
    if (nint->response[a] != NULL)
      serialize_command_list (nint->response[a], out);
  }
}

NewInteractionCommandList *deserialize_command_list (DataStream *in) {
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
NewInteraction *deserialize_new_interaction (DataStream *in) {
  int a;

  if (in->ReadInt32() != 1)
    return NULL;
  nitemp = new NewInteraction;
  nitemp->numEvents = in->ReadInt32();
  if (nitemp->numEvents > MAX_NEWINTERACTION_EVENTS) {
    quit("Error: this interaction was saved with a newer version of AGS");
    return NULL;
  }
  in->ReadArrayOfInt32 (&nitemp->eventTypes[0], nitemp->numEvents);
  //in->ReadArray (&nitemp->response[0], sizeof(void*), nitemp->numEvents);

  // 64 bit: The pointer is only checked against NULL to determine whether the event exists
  for (a = 0; a < nitemp->numEvents; a++)
    nitemp->response[a] = (NewInteractionCommandList*)in->ReadInt32();

  for (a = 0; a < nitemp->numEvents; a++) {
    if (nitemp->response[a] != NULL)
      nitemp->response[a] = deserialize_command_list (in);
    nitemp->timesRun[a] = 0;
  }
  return nitemp;
}

void deserialize_interaction_scripts(DataStream *in, InteractionScripts *scripts)
{
  int numEvents = in->ReadInt32();
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
    quit("Too many interaction script events");
  scripts->numEvents = numEvents;

  String buffer;
  for (int i = 0; i < numEvents; i++)
  {
    buffer = in->ReadString(200);
    scripts->scriptFuncNames[i] = new char[buffer.GetLength() + 1];
    strcpy(scripts->scriptFuncNames[i], buffer);
  }
}
