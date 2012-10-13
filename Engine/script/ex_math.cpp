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
// Exporting Math script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_math_script_functions()
{
	ccAddExternalStaticFunction("Maths::ArcCos^1", (void*)Math_ArcCos);
	ccAddExternalStaticFunction("Maths::ArcSin^1", (void*)Math_ArcSin);
	ccAddExternalStaticFunction("Maths::ArcTan^1", (void*)Math_ArcTan);
	ccAddExternalStaticFunction("Maths::ArcTan2^2", (void*)Math_ArcTan2);
	ccAddExternalStaticFunction("Maths::Cos^1", (void*)Math_Cos);
	ccAddExternalStaticFunction("Maths::Cosh^1", (void*)Math_Cosh);
	ccAddExternalStaticFunction("Maths::DegreesToRadians^1", (void*)Math_DegreesToRadians);
	ccAddExternalStaticFunction("Maths::Exp^1", (void*)Math_Exp);
	ccAddExternalStaticFunction("Maths::Log^1", (void*)Math_Log);
	ccAddExternalStaticFunction("Maths::Log10^1", (void*)Math_Log10);
	ccAddExternalStaticFunction("Maths::RadiansToDegrees^1", (void*)Math_RadiansToDegrees);
	ccAddExternalStaticFunction("Maths::RaiseToPower^2", (void*)Math_RaiseToPower);
	ccAddExternalStaticFunction("Maths::Sin^1", (void*)Math_Sin);
	ccAddExternalStaticFunction("Maths::Sinh^1", (void*)Math_Sinh);
	ccAddExternalStaticFunction("Maths::Sqrt^1", (void*)Math_Sqrt);
	ccAddExternalStaticFunction("Maths::Tan^1", (void*)Math_Tan);
	ccAddExternalStaticFunction("Maths::Tanh^1", (void*)Math_Tanh);
	ccAddExternalStaticFunction("Maths::get_Pi", (void*)Math_GetPi);
}
