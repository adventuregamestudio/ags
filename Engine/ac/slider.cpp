//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/slider.h"
#include "ac/common.h"
#include "debug/debug_log.h"

using namespace AGS::Common;

void Slider_SetMax(GUISlider *guisl, int valn) {
    guisl->SetMaxValue(valn);
}

int Slider_GetMax(GUISlider *guisl) {
    return guisl->GetMaxValue();
}

void Slider_SetMin(GUISlider *guisl, int valn) {
    guisl->SetMinValue(valn);
}

int Slider_GetMin(GUISlider *guisl) {
    return guisl->GetMinValue();
}

void Slider_SetValue(GUISlider *guisl, int valn) {
    guisl->SetValue(valn);
}

int Slider_GetValue(GUISlider *guisl) {
    return guisl->GetValue();
}

int Slider_GetBackgroundGraphic(GUISlider *guisl) {
    return (guisl->GetBgImage() > 0) ? guisl->GetBgImage() : 0;
}

void Slider_SetBackgroundGraphic(GUISlider *guisl, int newImage) 
{
    guisl->SetBgImage(newImage);
}

int Slider_GetHandleGraphic(GUISlider *guisl) {
    return (guisl->GetHandleImage() > 0) ? guisl->GetHandleImage() : 0;
}

void Slider_SetHandleGraphic(GUISlider *guisl, int newImage) 
{
    guisl->SetHandleImage(newImage);
}

int Slider_GetHandleOffset(GUISlider *guisl) {
    return guisl->GetHandleOffset();
}

void Slider_SetHandleOffset(GUISlider *guisl, int newOffset) 
{
    guisl->SetHandleOffset(newOffset);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int (GUISlider *guisl)
RuntimeScriptValue Sc_Slider_GetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetBackgroundGraphic);
}

// void (GUISlider *guisl, int newImage)
RuntimeScriptValue Sc_Slider_SetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetBackgroundGraphic);
}

// int (GUISlider *guisl)
RuntimeScriptValue Sc_Slider_GetHandleGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleGraphic);
}

// void (GUISlider *guisl, int newImage)
RuntimeScriptValue Sc_Slider_SetHandleGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleGraphic);
}

// int (GUISlider *guisl)
RuntimeScriptValue Sc_Slider_GetHandleOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleOffset);
}

// void (GUISlider *guisl, int newOffset)
RuntimeScriptValue Sc_Slider_SetHandleOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleOffset);
}

// int (GUISlider *guisl)
RuntimeScriptValue Sc_Slider_GetMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetMax);
}

// void (GUISlider *guisl, int valn)
RuntimeScriptValue Sc_Slider_SetMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetMax);
}

// int (GUISlider *guisl)
RuntimeScriptValue Sc_Slider_GetMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetMin);
}

// void (GUISlider *guisl, int valn)
RuntimeScriptValue Sc_Slider_SetMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetMin);
}

// int (GUISlider *guisl)
RuntimeScriptValue Sc_Slider_GetValue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetValue);
}

// void Slider_SetValue(GUISlider *guisl, int valn)
RuntimeScriptValue Sc_Slider_SetValue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetValue);
}


void RegisterSliderAPI()
{
    ScFnRegister slider_api[] = {
        { "Slider::get_BackgroundGraphic",    API_FN_PAIR(Slider_GetBackgroundGraphic) },
        { "Slider::set_BackgroundGraphic",    API_FN_PAIR(Slider_SetBackgroundGraphic) },
        { "Slider::get_HandleGraphic",        API_FN_PAIR(Slider_GetHandleGraphic) },
        { "Slider::set_HandleGraphic",        API_FN_PAIR(Slider_SetHandleGraphic) },
        { "Slider::get_HandleOffset",         API_FN_PAIR(Slider_GetHandleOffset) },
        { "Slider::set_HandleOffset",         API_FN_PAIR(Slider_SetHandleOffset) },
        { "Slider::get_Max",                  API_FN_PAIR(Slider_GetMax) },
        { "Slider::set_Max",                  API_FN_PAIR(Slider_SetMax) },
        { "Slider::get_Min",                  API_FN_PAIR(Slider_GetMin) },
        { "Slider::set_Min",                  API_FN_PAIR(Slider_SetMin) },
        { "Slider::get_Value",                API_FN_PAIR(Slider_GetValue) },
        { "Slider::set_Value",                API_FN_PAIR(Slider_SetValue) },
    };

    ccAddExternalFunctions(slider_api);
}
