
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__MATH_H
#define __AGS_EE_AC__MATH_H

// MACPORT FIX 9/6/5: undef M_PI first
#undef M_PI
#define M_PI 3.14159265358979323846

// unfortunately MSVC and GCC automatically push floats as doubles
// to functions, thus we need to manually access it as 32-bit
#define SCRIPT_FLOAT(x) long __script_float##x
#define INIT_SCRIPT_FLOAT(x) float x; memcpy(&x, &__script_float##x, sizeof(float))
#define FLOAT_RETURN_TYPE long
#define RETURN_FLOAT(x) long __ret##x; memcpy(&__ret##x, &x, sizeof(float)); return __ret##x

enum RoundDirections {
    eRoundDown = 0,
    eRoundNearest = 1,
    eRoundUp = 2
};


int FloatToInt(SCRIPT_FLOAT(value), int roundDirection);
FLOAT_RETURN_TYPE IntToFloat(int value);
FLOAT_RETURN_TYPE StringToFloat(const char *theString);
FLOAT_RETURN_TYPE Math_Cos(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_Sin(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_Tan(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcCos(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcSin(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcTan(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_ArcTan2(SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval));
FLOAT_RETURN_TYPE Math_Log(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Log10(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Exp(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Cosh(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Sinh(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_Tanh(SCRIPT_FLOAT(num));
FLOAT_RETURN_TYPE Math_RaiseToPower(SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp));
FLOAT_RETURN_TYPE Math_DegreesToRadians(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_RadiansToDegrees(SCRIPT_FLOAT(value));
FLOAT_RETURN_TYPE Math_GetPi();
FLOAT_RETURN_TYPE Math_Sqrt(SCRIPT_FLOAT(value));

int __Rand(int upto);
#define Random __Rand

#endif // __AGS_EE_AC__MATH_H
