using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class Expression : List<Token>
    {
        private const int MODIFICATION_OPERATOR_PRECEDENCE = 50;

        public Expression() { }

        public Expression(List<Token> copyFrom)
        {
            this.AddRange(copyFrom);
        }

        public Expression SplitExpressionOnLowestPrecendenceOperator()
        {
            int splitAtIndex = FindIndexOfLowestPrecedenceOperator();
            if (splitAtIndex < 0)
            {
                return this;
            }
            Expression leftHandSide = new Expression(this.GetRange(0, splitAtIndex));
            Expression rightHandSide = new Expression(this.GetRange(splitAtIndex + 1, (this.Count - splitAtIndex) - 1));
            return new SplitExpression(this[splitAtIndex], leftHandSide, rightHandSide, this);
        }

        private int FindIndexOfLowestPrecedenceOperator()
        {
            int foundIndex = -1;
            int foundPrecedence = -1;
            int bracketLevel = 0;

            for (int i = 0; i < this.Count; i++)
            {
                if (CompilerUtils.AdjustBracketLevelIfTokenIsBracket(this[i], ref bracketLevel))
                {
                    // do nothing, the bracketlevel has been changed
                }
                else if (bracketLevel == 0)
                {
                    if (this[i] is OperatorToken)
                    {
                        int thisPrecedence = ((OperatorToken)this[i]).Precedence;
                        if (thisPrecedence > foundPrecedence)
                        {
                            foundPrecedence = thisPrecedence;
                            foundIndex = i;
                        }
                    }
                    else if (this[i] is KeywordToken)
                    {
                        if (((KeywordToken)this[i]).IsModificationOperator)
                        {
                            if (MODIFICATION_OPERATOR_PRECEDENCE > foundPrecedence)
                            {
                                foundPrecedence = MODIFICATION_OPERATOR_PRECEDENCE;
                                foundIndex = i;
                            }
                        }
                    }
                }
            }

            return foundIndex;
        }

        public override string ToString()
        {
            string result = "EXPR: ";
            foreach (Token token in this)
            {
                result += token.Name + " ";
            }
            return result;
        }
    }
}
