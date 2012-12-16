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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "math.h"
#include "ac/common.h"
#include "platform/base/override_defines.h"

int FloatToInt(SCRIPT_FLOAT(value), int roundDirection) {
    INIT_SCRIPT_FLOAT(value);

    int intval;

    if (value >= 0.0) {
        if (roundDirection == eRoundDown)
            intval = (int)value;
        else if (roundDirection == eRoundNearest)
            intval = (int)(value + 0.5);
        else if (roundDirection == eRoundUp)
            intval = (int)(value + 0.999999);
        else
            quit("!FloatToInt: invalid round direction");
    }
    else {
        // negative number
        if (roundDirection == eRoundUp)
            intval = (int)value; // this just truncates
        else if (roundDirection == eRoundNearest)
            intval = (int)(value - 0.5);
        else if (roundDirection == eRoundDown)
            intval = (int)(value - 0.999999);
        else
            quit("!FloatToInt: invalid round direction");
    }

    return intval;
}

FLOAT_RETURN_TYPE IntToFloat(int value) {
    float fval = value;

    RETURN_FLOAT(fval);
}

FLOAT_RETURN_TYPE StringToFloat(const char *theString) {
    float fval = atof(theString);

    RETURN_FLOAT(fval);
}

FLOAT_RETURN_TYPE Math_Cos(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = cos(value);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Sin(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = sin(value);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Tan(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = tan(value);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcCos(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = acos(value);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcSin(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = asin(value);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcTan(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = atan(value);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcTan2(SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval)) {
    INIT_SCRIPT_FLOAT(yval);
    INIT_SCRIPT_FLOAT(xval);

    float value = atan2(yval, xval);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Log(SCRIPT_FLOAT(num)) {
    INIT_SCRIPT_FLOAT(num);

    float value = log(num);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Log10(SCRIPT_FLOAT(num)) {
    INIT_SCRIPT_FLOAT(num);

    float value = ::log10(num);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Exp(SCRIPT_FLOAT(num)) {
    INIT_SCRIPT_FLOAT(num);

    float value = exp(num);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Cosh(SCRIPT_FLOAT(num)) {
    INIT_SCRIPT_FLOAT(num);

    float value = cosh(num);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Sinh(SCRIPT_FLOAT(num)) {
    INIT_SCRIPT_FLOAT(num);

    float value = sinh(num);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Tanh(SCRIPT_FLOAT(num)) {
    INIT_SCRIPT_FLOAT(num);

    float value = tanh(num);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_RaiseToPower(SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp)) {
    INIT_SCRIPT_FLOAT(base);
    INIT_SCRIPT_FLOAT(exp);

    float value = ::pow(base, exp);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_DegreesToRadians(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = value * (M_PI / 180.0);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_RadiansToDegrees(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    value = value * (180.0 / M_PI);

    RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_GetPi() {
    float pi = M_PI;

    RETURN_FLOAT(pi);
}

FLOAT_RETURN_TYPE Math_Sqrt(SCRIPT_FLOAT(value)) {
    INIT_SCRIPT_FLOAT(value);

    if (value < 0.0)
        quit("!Sqrt: cannot perform square root of negative number");

    value = ::sqrt(value);

    RETURN_FLOAT(value);
}

int __Rand(int upto) {
    upto++;
    if (upto < 1)
        quit("!Random: invalid parameter passed -- must be at least 0.");
    return rand()%upto;
}


//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_ArcCos(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_ArcCos)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_ArcSin(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_ArcSin)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_ArcTan(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_ArcTan)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval))
RuntimeScriptValue Sc_Math_ArcTan2(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Math_ArcTan2)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_Cos(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Cos)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(num))
RuntimeScriptValue Sc_Math_Cosh(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Cosh)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_DegreesToRadians(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_DegreesToRadians)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(num))
RuntimeScriptValue Sc_Math_Exp(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Exp)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(num))
RuntimeScriptValue Sc_Math_Log(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Log)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(num))
RuntimeScriptValue Sc_Math_Log10(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Log10)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_RadiansToDegrees(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_RadiansToDegrees)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp))
RuntimeScriptValue Sc_Math_RaiseToPower(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Math_RaiseToPower)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_Sin(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Sin)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(num))
RuntimeScriptValue Sc_Math_Sinh(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Sinh)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_Sqrt(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Sqrt)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(value))
RuntimeScriptValue Sc_Math_Tan(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Tan)
}

// FLOAT_RETURN_TYPE (SCRIPT_FLOAT(num))
RuntimeScriptValue Sc_Math_Tanh(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Tanh)
}

// FLOAT_RETURN_TYPE ()
RuntimeScriptValue Sc_Math_GetPi(void *self, RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Math_GetPi)
}


void RegisterMathAPI()
{
    ccAddExternalStaticFunction("Maths::ArcCos^1",              Sc_Math_ArcCos);
    ccAddExternalStaticFunction("Maths::ArcSin^1",              Sc_Math_ArcSin);
    ccAddExternalStaticFunction("Maths::ArcTan^1",              Sc_Math_ArcTan);
    ccAddExternalStaticFunction("Maths::ArcTan2^2",             Sc_Math_ArcTan2);
    ccAddExternalStaticFunction("Maths::Cos^1",                 Sc_Math_Cos);
    ccAddExternalStaticFunction("Maths::Cosh^1",                Sc_Math_Cosh);
    ccAddExternalStaticFunction("Maths::DegreesToRadians^1",    Sc_Math_DegreesToRadians);
    ccAddExternalStaticFunction("Maths::Exp^1",                 Sc_Math_Exp);
    ccAddExternalStaticFunction("Maths::Log^1",                 Sc_Math_Log);
    ccAddExternalStaticFunction("Maths::Log10^1",               Sc_Math_Log10);
    ccAddExternalStaticFunction("Maths::RadiansToDegrees^1",    Sc_Math_RadiansToDegrees);
    ccAddExternalStaticFunction("Maths::RaiseToPower^2",        Sc_Math_RaiseToPower);
    ccAddExternalStaticFunction("Maths::Sin^1",                 Sc_Math_Sin);
    ccAddExternalStaticFunction("Maths::Sinh^1",                Sc_Math_Sinh);
    ccAddExternalStaticFunction("Maths::Sqrt^1",                Sc_Math_Sqrt);
    ccAddExternalStaticFunction("Maths::Tan^1",                 Sc_Math_Tan);
    ccAddExternalStaticFunction("Maths::Tanh^1",                Sc_Math_Tanh);
    ccAddExternalStaticFunction("Maths::get_Pi",                Sc_Math_GetPi);
}
