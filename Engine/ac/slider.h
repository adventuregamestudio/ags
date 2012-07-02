
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SLIDER_H
#define __AGS_EE_AC__SLIDER_H

#include "gui/guislider.h"

void	Slider_SetMax(GUISlider *guisl, int valn);
int		Slider_GetMax(GUISlider *guisl);
void	Slider_SetMin(GUISlider *guisl, int valn);
int		Slider_GetMin(GUISlider *guisl);
void	Slider_SetValue(GUISlider *guisl, int valn);
int		Slider_GetValue(GUISlider *guisl);
int		Slider_GetBackgroundGraphic(GUISlider *guisl);
void	Slider_SetBackgroundGraphic(GUISlider *guisl, int newImage);
int		Slider_GetHandleGraphic(GUISlider *guisl);
void	Slider_SetHandleGraphic(GUISlider *guisl, int newImage);
int		Slider_GetHandleOffset(GUISlider *guisl);
void	Slider_SetHandleOffset(GUISlider *guisl, int newOffset);

#endif // __AGS_EE_AC__SLIDER_H
