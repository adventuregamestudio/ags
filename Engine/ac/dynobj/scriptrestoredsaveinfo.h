//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_SCRIPTRESTOREDSAVEINFO_H
#define __AC_SCRIPTRESTOREDSAVEINFO_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "game/savegame.h"
#include "game/savegame_internal.h"

class ScriptRestoredSaveInfo final : public CCBasicObject
{
public:
    using SaveRestorationFlags = AGS::Engine::SaveRestorationFlags;
    using SavegameDescription = AGS::Engine::SavegameDescription;
    using SaveRestoredDataCounts = AGS::Engine::SaveRestoredDataCounts;
    using SaveCmpSelection = AGS::Engine::SaveCmpSelection;

    ScriptRestoredSaveInfo(SaveRestorationFlags result, const SavegameDescription &save_desc,
            const SaveRestoredDataCounts &counts, bool default_cancel)
        : _result(result)
        , _desc(save_desc)
        , _counts(counts)
        , _cancel(default_cancel)
    {}

    const char *GetType() override { return "RestoredSaveInfo"; }
    int Dispose(void *address, bool force) override
    {
        delete (ScriptRestoredSaveInfo*)address;
        return 1;
    }

    bool GetCancel() const { return _cancel; }
    void SetCancel(bool cancel) { _cancel = cancel; }
    SaveCmpSelection GetRetryWithoutComponents() const { return _retryWithoutComponents; }
    void SetRetryWithoutComponents(SaveCmpSelection cmp) { _retryWithoutComponents = cmp; }
    SaveRestorationFlags GetResult() const { return _result; }
    const SavegameDescription &GetDesc() const { return _desc; }
    const SaveRestoredDataCounts &GetCounts() const { return _counts; }

private:
    SavegameDescription _desc;
    SaveRestorationFlags _result;
    SaveRestoredDataCounts _counts;
    bool _cancel = false;
    SaveCmpSelection _retryWithoutComponents = SaveCmpSelection::kSaveCmp_None;
};

#endif // __AC_SCRIPTRESTOREDSAVEINFO_H
