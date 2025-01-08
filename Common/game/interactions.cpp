//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "game/interactions.h"
#include <algorithm>
#include <string.h>
#include "ac/common.h" // quit
#include "util/stream.h"
#include "util/string_utils.h"

namespace AGS
{
namespace Common
{

//-----------------------------------------------------------------------------
//
// InteractionEvents (modern interaction system).
//
//-----------------------------------------------------------------------------

std::unique_ptr<InteractionEvents> InteractionEvents::CreateFromStream_v361(Stream *in)
{
    std::unique_ptr<InteractionEvents> inter(new InteractionEvents());
    inter->Read_v361(in);
    return inter;
}

std::unique_ptr<InteractionEvents> InteractionEvents::CreateFromStream_v362(Stream *in)
{
    std::unique_ptr<InteractionEvents> inter(new InteractionEvents());
    inter->Read_v362(in);
    return inter;
}

void InteractionEvents::Read_v361(Stream *in)
{
    Events.clear();
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        Events.push_back( { String::FromStream(in) } );
    }
}

HError InteractionEvents::Read_v362(Stream *in)
{
    Events.clear();
    InteractionEventsVersion ver = (InteractionEventsVersion)in->ReadInt32();
    if (ver != kInterEvents_v362)
        return new Error(String::FromFormat("InteractionEvents version not supported: %d", ver));

    ScriptModule = StrUtil::ReadString(in);
    const size_t evt_count = in->ReadInt32();
    for (size_t i = 0; i < evt_count; ++i)
    {
        Events.push_back( { StrUtil::ReadString(in) } );
    }
    return HError::None();
}

void InteractionEvents::Write_v361(Stream *out) const
{
    out->WriteInt32(Events.size());
    for (const auto &evt : Events)
    {
        evt.FunctionName.Write(out);
    }
}

void InteractionEvents::Write_v362(Stream *out) const
{
    out->WriteInt32(kInterEvents_v362);
    StrUtil::WriteString(ScriptModule, out);
    out->WriteInt32(Events.size());
    for (const auto &evt : Events)
    {
        StrUtil::WriteString(evt.FunctionName, out);
    }
}

//-----------------------------------------------------------------------------
//
// Interactions (legacy interaction system).
//
//-----------------------------------------------------------------------------

InteractionValue::InteractionValue()
{
    Type = kInterValLiteralInt;
    Value = 0;
    Extra = 0;
}

void InteractionValue::Read(Stream *in)
{
    Type  = (InterValType)in->ReadInt8();
    in->Seek(3); // alignment padding to int32
    Value = in->ReadInt32();
    Extra = in->ReadInt32();
}

void InteractionValue::Write(Stream *out) const
{
    out->WriteInt8(Type);
    out->WriteByteCount(0, 3); // alignment padding to int32
    out->WriteInt32(Value);
    out->WriteInt32(Extra);
}

//-----------------------------------------------------------------------------

InteractionCommand::InteractionCommand()
    : Type(0)
    , Parent(nullptr)
{
}

InteractionCommand::InteractionCommand(const InteractionCommand &ic)
{
    *this = ic;
}

void InteractionCommand::Assign(const InteractionCommand &ic, InteractionCommandList *parent)
{
    Type = ic.Type;
    memcpy(Data, ic.Data, sizeof(Data));
    Children.reset(ic.Children.get() ? new InteractionCommandList(*ic.Children) : nullptr);
    Parent = parent;
}

void InteractionCommand::Reset()
{
    Type = 0;
    memset(Data, 0, sizeof(Data));
    Children.reset();
    Parent = nullptr;
}

void InteractionCommand::ReadValues(Stream *in)
{
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        Data[i].Read(in);
    }
}

void InteractionCommand::Read(Stream *in, bool &has_children)
{
    in->ReadInt32(); // skip the 32-bit vtbl ptr (the old serialization peculiarity)
    Type = in->ReadInt32();
    ReadValues(in);
    has_children = in->ReadInt32() != 0;
    in->ReadInt32(); // skip 32-bit Parent pointer
}

void InteractionCommand::WriteValues(Stream *out) const
{
    for (int i = 0; i < MAX_ACTION_ARGS; ++i)
    {
        Data[i].Write(out);
    }
}

void InteractionCommand::Write(Stream *out) const
{
    out->WriteInt32(0); // write dummy 32-bit vtbl ptr
    out->WriteInt32(Type);
    WriteValues(out);
    out->WriteInt32(Children.get() ? 1 : 0); // notify that has children
    out->WriteInt32(0); // skip 32-bit Parent pointer
}

InteractionCommand &InteractionCommand::operator = (const InteractionCommand &ic)
{
    if (this == &ic) {
        return *this;  // prevent self-assignment
    }

    Type = ic.Type;
    memcpy(Data, ic.Data, sizeof(Data));
    Children.reset(ic.Children.get() ? new InteractionCommandList(*ic.Children) : nullptr);
    Parent = ic.Parent;
    return *this;
}

//-----------------------------------------------------------------------------

InteractionCommandList::InteractionCommandList()
    : TimesRun(0)
{
}

InteractionCommandList::InteractionCommandList(const InteractionCommandList &icmd_list)
{
    TimesRun = icmd_list.TimesRun;
    Cmds.resize(icmd_list.Cmds.size());
    for (size_t i = 0; i < icmd_list.Cmds.size(); ++i)
    {
        Cmds[i].Assign(icmd_list.Cmds[i], this);
    }
}

