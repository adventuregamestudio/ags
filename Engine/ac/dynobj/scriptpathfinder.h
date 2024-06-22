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
#ifndef __AGS_EE_DYNOBJ__SCRIPTPATHFINDER_H
#define __AGS_EE_DYNOBJ__SCRIPTPATHFINDER_H

#include <memory>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "ac/route_finder.h"


class ScriptPathfinder : public AGSCCDynamicObject
{
public:
    virtual AGS::Engine::IRouteFinder *GetRouteFinder() = 0;
    virtual void SyncPathfinder() = 0;
};

class ScriptMaskPathfinder : public ScriptPathfinder
{
public:
    // Creates and registers a MaskPathfinder
    static ScriptMaskPathfinder *CreateFromMaskSprite(int mask_sprite);

    AGS::Engine::IRouteFinder *GetRouteFinder() override;
    void SyncPathfinder() override;

    void SetMaskSprite(int mask_sprite);

    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    ScriptMaskPathfinder() = default;

    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    uint32_t _maskSprite = 0u;
    std::unique_ptr<AGS::Engine::MaskRouteFinder> _finder;
};

class RoomPathfinder : public ScriptPathfinder
{
public:
    // Creates and registers a RoomPathfinder
    static RoomPathfinder *Create();

    AGS::Engine::IRouteFinder *GetRouteFinder() override;
    void SyncPathfinder() override;

    const char *GetType() override;
    int Dispose(void *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

private:
    RoomPathfinder() = default;

    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    AGS::Engine::MaskRouteFinder *_finder = nullptr;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTPATHFINDER_H
