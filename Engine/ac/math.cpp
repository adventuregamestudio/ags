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

#include "ac/math.h"
#include <cmath>
#include "ac/common.h" // quit
#include "util/math.h"

int FloatToInt(float value, int roundDirection)
{
    switch (roundDirection)
    {
    case eRoundDown:
        return static_cast<int>(std::floor(value));
    case eRoundUp:
        return static_cast<int>(std::ceil(value));
    case eRoundNearest:
        return static_cast<int>(std::round(value));
    default:
        quit("!FloatToInt: invalid round direction");
    }
    return 0;
}

float IntToFloat(int value)
{
    return static_cast<float>(value);
}

float StringToFloat(const char *theString)
{
    return static_cast<float>(atof(theString));
}

float Math_Cos(float value)
{
    return cos(value);
}

float Math_Sin(float value)
{
    return sin(value);
}

float Math_Tan(float value)
{
    return tan(value);
}

float Math_ArcCos(float value)
{
    return acos(value);
}

float Math_ArcSin(float value)
{
    return asin(value);
}

float Math_ArcTan(float value)
{
    return atan(value);
}

float Math_ArcTan2(float yval, float xval)
{
    return atan2(yval, xval);
}

float Math_Log(float value)
{
    return log(value);
}

float Math_Log10(float value)
{
    return ::log10(value);
}

float Math_Exp(float value)
{
    return exp(value);
}

float Math_Cosh(float value)
{
    return cosh(value);
}

float Math_Sinh(float value)
{
    return sinh(value);
}

float Math_Tanh(float value)
{
    return tanh(value);
}

float Math_RaiseToPower(float base, float exp)
{
    return ::pow(base, exp);
}

float Math_DegreesToRadians(float value)
{
    return static_cast<float>(value * (M_PI / 180.0));
}

float Math_RadiansToDegrees(float value)
{
    return static_cast<float>(value * (180.0 / M_PI));
}

float Math_GetPi()
{
    return static_cast<float>(M_PI);
}

float Math_Sqrt(float value)
{
    if (value < 0.0)
        quit("!Sqrt: cannot perform square root of negative number");

    return ::sqrt(value);
}

int __Rand(int upto)
{
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

// float (float value)
RuntimeScriptValue Sc_Math_ArcCos(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_ArcCos);
}

// float (float value)
RuntimeScriptValue Sc_Math_ArcSin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_ArcSin);
}

// float (float value)
RuntimeScriptValue Sc_Math_ArcTan(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_ArcTan);
}

// float (SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval))
RuntimeScriptValue Sc_Math_ArcTan2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT2(Math_ArcTan2);
}

// float (float value)
RuntimeScriptValue Sc_Math_Cos(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Cos);
}

// float (float value)
RuntimeScriptValue Sc_Math_Cosh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Cosh);
}

// float (float value)
RuntimeScriptValue Sc_Math_DegreesToRadians(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_DegreesToRadians);
}

// float (float value)
RuntimeScriptValue Sc_Math_Exp(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Exp);
}

// float (float value)
RuntimeScriptValue Sc_Math_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Log);
}

// float (float value)
RuntimeScriptValue Sc_Math_Log10(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Log10);
}

// float (float value)
RuntimeScriptValue Sc_Math_RadiansToDegrees(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_RadiansToDegrees);
}

// float (SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp))
RuntimeScriptValue Sc_Math_RaiseToPower(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT2(Math_RaiseToPower);
}

// float (float value)
RuntimeScriptValue Sc_Math_Sin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Sin);
}

// float (float value)
RuntimeScriptValue Sc_Math_Sinh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Sinh);
}

// float (float value)
RuntimeScriptValue Sc_Math_Sqrt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Sqrt);
}

// float (float value)
RuntimeScriptValue Sc_Math_Tan(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Tan);
}

// float (float value)
RuntimeScriptValue Sc_Math_Tanh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT(Math_Tanh);
}

// float ()
RuntimeScriptValue Sc_Math_GetPi(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Math_GetPi);
}


void RegisterMathAPI()
{
    ScFnRegister math_api[] = {
        { "Maths::ArcCos^1",              API_FN_PAIR(Math_ArcCos) },
        { "Maths::ArcSin^1",              API_FN_PAIR(Math_ArcSin) },
        { "Maths::ArcTan^1",              API_FN_PAIR(Math_ArcTan) },
        { "Maths::ArcTan2^2",             API_FN_PAIR(Math_ArcTan2) },
        { "Maths::Cos^1",                 API_FN_PAIR(Math_Cos) },
        { "Maths::Cosh^1",                API_FN_PAIR(Math_Cosh) },
        { "Maths::DegreesToRadians^1",    API_FN_PAIR(Math_DegreesToRadians) },
        { "Maths::Exp^1",                 API_FN_PAIR(Math_Exp) },
        { "Maths::Log^1",                 API_FN_PAIR(Math_Log) },
        { "Maths::Log10^1",               API_FN_PAIR(Math_Log10) },
        { "Maths::RadiansToDegrees^1",    API_FN_PAIR(Math_RadiansToDegrees) },
        { "Maths::RaiseToPower^2",        API_FN_PAIR(Math_RaiseToPower) },
        { "Maths::Sin^1",                 API_FN_PAIR(Math_Sin) },
        { "Maths::Sinh^1",                API_FN_PAIR(Math_Sinh) },
        { "Maths::Sqrt^1",                API_FN_PAIR(Math_Sqrt) },
        { "Maths::Tan^1",                 API_FN_PAIR(Math_Tan) },
        { "Maths::Tanh^1",                API_FN_PAIR(Math_Tanh) },
        { "Maths::get_Pi",                API_FN_PAIR(Math_GetPi) },
    };

    ccAddExternalFunctions(math_api);
}
