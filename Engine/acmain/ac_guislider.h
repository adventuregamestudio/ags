
#include "acgui/ac_guislider.h"

void Slider_SetMax(GUISlider *guisl, int valn);
int Slider_GetMax(GUISlider *guisl);
void Slider_SetMin(GUISlider *guisl, int valn);
int Slider_GetMin(GUISlider *guisl);
void Slider_SetValue(GUISlider *guisl, int valn);
int Slider_GetValue(GUISlider *guisl);
void SetSliderValue(int guin,int objn, int valn);
int GetSliderValue(int guin,int objn);
int Slider_GetBackgroundGraphic(GUISlider *guisl);
void Slider_SetBackgroundGraphic(GUISlider *guisl, int newImage);
int Slider_GetHandleGraphic(GUISlider *guisl);
void Slider_SetHandleGraphic(GUISlider *guisl, int newImage);
int Slider_GetHandleOffset(GUISlider *guisl);
void Slider_SetHandleOffset(GUISlider *guisl, int newOffset);