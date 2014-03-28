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

#include <stdio.h>              // NULL definition xD
#include <string.h>
#include "ac/interaction.h"
#include "ac/common.h"
#include "util/string_utils.h"      // fputstring, etc
#include "util/alignedstream.h"
#include "util/string.h"

using AGS::Common::AlignedStream;
using AGS::Common::Stream;
using AGS::Common::String;

InteractionVariable globalvars[MAX_GLOBAL_VARIABLES] = {{"Global 1", 0, 0}};
int numGlobalVars = 1;

NewInteractionValue::NewInteractionValue() {
    valType = VALTYPE_LITERALINT;
    val = 0;
    extra = 0;
}


void NewInteractionValue::ReadFromFile(Stream *in)
{
    in->Read(&valType, 1);
    val = in->ReadInt32();
    extra = in->ReadInt32();
}

void NewInteractionValue::WriteToFile(Stream *out)
{
    out->Write(&valType, 1);
    out->WriteInt32(val);
    out->WriteInt32(extra);
}


void InteractionVariable::ReadFromFile(Stream *in)
{
    in->Read(name, 23);
    type = in->ReadInt8();
    value = in->ReadInt32();
}

void InteractionVariable::WriteToFile(Common::Stream *out)
{
    out->Write(name, 23);
    out->WriteInt8(type);
    out->WriteInt32(value);
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

void NewInteractionCommand::ReadNewInteractionValues_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void NewInteractionCommand::ReadFromFile_v321(Stream *in)
{
    in->ReadInt32(); // skip the vtbl ptr
    type = in->ReadInt32();
    ReadNewInteractionValues_Aligned(in);
    // all that matters is whether or not these are null...
    children = (NewInteractionAction *) (long)in->ReadInt32();
    parent = (NewInteractionCommandList *) (long)in->ReadInt32();
}

void NewInteractionCommand::WriteNewInteractionValues_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        data[i].WriteToFile(&align_s);
        align_s.Reset();
    }
}

void NewInteractionCommand::WriteToFile_v321(Stream *out)
{
    out->WriteInt32(0); // write dummy vtbl ptr 
    out->WriteInt32(type);
    WriteNewInteractionValues_Aligned(out);
    out->WriteInt32((long)children);
    out->WriteInt32((long)parent);
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

void NewInteractionCommandList::ReadInteractionCommands_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int iteratorCount = 0; iteratorCount < numCommands; ++iteratorCount)
    {
        command[iteratorCount].ReadFromFile_v321(&align_s);
        align_s.Reset();
    }
}

void NewInteractionCommandList::WriteInteractionCommands_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int iteratorCount = 0; iteratorCount < numCommands; ++iteratorCount)
    {
        command[iteratorCount].WriteToFile_v321(&align_s);
        align_s.Reset();
    }
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

void NewInteraction::ReadFromFile(Stream *in)
{
    // it's all ints! <- JJS: No, it's not! There are pointer too.

  numEvents = in->ReadInt32();
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
      quit("Error: can't deserialize interaction: too many events");
  in->ReadArray(&eventTypes, sizeof(*eventTypes), MAX_NEWINTERACTION_EVENTS);
  in->ReadArray(&timesRun, sizeof(*timesRun), MAX_NEWINTERACTION_EVENTS);

  // This function is called only when reading RoomStatus from savedgame,
  // and the following response pointer values are never really used anywhere
  // (and apparently are always NULL). The real full item deserialization is
  // made by calling deserialize_new_interaction().
  memset(response, 0, sizeof(response));
  for (int i = 0; i < MAX_NEWINTERACTION_EVENTS; i++)
  {
    in->ReadInt32(); // response[i] 32-bit pointer;
  }

//    in->ReadArray(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int));
}
void NewInteraction::WriteToFile(Stream *out)
{
  out->WriteInt32(numEvents);
  out->WriteArray(&eventTypes, sizeof(*eventTypes), MAX_NEWINTERACTION_EVENTS);
  out->WriteArray(&timesRun, sizeof(*timesRun), MAX_NEWINTERACTION_EVENTS);

  for (int i = 0; i < MAX_NEWINTERACTION_EVENTS; i++)
    out->WriteInt32((int)(response[i] != NULL));

//    fwrite(&numEvents, sizeof(int), sizeof(NewInteraction)/sizeof(int), fp);
}


InteractionScripts::InteractionScripts() {
    numEvents = 0;
}

InteractionScripts::~InteractionScripts() {
    for (int i = 0; i < numEvents; i++)
        delete[] scriptFuncNames[i];
}

void serialize_command_list (NewInteractionCommandList *nicl, Stream *out) {
  if (nicl == NULL)
    return;
  out->WriteInt32 (nicl->numCommands);
  out->WriteInt32 (nicl->timesRun);

  nicl->WriteInteractionCommands_Aligned(out);

  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL)
      serialize_command_list (nicl->command[k].get_child_list(), out);
  }
}

void serialize_new_interaction (NewInteraction *nint, Stream *out) {
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

NewInteractionCommandList *deserialize_command_list (Stream *in) {
  NewInteractionCommandList *nicl = new NewInteractionCommandList;
  nicl->numCommands = in->ReadInt32();
  nicl->timesRun = in->ReadInt32();

  nicl->ReadInteractionCommands_Aligned(in);

  for (int k = 0; k < nicl->numCommands; k++) {
    if (nicl->command[k].children != NULL) {
      nicl->command[k].children = deserialize_command_list (in);
    }
    nicl->command[k].parent = nicl;
  }
  return nicl;
}

NewInteraction *nitemp;
NewInteraction *deserialize_new_interaction (Stream *in) {
  int a;

  if (in->ReadInt32() != 1)
    return NULL;
  nitemp = new NewInteraction;
  nitemp->numEvents = in->ReadInt32();
  if (nitemp->numEvents > MAX_NEWINTERACTION_EVENTS) {
    quit("Error: can't deserialize interaction: too many events");
    return NULL;
  }
  in->ReadArrayOfInt32 (&nitemp->eventTypes[0], nitemp->numEvents);

  bool load_response[MAX_NEWINTERACTION_EVENTS];
  for (a = 0; a < nitemp->numEvents; a++)
    load_response[a] = in->ReadInt32() != 0;

  memset(nitemp->response, 0, sizeof(nitemp->response));
  for (a = 0; a < nitemp->numEvents; a++) {
    if (load_response[a])
      nitemp->response[a] = deserialize_command_list (in);
    nitemp->timesRun[a] = 0;
  }
  return nitemp;
}

void deserialize_interaction_scripts(Stream *in, InteractionScripts *scripts)
{
  int numEvents = in->ReadInt32();
  if (numEvents > MAX_NEWINTERACTION_EVENTS)
    quit("Too many interaction script events");
  scripts->numEvents = numEvents;

  String buffer;
  for (int i = 0; i < numEvents; i++)
  {
    buffer.Read(in, 200);
    scripts->scriptFuncNames[i] = new char[buffer.GetLength() + 1];
    strcpy(scripts->scriptFuncNames[i], buffer);
  }
}
