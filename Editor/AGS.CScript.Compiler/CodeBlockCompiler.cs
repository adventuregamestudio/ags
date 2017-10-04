using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class CodeBlockCompiler
    {
        private ScriptReader _source;
        private CompilerState _state;

        internal CodeBlockCompiler(CompilerState state)
        {
            _state = state;
        }

        /// <summary>
        /// Starts processing a code block from the input stream. It is assumed
        /// to start with a { or be a single expression.
        /// </summary>
        public void Process(ScriptReader script)
        {
            _source = script;

            ProcessCodeBlock();
        }

        /// <summary>
        /// Starts processing a code block from the input stream. It is assumed
        /// to start with a { or be a single expression.
        /// </summary>
        private void ProcessCodeBlock()
        {
            if (_source.NextIsKeyword(PredefinedSymbol.OpenBrace))
            {
                _state.StartNewScope();

                while (!_source.NextIsKeyword(PredefinedSymbol.CloseBrace))
                {
                    ProcessNextCodeStatement(true);
                }

                FreeLocalVariables(_state.CurrentScope, true);
                _state.EndScope();
            }
            else
            {
                ProcessNextCodeStatement(false);
            }
        }

        private void FreeAllLocalVariablesButLeaveValid()
        {
            foreach (LocalScope scope in _state.AllScopes)
            {
                FreeLocalVariables(scope, false);
            }
        }

        /// <summary>
        /// Frees the memory associated with the specified scope, and
        /// generates code to decrement the stack pointer accordingly.
        /// </summary>
        /// <param name="scope">the scope to free</param>
        /// <param name="unDeclareVars">whether to also un-define the local
        /// variables to prevent further use</param>
        private void FreeLocalVariables(LocalScope scope, bool unDeclareVars)
        {
            foreach (ScriptVariable variable in scope.Members)
            {
                // TODO: Generate code to free local variables
                if (unDeclareVars)
                {
                    // TODO: Undefine local variables
                }
            }
        }

        private void ProcessNextCodeStatement(bool allowVariableDeclarations)
        {
            Token nextToken = _source.PeekNextToken();
            while (nextToken is ModifierToken)
            {
                _source.ReadNextToken();

                _state.NextTokenModifiers.Add((ModifierToken)nextToken);

                nextToken = _source.PeekNextToken();
            }

            if (nextToken is EndOfStreamToken)
            {
                throw new CompilerMessage(ErrorCode.EndOfInputReached, "The end of the script was reached in the middle of a function");
            }

            if ((nextToken is KeywordToken) &&
                (((KeywordToken)nextToken).SymbolType == PredefinedSymbol.OpenBrace))
            {
                ProcessCodeBlock();
            }
            else if (nextToken is KeywordToken)
            {
                _source.ReadNextToken();
                ProcessKeyword((KeywordToken)nextToken);
            }
            else if (nextToken.IsVariableType)
            {
                _source.PushLocation();
                Token variableType = _source.ReadNextToken();

                if (_source.NextIsKeyword(PredefinedSymbol.Dot))
                {
                    // it's a static member access
                    _source.PopLocation();
                    GenerateCodeForExpression(ReadExpression(true, PredefinedSymbol.Semicolon));
                }
                else
                {
                    _source.DeletePushedLocation();

                    if (!allowVariableDeclarations)
                    {
                        throw new CompilerMessage(ErrorCode.VariableDeclarationNotAllowedHere, "The variable would go out of scope as soon as it was declared");
                    }
                    do
                    {
                        DeclareLocalVariable(variableType);
                    }
                    while (_source.NextIsKeyword(PredefinedSymbol.Comma));

                    _source.ExpectKeyword(PredefinedSymbol.Semicolon);
                    _state.NextTokenModifiers.Clear();
                }
            }
            else
            {
                GenerateCodeForExpression(ReadExpression(true, PredefinedSymbol.Semicolon));
            }
        }

        private void DeclareLocalVariable(Token variableType)
        {
            _source.IgnoreAsteriskIfPresent();

            Token variableName = _source.ReadNextTokenAndThrowIfAlreadyDefined();

            CompilerUtils.SetArrayPropertiesOnTokenFromStream(_source, variableName);

            CompilerUtils.VerifyModifiersAgainstType(ModifierTargets.LocalVariable, _state.NextTokenModifiers);
            /* TODO: get this working
            ScriptVariable newVariable = ProcessVariableDeclaration(variableType, _output.GlobalData, null, _state.NextTokenModifiers);

            variableName.Define(TokenType.LocalVariable, newVariable); */

            if (_source.NextIsKeyword(PredefinedSymbol.SetEqual))
            {
                ReadExpression(false, PredefinedSymbol.Semicolon, PredefinedSymbol.Comma);
                // TODO: check types, assign result to variable
            }
        }

        private void ProcessKeyword(KeywordToken keywordToken)
        {
            PredefinedSymbol keyword = keywordToken.SymbolType;

            if (keyword == PredefinedSymbol.If)
            {
                _source.ExpectKeyword(PredefinedSymbol.OpenParenthesis);

                Expression condition = ReadExpression(true, PredefinedSymbol.CloseParenthesis);

                GenerateCodeForExpression(condition);

                ProcessCodeBlock();

                if (_source.NextIsKeyword(PredefinedSymbol.Else))
                {
                    ProcessCodeBlock();
                }
            }
            else if (keyword == PredefinedSymbol.While)
            {
                _source.ExpectKeyword(PredefinedSymbol.OpenParenthesis);

                Expression condition = ReadExpression(true, PredefinedSymbol.CloseParenthesis);

                GenerateCodeForExpression(condition);

                ProcessCodeBlock();
            }
            else if (keyword == PredefinedSymbol.For)
            {
                // TODO: Handle for loops
            }
            else if (keyword == PredefinedSymbol.Break)
            {
                // TODO: Handle break
            }
            else if (keyword == PredefinedSymbol.Continue)
            {
                // TODO: Handle continue
            }
            else if (keyword == PredefinedSymbol.Return)
            {
                Expression resultToReturn = ReadExpression(true, PredefinedSymbol.Semicolon);

                GenerateCodeForExpression(resultToReturn);

                // TODO: Generate code to return and release local vars
                FreeAllLocalVariablesButLeaveValid();
            }
            else
            {
                throw new CompilerMessage(ErrorCode.InvalidUseOfKeyword, "Keyword '" + keywordToken.Name + "' is not valid here");
            }
        }

        private void GenerateCodeForExpression(Expression expression)
        {
            Expression split = expression.SplitExpressionOnLowestPrecendenceOperator();
            if (split is SplitExpression)
            {
                SplitExpression splitExpression = (SplitExpression)split;

                if (splitExpression.Operator is OperatorToken)
                {
                    GenerateCodeForStandardOperator(splitExpression);
                }
                else
                {
                    GenerateCodeForAssignmentOperator(splitExpression);
                }

                // TODO: Check type mismatch after expressions have been parsed
                //splitExpression.CheckForTypeMismatch();
                //splitExpression.CheckOperatorCanBeAppliedToTheseTypes();
            }
            else
            {
                GenerateCodeForExpressionWithoutOperator(split);
            }
        }

        private void GenerateCodeForExpressionWithoutOperator(Expression expression)
        {
            System.Diagnostics.Trace.WriteLine(expression.ToString());

            ScriptReader reader = new ScriptReader(expression, _source.LineNumber);
            Token firstToken = reader.ReadNextToken();

            bool staticAccess = false;
            if (firstToken.IsVariableType)
            {
                staticAccess = true;
            }
            else if (reader.NextIsKeyword(PredefinedSymbol.OpenSquareBracket))
            {
                Expression arrayIndex = ReadExpression(reader, true, PredefinedSymbol.CloseSquareBracket);
                GenerateCodeForExpression(arrayIndex);
                // TODO: Array access
            }

            if (reader.NextIsKeyword(PredefinedSymbol.Dot))
            {
                if ((firstToken.Type != TokenType.LocalVariable) &&
                    (firstToken.Type != TokenType.GlobalVariable) &&
                    (firstToken.Type != TokenType.StructType))
                {
                    throw new CompilerMessage(ErrorCode.ParentIsNotAStruct, "'" + firstToken.Name + "' is not a struct");
                }
                ////Token memberName = reader.ReadNextToken();

                // TODO: struct member stuff
                if (staticAccess)
                {
                }
            }
            else if (staticAccess)
            {
                throw new CompilerMessage(ErrorCode.InvalidUseOfStruct, "Struct cannot be used in this way");
            }
            else
            {
                // TODO: just read the variable itself / execute the function
            }

            // TODO: Code this
        }

        private void GenerateCodeForAssignmentOperator(SplitExpression expression)
        {
            KeywordToken theOperator = expression.Operator as KeywordToken;
            if ((theOperator == null) || (!theOperator.IsModificationOperator))
            {
                throw new CompilerMessage(ErrorCode.InternalError, "GenerateCodeForAssignmentOperator called without KeywordToken");
            }

            if (expression.LeftHandSide.Count < 1)
            {
                throw new CompilerMessage(ErrorCode.OperatorExpectsLeftHandSide, "Expected expression before '" + expression.Operator.Name + "'");
            }

            if ((theOperator.SymbolType != PredefinedSymbol.PlusPlus) &&
                (theOperator.SymbolType != PredefinedSymbol.MinusMinus))
            {
                GenerateCodeForExpression(expression.RightHandSide);
            }

            if (theOperator.SymbolType != PredefinedSymbol.SetEqual)
            {
                GenerateCodeForExpression(expression.LeftHandSide);
            }

            // TODO: Apply operator

            //TODO: GenerateCodeToWriteNewVariableValue
        }

        private void GenerateCodeForStandardOperator(SplitExpression expression)
        {
            OperatorToken theOperator = expression.Operator as OperatorToken;
            if (theOperator == null)
            {
                throw new CompilerMessage(ErrorCode.InternalError, "ProcessStandardOperator called without OperatorToken");
            }

            if (expression.RightHandSide.Count < 1)
            {
                throw new CompilerMessage(ErrorCode.OperatorExpectsRightHandSide, "Expected expression after '" + expression.Operator.Name + "'");
            }

            if (expression.LeftHandSide.Count < 1)
            {
                if (theOperator.HasLeftHandSide == RequiredState.Required)
                {
                    throw new CompilerMessage(ErrorCode.OperatorExpectsLeftHandSide, "Expected expression before '" + expression.Operator.Name + "'");
                }
            }
            else if (theOperator.HasLeftHandSide == RequiredState.NotAllowed)
            {
                throw new CompilerMessage(ErrorCode.OperatorDoesNotExpectLeftHandSide, "Unexpected expression before '" + expression.Operator.Name + "'");
            }
            else
            {
                GenerateCodeForExpression(expression.LeftHandSide);
            }

            GenerateCodeForExpression(expression.RightHandSide);

            // TODO: Apply operator
        }

        private Expression ReadExpression(bool readTerminatingSymbolFromStream, params PredefinedSymbol[] endsWithSymbols)
        {
            return ReadExpression(_source, readTerminatingSymbolFromStream, endsWithSymbols);
        }

        private Expression ReadExpression(ScriptReader source, bool readTerminatingSymbolFromStream, params PredefinedSymbol[] endsWithSymbols)
        {
            Expression expression = new Expression();
            int bracketLevel = 0;
            while (bracketLevel >= 0)
            {
                foreach (PredefinedSymbol terminatingSymbol in endsWithSymbols)
                {
                    if ((bracketLevel == 0) && (source.NextIsKeyword(terminatingSymbol, !readTerminatingSymbolFromStream)))
                    {
                        return expression;
                    }
                }

                Token thisToken = source.PeekNextToken();
                if (thisToken is EndOfStreamToken)
                {
                    throw new CompilerMessage(ErrorCode.EndOfInputReached, "End of script reached in the middle of an expression");
                }

                CompilerUtils.AdjustBracketLevelIfTokenIsBracket(thisToken, ref bracketLevel);

                if (bracketLevel >= 0)
                {
                    expression.Add(source.ReadNextToken());
                }
            }
            throw new CompilerMessage(ErrorCode.UnexpectedToken, "Unexpected '" + source.ReadNextToken() + "'");
        }
    }
}
