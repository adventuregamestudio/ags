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
#ifndef __AC_INVENTORYITEMINFO_H
#define __AC_INVENTORYITEMINFO_H

#include "game/scripteventstable.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_types.h"

#define IFLG_STARTWITH 1
#define LEGACY_MAX_INVENTORY_NAME_LENGTH 25

// Inventory item event indexes
enum InventoryEventID
{
    // an interaction with any cursor mode that normally has a event
    kInventoryEvent_AnyClick = 0
};

struct InventoryItemInfo
{
    AGS::Common::String name = {};
    int  pic = 0;
    int  cursorPic = 0, hotx = 0, hoty = 0;
    uint8_t flags = 0u; // IFLG_STARTWITH
    // Interaction events (cursor-based)
    AGS::Common::ScriptEventHandlers interactions = {};

    // Provides a script events table
    const AGS::Common::ScriptEventsTable &GetEvents() const { return _events; }
    AGS::Common::ScriptEventsTable &GetEvents() { return _events; }
    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

    void ReadFromFile(AGS::Common::Stream *in);
    void WriteToFile(AGS::Common::Stream *out);
    void ReadFromSavegame(AGS::Common::Stream *in);
    void WriteToSavegame(AGS::Common::Stream *out) const;

private:
    // Script events schema
    static AGS::Common::ScriptEventsSchema _eventSchema;
    // Common events
    AGS::Common::ScriptEventsTable _events = AGS::Common::ScriptEventsTable(&InventoryItemInfo::_eventSchema);
};

#endif // __AC_INVENTORYITEMINFO_H
