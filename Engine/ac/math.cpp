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
#include "ac/math.h"
#include <cmath>
#include "ac/common.h" // quit
#include "debug/debug_log.h"
#include "util/math.h"

using namespace AGS::Common;

float RoundImpl(const char *apiname, float value, int roundDirection)
{
    switch (roundDirection)
    {
    case eRoundDown:
        return std::floor(value);
    case eRoundUp:
        return std::ceil(value);
    case eRoundNearest:
        return std::round(value);
    case eRoundTowardsZero:
        return std::trunc(value);
    case eRoundAwayFromZero:
        return value < 0.f ? std::floor(value) : std::ceil(value);
    default:
        debug_script_warn("!%s: invalid round direction %d", apiname, roundDirection);
        return value;
    }
    return 0.f;
}

int FloatToInt(float value, int roundDirection)
{
    return static_cast<int>(RoundImpl("FloatToInt", value, roundDirection));
}

float IntToFloat(int value)
{
    return static_cast<float>(value);
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
    return static_cast<float>(Math::DegreesToRadians(value));
}

float Math_RadiansToDegrees(float value)
{
    return static_cast<float>(Math::RadiansToDegrees(value));
}

int Math_Random(int limit)
{
    // NOTE: we clamp rand max to INT16_MAX for cross-platform compatibility;
    // perhaps replacing rand with another random number generator would solve this problem.
    if (limit <= 0 || limit > INT16_MAX + 1)
    {
        debug_script_warn("!Maths.Random: invalid parameter %d -- must be in range (1..%d)", limit, INT16_MAX + 1);
        return 0;
    }

    return rand() % (Math::Clamp<int>(limit, 0, INT16_MAX + 1));
}

float Math_RandomFloat()
{
    return static_cast<float>(static_cast<double>(rand()) / (RAND_MAX + 1.0));
}

float Math_Round(float value, int roundDirection)
{
    return RoundImpl("Math.Round", value, roundDirection);
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

int Random(int upto)
{
    if (upto < 0 || upto > RAND_MAX)
    {
        debug_script_warn("!Random: invalid parameter %d -- must be in range (0..%d)", upto, RAND_MAX);
        return 0;
    }

    return rand() % (++upto);
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

RuntimeScriptValue Sc_Math_Random(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Math_Random);
}

RuntimeScriptValue Sc_Math_RandomFloat(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Math_RandomFloat);
}

RuntimeScriptValue Sc_Math_Round(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PFLOAT_PINT(Math_Round);
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
        { "Maths::Random^1",              API_FN_PAIR(Math_Random) },
        { "Maths::RandomFloat^0",         API_FN_PAIR(Math_RandomFloat) },
        { "Maths::Round^2",               API_FN_PAIR(Math_Round) },
        { "Maths::Sin^1",                 API_FN_PAIR(Math_Sin) },
        { "Maths::Sinh^1",                API_FN_PAIR(Math_Sinh) },
        { "Maths::Sqrt^1",                API_FN_PAIR(Math_Sqrt) },
        { "Maths::Tan^1",                 API_FN_PAIR(Math_Tan) },
        { "Maths::Tanh^1",                API_FN_PAIR(Math_Tanh) },
        { "Maths::get_Pi",                API_FN_PAIR(Math_GetPi) },
    };

    ccAddExternalFunctions(math_api);
}
