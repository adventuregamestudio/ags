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
#ifndef __AC_GUISLIDER_H
#define __AC_GUISLIDER_H

#include <vector>
#include "gui/guiobject.h"

namespace AGS
{
namespace Common
{

class GUISlider : public GUIObject
{
public:
    GUISlider();

    // Tells if the slider is horizontal (otherwise - vertical)
    bool IsHorizontal() const;
    bool IsOverControl(int x, int y, int leeway) const override;
    // Compatibility: sliders are not clipped as of 3.6.0
    bool IsContentClipped() const override { return false; }
    bool HasAlphaChannel() const override;

    // Operations
    Rect CalcGraphicRect(bool clipped) override;
    void Draw(Bitmap *ds, int x = 0, int y = 0) override;

    // Events
    bool OnMouseDown() override;
    void OnMouseMove(int xp, int yp) override;
    void OnMouseUp() override;

    // Serialization
    void ReadFromFile(Stream *in, GuiVersion gui_version) override;
    void WriteToFile(Stream *out) const override;
    void ReadFromSavegame(Stream *in, GuiSvgVersion svg_ver) override;
    void WriteToSavegame(Stream *out) const override;

// TODO: these members are currently public; hide them later
public:
    int32_t MinValue;
    int32_t MaxValue;
    int32_t Value;
    int32_t BgImage;
    int32_t HandleImage;
    int32_t HandleOffset;
    bool    IsMousePressed;

private:
    // Updates dynamic metrics and positions of elements
    void UpdateMetrics();

    // Cached coordinates of slider bar; in relative coords
    Rect    _cachedBar;
    // Cached coordinates of slider handle; in relative coords
    Rect    _cachedHandle;
    // The length of the handle movement range, in pixels
    int     _handleRange;
};

} // namespace Common
} // namespace AGS

extern std::vector<AGS::Common::GUISlider> guislider;

#endif // __AC_GUISLIDER_H
