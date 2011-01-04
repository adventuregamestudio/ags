using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal enum PredefinedSymbol
	{
		None,
		StructDefinition,
		If,
		Else,
		While,
		VariableArguments,
		Return,
		MemberOf,
		Enum,
		Null,
		Extends,
		Export,
		SetEqual,
		Semicolon,
		Comma,
		OpenParenthesis,
		CloseParenthesis,
		OpenBrace,
		CloseBrace,
		OpenSquareBracket,
		CloseSquareBracket,
		Dot,
		PlusEquals,
		MinusEquals,
		PlusPlus,
		MinusMinus,
		Import,
		ReadOnly,
		Attribute,
		Managed,
		Static,
		Protected,
		WriteProtected,
		Const,
		InternalString,
		AutoPtr,
		NoLoopCheck
	}
}
