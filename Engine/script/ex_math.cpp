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
	scAdd_External_Symbol("Maths::ArcCos^1", (void*)Math_ArcCos);
	scAdd_External_Symbol("Maths::ArcSin^1", (void*)Math_ArcSin);
	scAdd_External_Symbol("Maths::ArcTan^1", (void*)Math_ArcTan);
	scAdd_External_Symbol("Maths::ArcTan2^2", (void*)Math_ArcTan2);
	scAdd_External_Symbol("Maths::Cos^1", (void*)Math_Cos);
	scAdd_External_Symbol("Maths::Cosh^1", (void*)Math_Cosh);
	scAdd_External_Symbol("Maths::DegreesToRadians^1", (void*)Math_DegreesToRadians);
	scAdd_External_Symbol("Maths::Exp^1", (void*)Math_Exp);
	scAdd_External_Symbol("Maths::Log^1", (void*)Math_Log);
	scAdd_External_Symbol("Maths::Log10^1", (void*)Math_Log10);
	scAdd_External_Symbol("Maths::RadiansToDegrees^1", (void*)Math_RadiansToDegrees);
	scAdd_External_Symbol("Maths::RaiseToPower^2", (void*)Math_RaiseToPower);
	scAdd_External_Symbol("Maths::Sin^1", (void*)Math_Sin);
	scAdd_External_Symbol("Maths::Sinh^1", (void*)Math_Sinh);
	scAdd_External_Symbol("Maths::Sqrt^1", (void*)Math_Sqrt);
	scAdd_External_Symbol("Maths::Tan^1", (void*)Math_Tan);
	scAdd_External_Symbol("Maths::Tanh^1", (void*)Math_Tanh);
	scAdd_External_Symbol("Maths::get_Pi", (void*)Math_GetPi);
}
