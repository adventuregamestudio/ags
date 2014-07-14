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
        For,
        Break,
        Continue,
        Do,
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
        TimesEquals,
        OverEquals,
        AndEquals,
        OrEquals,
        XorEquals,
        ShiftLeftEquals,
        ShiftRightEquals,
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
