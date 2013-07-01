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
//
// RoomObject, a class of room object.
//
//=============================================================================
#ifndef __AGS_EE_GAME__ROOMOBJECT_H
#define __AGS_EE_GAME__ROOMOBJECT_H

#include "core/types.h"
#include "ac/interaction.h"

namespace AGS
{

namespace Common { class Stream; }

namespace Engine
{

using Common::CustomProperties;
using Common::Stream;

struct RoomObject
{
    //-----------------------------------------------------
    // !!IMPORTANT!!
    // The following members should strictly correspond to struct AGSObject
    // from plugin interface
    int32_t X;
    int32_t Y;
    int32_t Transparency; // current transparency setting
    int16_t TintR;
    int16_t TintG;
    int16_t TintB;
    int16_t TintLevel;
    int16_t TintLight;
    int16_t LastZoom;     // zoom level last time
    int16_t LastWidth;
    int16_t LastHeight;   // width/height last time drawn
    int16_t SpriteIndex;  // sprite slot number
    int16_t Baseline;     // <=0 to use Y co-ordinate; >0 for specific baseline
    int16_t View;
    int16_t Loop;
    int16_t Frame;        // only used to track animation - 'SpriteIndex' holds the current sprite
    int16_t Wait;
    int16_t Moving;
    int8_t  Cycling;      // is it currently animating?
    int8_t  OverallSpeed;
    int8_t  IsOn;
    int8_t  Flags;
    // end of plugin export
    //-----------------------------------------------------

    int16_t BlockingWidth;
    int16_t BlockingHeight;

    NewInteraction   Interaction;
    CustomProperties Properties;

    int32_t GetWidth();
    int32_t GetHeight();
    int32_t GetBaseline();

	void UpdateCyclingView();
	void UpdateCycleViewForwards();
	void UpdateCycleViewBackwards();

    void ReadFromFile(Stream *in);
    void WriteToFile(Stream *out);
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__ROOMOBJECT_H
