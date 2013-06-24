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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SLIDER_H
#define __AGS_EE_AC__SLIDER_H

#include "gui/guislider.h"

using AGS::Common::GuiSlider;

void	Slider_SetMax(GuiSlider *guisl, int valn);
int		Slider_GetMax(GuiSlider *guisl);
void	Slider_SetMin(GuiSlider *guisl, int valn);
int		Slider_GetMin(GuiSlider *guisl);
void	Slider_SetValue(GuiSlider *guisl, int valn);
int		Slider_GetValue(GuiSlider *guisl);
int		Slider_GetBackgroundGraphic(GuiSlider *guisl);
void	Slider_SetBackgroundGraphic(GuiSlider *guisl, int newImage);
int		Slider_GetHandleGraphic(GuiSlider *guisl);
void	Slider_SetHandleGraphic(GuiSlider *guisl, int newImage);
int		Slider_GetHandleOffset(GuiSlider *guisl);
void	Slider_SetHandleOffset(GuiSlider *guisl, int newOffset);

#endif // __AGS_EE_AC__SLIDER_H