void InteractionCommandList::Reset()
{
    Cmds.clear();
    TimesRun = 0;
}

void InteractionCommandList::ReadCommands(Stream *in, std::vector<bool> &cmd_children)
{
    for (size_t i = 0; i < Cmds.size(); ++i)
    {
        bool has_children;
        Cmds[i].Read(in, has_children);
        cmd_children[i] = has_children;
    }
}

void InteractionCommandList::Read(Stream *in)
{
    size_t cmd_count = in->ReadInt32();
    TimesRun = in->ReadInt32();

    std::vector<bool> cmd_children;
    Cmds.resize(cmd_count);
    cmd_children.resize(cmd_count);
    ReadCommands(in, cmd_children);

    for (size_t i = 0; i < cmd_count; ++i)
    {
        if (cmd_children[i])
        {
            Cmds[i].Children.reset(new InteractionCommandList());
            Cmds[i].Children->Read(in);
        }
        Cmds[i].Parent = this;
    }
}

void InteractionCommandList::WriteCommands(Stream *out) const
{
    for (InterCmdVector::const_iterator it = Cmds.begin(); it != Cmds.end(); ++it)
    {
        it->Write(out);
    }
}

void InteractionCommandList::Write(Stream *out) const
{
    size_t cmd_count = Cmds.size();
    out->WriteInt32(cmd_count);
    out->WriteInt32(TimesRun);

    WriteCommands(out);

    for (size_t i = 0; i < cmd_count; ++i)
    {
        if (Cmds[i].Children.get() != nullptr)
            Cmds[i].Children->Write(out);
    }
}

//-----------------------------------------------------------------------------

InteractionEvent::InteractionEvent()
    : Type(0)
    , TimesRun(0)
{
}

InteractionEvent::InteractionEvent(const InteractionEvent &ie)
{
    *this = ie;
}

InteractionEvent &InteractionEvent::operator = (const InteractionEvent &ie)
{
    Type = ie.Type;
    TimesRun = ie.TimesRun;
    Response.reset(ie.Response.get() ? new InteractionCommandList(*ie.Response) : nullptr);
    return *this;
}

//-----------------------------------------------------------------------------

Interaction::Interaction()
{
}

Interaction::Interaction(const Interaction &ni)
{
    *this = ni;
}

Interaction &Interaction::operator =(const Interaction &ni)
{
    Events.resize(ni.Events.size());
    for (size_t i = 0; i < ni.Events.size(); ++i)
    {
        Events[i] = InteractionEvent(ni.Events[i]);
    }
    return *this;
}

void Interaction::CopyTimesRun(const Interaction &inter)
{
    assert(Events.size() == inter.Events.size());
    size_t count = std::min(Events.size(), inter.Events.size());
    for (size_t i = 0; i < count; ++i)
    {
        Events[i].TimesRun = inter.Events[i].TimesRun;
    }
}

void Interaction::Reset()
{
    Events.clear();
}

std::unique_ptr<Interaction> Interaction::CreateFromStream(Stream *in)
{
    if (in->ReadInt32() != kInteractionVersion_Initial)
        return nullptr; // unsupported format

    const size_t evt_count = in->ReadInt32();
    if (evt_count > MAX_NEWINTERACTION_EVENTS)
        quit("Can't deserialize interaction: too many events"); // FIXME: dont quit, report HError

    int types[MAX_NEWINTERACTION_EVENTS];
    int load_response[MAX_NEWINTERACTION_EVENTS];
    in->ReadArrayOfInt32(types, evt_count);
    in->ReadArrayOfInt32(load_response, evt_count);

    std::unique_ptr<Interaction> inter(new Interaction());
    inter->Events.resize(evt_count);
    for (size_t i = 0; i < evt_count; ++i)
    {
        InteractionEvent &evt = inter->Events[i];
        evt.Type = types[i];
        if (load_response[i] != 0)
        {
            evt.Response.reset(new InteractionCommandList());
            evt.Response->Read(in);
        }
    }
    return inter;
}

void Interaction::Write(Stream *out) const
{
    out->WriteInt32(kInteractionVersion_Initial);  // Version
    const size_t evt_count = Events.size();
    out->WriteInt32(evt_count);
    for (size_t i = 0; i < evt_count; ++i)
    {
        out->WriteInt32(Events[i].Type);
    }

    // The pointer is only checked against NULL to determine whether the event exists
    for (size_t i = 0; i < evt_count; ++i)
    {
        out->WriteInt32 (Events[i].Response.get() ? 1 : 0);
    }

    for (size_t i = 0; i < evt_count; ++i)
    {
        if (Events[i].Response.get())
            Events[i].Response->Write(out);
    }
}

//-----------------------------------------------------------------------------

#define INTER_VAR_NAME_LENGTH 23

InteractionVariable::InteractionVariable()
    : Type(0)
    , Value(0)
{
}

InteractionVariable::InteractionVariable(const String &name, char type, int val)
    : Name(name)
    , Type(type)
    , Value(val)
{
}

void InteractionVariable::Read(Stream *in)
{
    Name.ReadCount(in, INTER_VAR_NAME_LENGTH);
    Type  = in->ReadInt8();
    Value = in->ReadInt32();
}

void InteractionVariable::Write(Common::Stream *out) const
{
    out->Write(Name.GetCStr(), INTER_VAR_NAME_LENGTH);
    out->WriteInt8(Type);
    out->WriteInt32(Value);
}

} // namespace Common
} // namespace AGS
