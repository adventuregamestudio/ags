
#include "acmain/ac_maindefines.h"

// *** SLIDER FUNCTIONS

void Slider_SetMax(GUISlider *guisl, int valn) {

  if (valn != guisl->max) {
    guisl->max = valn;

    if (guisl->value > guisl->max)
      guisl->value = guisl->max;
    if (guisl->min > guisl->max)
      quit("!Slider.Max: minimum cannot be greater than maximum");

    guis_need_update = 1;
  }

}

int Slider_GetMax(GUISlider *guisl) {
  return guisl->max;
}

void Slider_SetMin(GUISlider *guisl, int valn) {

  if (valn != guisl->min) {
    guisl->min = valn;

    if (guisl->value < guisl->min)
      guisl->value = guisl->min;
    if (guisl->min > guisl->max)
      quit("!Slider.Min: minimum cannot be greater than maximum");

    guis_need_update = 1;
  }

}

int Slider_GetMin(GUISlider *guisl) {
  return guisl->min;
}

void Slider_SetValue(GUISlider *guisl, int valn) {
  if (valn > guisl->max) valn = guisl->max;
  if (valn < guisl->min) valn = guisl->min;

  if (valn != guisl->value) {
    guisl->value = valn;
    guis_need_update = 1;
  }
}

int Slider_GetValue(GUISlider *guisl) {
  return guisl->value;
}

void SetSliderValue(int guin,int objn, int valn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!SetSliderValue: invalid GUI number");
  if (guis[guin].get_control_type(objn)!=GOBJ_SLIDER)
    quit("!SetSliderValue: specified control is not a slider");

  GUISlider*guisl=(GUISlider*)guis[guin].objs[objn];
  Slider_SetValue(guisl, valn);
}

int GetSliderValue(int guin,int objn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!GetSliderValue: invalid GUI number");
  if (guis[guin].get_control_type(objn)!=GOBJ_SLIDER)
    quit("!GetSliderValue: specified control is not a slider");

  GUISlider*guisl=(GUISlider*)guis[guin].objs[objn];
  return Slider_GetValue(guisl);
}

int Slider_GetBackgroundGraphic(GUISlider *guisl) {
  return (guisl->bgimage > 0) ? guisl->bgimage : 0;
}

void Slider_SetBackgroundGraphic(GUISlider *guisl, int newImage) 
{
  if (newImage != guisl->bgimage)
  {
    guisl->bgimage = newImage;
    guis_need_update = 1;
  }
}

int Slider_GetHandleGraphic(GUISlider *guisl) {
  return (guisl->handlepic > 0) ? guisl->handlepic : 0;
}

void Slider_SetHandleGraphic(GUISlider *guisl, int newImage) 
{
  if (newImage != guisl->handlepic)
  {
    guisl->handlepic = newImage;
    guis_need_update = 1;
  }
}

int Slider_GetHandleOffset(GUISlider *guisl) {
  return guisl->handleoffset;
}

void Slider_SetHandleOffset(GUISlider *guisl, int newOffset) 
{
  if (newOffset != guisl->handleoffset)
  {
    guisl->handleoffset = newOffset;
    guis_need_update = 1;
  }
}
