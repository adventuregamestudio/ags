using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class CompilerUtils
    {
        public static bool AdjustBracketLevelIfTokenIsBracket(Token token, ref int bracketLevel)
        {
            if (token is KeywordToken)
            {
                PredefinedSymbol symbol = ((KeywordToken)token).SymbolType;
                if ((symbol == PredefinedSymbol.OpenBrace) ||
                    (symbol == PredefinedSymbol.OpenParenthesis) ||
                    (symbol == PredefinedSymbol.OpenSquareBracket))
                {
                    bracketLevel++;
                    return true;
                }
                else if ((symbol == PredefinedSymbol.CloseBrace) ||
                    (symbol == PredefinedSymbol.CloseParenthesis) ||
                    (symbol == PredefinedSymbol.CloseSquareBracket))
                {
                    bracketLevel--;
                    return true;
                }
            }
            return false;
        }

        public static void SetArrayPropertiesOnTokenFromStream(ScriptReader source, Token variableName)
        {
            if (source.NextIsKeyword(PredefinedSymbol.OpenSquareBracket))
            {
                variableName.IsArray = true;

                if (source.NextIsKeyword(PredefinedSymbol.CloseSquareBracket))
                {
                    variableName.ArraySize = Token.ARRAY_SIZE_DYNAMIC;
                }
                else
                {
                    variableName.ArraySize = source.ReadNextAsConstInt();
                    source.ExpectKeyword(PredefinedSymbol.CloseSquareBracket);
                }
            }
        }

        public static void VerifyModifiersAgainstType(ModifierTargets target, List<ModifierToken> modifiers)
        {
            foreach (ModifierToken modifier in modifiers)
            {
                if ((modifier.Targets & target) == 0)
                {
                    throw new CompilerMessage(ErrorCode.InvalidUseOfKeyword, "Invalid use of '" + modifier.Name + "'");
                }
            }
        }

        public static Token GetTokenForStructMember(TokenizedScript script, Token structName, Token memberName, out string mangledName)
        {
            mangledName = structName.Name + "::" + memberName.Name;
            return script.FindTokenWithName(mangledName);
        }

    }
}
