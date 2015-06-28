using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class DefaultTokenStore
	{
		private static List<Token> _DefaultTokens = null;

		public static List<Token> GetTokenList()
		{
			if (_DefaultTokens == null)
			{
				_DefaultTokens = new List<Token>();
				_DefaultTokens.Add(new ScalarVariableTypeToken("int", 4));
				_DefaultTokens.Add(new ScalarVariableTypeToken("char", 1));
				_DefaultTokens.Add(new ScalarVariableTypeToken("long", 4));
				_DefaultTokens.Add(new ScalarVariableTypeToken("short", 2));
				_DefaultTokens.Add(new ScalarVariableTypeToken("void", 0));
				_DefaultTokens.Add(new ScalarVariableTypeToken("float", 4));
                _DefaultTokens.Add(new ScalarVariableTypeToken("string", 4, true));
				_DefaultTokens.Add(new OperatorToken("!", 1, Opcodes.SCMD_NOTREG, RequiredState.NotAllowed));
				_DefaultTokens.Add(new OperatorToken("*", 2, Opcodes.SCMD_MULREG));
				_DefaultTokens.Add(new OperatorToken("/", 3, Opcodes.SCMD_DIVREG));
				_DefaultTokens.Add(new OperatorToken("%", 4, Opcodes.SCMD_MODREG));
				_DefaultTokens.Add(new OperatorToken("+", 5, Opcodes.SCMD_ADDREG));
                _DefaultTokens.Add(new OperatorToken("-", 5, Opcodes.SCMD_SUBREG, RequiredState.Optional));
				_DefaultTokens.Add(new OperatorToken("<<", 7, Opcodes.SCMD_SHIFTLEFT));
				_DefaultTokens.Add(new OperatorToken(">>", 8, Opcodes.SCMD_SHIFTRIGHT));
				_DefaultTokens.Add(new OperatorToken("&", 9, Opcodes.SCMD_BITAND));
				_DefaultTokens.Add(new OperatorToken("|", 10, Opcodes.SCMD_BITOR));
				_DefaultTokens.Add(new OperatorToken("^", 11, Opcodes.SCMD_XORREG));
				_DefaultTokens.Add(new OperatorToken("==", 12, Opcodes.SCMD_ISEQUAL));
				_DefaultTokens.Add(new OperatorToken("!=", 13, Opcodes.SCMD_NOTEQUAL));
				_DefaultTokens.Add(new OperatorToken(">", 14, Opcodes.SCMD_GREATER));
				_DefaultTokens.Add(new OperatorToken("<", 15, Opcodes.SCMD_LESSTHAN));
				_DefaultTokens.Add(new OperatorToken(">=", 16, Opcodes.SCMD_GTE));
				_DefaultTokens.Add(new OperatorToken("<=", 17, Opcodes.SCMD_LTE));
				_DefaultTokens.Add(new OperatorToken("&&", 18, Opcodes.SCMD_AND));
				_DefaultTokens.Add(new OperatorToken("||", 19, Opcodes.SCMD_OR));
				_DefaultTokens.Add(new KeywordToken("=", PredefinedSymbol.SetEqual, true));
				_DefaultTokens.Add(new KeywordToken(";", PredefinedSymbol.Semicolon));
				_DefaultTokens.Add(new KeywordToken(",", PredefinedSymbol.Comma));
				_DefaultTokens.Add(new KeywordToken("(", PredefinedSymbol.OpenParenthesis));
				_DefaultTokens.Add(new KeywordToken(")", PredefinedSymbol.CloseParenthesis));
				_DefaultTokens.Add(new KeywordToken("{", PredefinedSymbol.OpenBrace));
				_DefaultTokens.Add(new KeywordToken("}", PredefinedSymbol.CloseBrace));
				_DefaultTokens.Add(new KeywordToken("[", PredefinedSymbol.OpenSquareBracket));
				_DefaultTokens.Add(new KeywordToken("]", PredefinedSymbol.CloseSquareBracket));
				_DefaultTokens.Add(new KeywordToken(".", PredefinedSymbol.Dot));
				_DefaultTokens.Add(new KeywordToken("+=", PredefinedSymbol.PlusEquals, true));
				_DefaultTokens.Add(new KeywordToken("-=", PredefinedSymbol.MinusEquals, true));
				_DefaultTokens.Add(new KeywordToken("*=", PredefinedSymbol.TimesEquals, true));
				_DefaultTokens.Add(new KeywordToken("/=", PredefinedSymbol.OverEquals, true));
				_DefaultTokens.Add(new KeywordToken("&=", PredefinedSymbol.AndEquals, true));
				_DefaultTokens.Add(new KeywordToken("|=", PredefinedSymbol.OrEquals, true));
				_DefaultTokens.Add(new KeywordToken("^=", PredefinedSymbol.XorEquals, true));
				_DefaultTokens.Add(new KeywordToken("<<=", PredefinedSymbol.ShiftLeftEquals, true));
				_DefaultTokens.Add(new KeywordToken(">>=", PredefinedSymbol.ShiftRightEquals, true));
				_DefaultTokens.Add(new KeywordToken("++", PredefinedSymbol.PlusPlus, true));
				_DefaultTokens.Add(new KeywordToken("--", PredefinedSymbol.MinusMinus, true));
				_DefaultTokens.Add(new KeywordToken("if", PredefinedSymbol.If));
				_DefaultTokens.Add(new KeywordToken("else", PredefinedSymbol.Else));
				_DefaultTokens.Add(new KeywordToken("while", PredefinedSymbol.While));
				_DefaultTokens.Add(new KeywordToken("for", PredefinedSymbol.For));
				_DefaultTokens.Add(new KeywordToken("break", PredefinedSymbol.Break));
				_DefaultTokens.Add(new KeywordToken("continue", PredefinedSymbol.Continue));
				_DefaultTokens.Add(new KeywordToken("do", PredefinedSymbol.Do));
				_DefaultTokens.Add(new KeywordToken("switch", PredefinedSymbol.Switch));
				_DefaultTokens.Add(new KeywordToken("case", PredefinedSymbol.Case));
				_DefaultTokens.Add(new KeywordToken("default", PredefinedSymbol.Default));
				_DefaultTokens.Add(new KeywordToken("...", PredefinedSymbol.VariableArguments));
				_DefaultTokens.Add(new KeywordToken("struct", PredefinedSymbol.StructDefinition));
				_DefaultTokens.Add(new KeywordToken("return", PredefinedSymbol.Return));
				_DefaultTokens.Add(new KeywordToken("::", PredefinedSymbol.MemberOf));
				_DefaultTokens.Add(new KeywordToken(":", PredefinedSymbol.Label));
				_DefaultTokens.Add(new KeywordToken("enum", PredefinedSymbol.Enum));
				_DefaultTokens.Add(new KeywordToken("null", PredefinedSymbol.Null));
				_DefaultTokens.Add(new KeywordToken("extends", PredefinedSymbol.Extends));
				_DefaultTokens.Add(new KeywordToken("export", PredefinedSymbol.Export));
				_DefaultTokens.Add(new ModifierToken("import", PredefinedSymbol.Import, ModifierTargets.GlobalFunction | ModifierTargets.GlobalVariable | ModifierTargets.MemberFunction | ModifierTargets.MemberVariable));
				_DefaultTokens.Add(new ModifierToken("readonly", PredefinedSymbol.ReadOnly, ModifierTargets.GlobalVariable | ModifierTargets.MemberVariable));
				_DefaultTokens.Add(new ModifierToken("attribute", PredefinedSymbol.Attribute, ModifierTargets.MemberVariable));
				_DefaultTokens.Add(new ModifierToken("managed", PredefinedSymbol.Managed, ModifierTargets.Struct));
				_DefaultTokens.Add(new ModifierToken("static", PredefinedSymbol.Static, ModifierTargets.MemberFunction | ModifierTargets.MemberVariable));
				_DefaultTokens.Add(new ModifierToken("protected", PredefinedSymbol.Protected, ModifierTargets.MemberFunction | ModifierTargets.MemberVariable));
				_DefaultTokens.Add(new ModifierToken("writeprotected", PredefinedSymbol.WriteProtected, ModifierTargets.MemberFunction | ModifierTargets.MemberVariable));
				_DefaultTokens.Add(new ModifierToken("const", PredefinedSymbol.Const, ModifierTargets.FunctionParameter));
				_DefaultTokens.Add(new ModifierToken("internalstring", PredefinedSymbol.InternalString, ModifierTargets.Struct));
				_DefaultTokens.Add(new ModifierToken("autoptr", PredefinedSymbol.AutoPtr, ModifierTargets.Struct));
				_DefaultTokens.Add(new ModifierToken("noloopcheck", PredefinedSymbol.NoLoopCheck, ModifierTargets.Function));
			}

			return _DefaultTokens;
		}
	}
}
