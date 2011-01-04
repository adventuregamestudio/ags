using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class SplitExpression : Expression
    {
        public Expression LeftHandSide;
        public Expression RightHandSide;
        public Token Operator;

        public SplitExpression(Token splitOperator, Expression leftHandSide, Expression rightHandSide, Expression completeExpression)
            : base(completeExpression)
        {
            this.Operator = splitOperator;
            this.LeftHandSide = leftHandSide;
            this.RightHandSide = rightHandSide;
        }

        public override string ToString()
        {
            return "SPLITEXPR";
        }
    }
}
