using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class ScriptCompiler : IScriptCompiler
	{
		private string _scriptName;
		private CompileResults _results = new CompileResults();
		private ScriptReader _source;
        private TokenizedScript _tokenizedScript;
		private CompilerState _state = new CompilerState();
        private CompiledScript _output;

		public CompileResults CompileScript(string script)
		{
			ITokenizer tokenizer = CompilerFactory.CreateTokenizer();
            _tokenizedScript = tokenizer.TokenizeScript(script);
			_source = new ScriptReader(_tokenizedScript);
			_output = new CompiledScript();

			while (true)
			{
				Token thisToken = _source.ReadNextToken();

				if (thisToken is EndOfStreamToken)
				{
					break;
				}

				try
				{
					ProcessTokenAtTopLevel(thisToken);
				}
				catch (CompilerMessage error)
				{
					RecordError(error.Code, error.Message);
                    break;
				}
			}

			return _results;
		}

		private void ProcessTokenAtTopLevel(Token thisToken)
		{
			if (thisToken is ModifierToken)
			{
				_state.NextTokenModifiers.Add((ModifierToken)thisToken);
			}
			else if (thisToken.IsVariableType)
			{
                bool wasFunction = false;
				do
				{
					wasFunction = ParseAndAddGlobalVariableOrFunction(thisToken);
				}
				while ((!wasFunction) && (_source.NextIsKeyword(PredefinedSymbol.Comma)));

                if (!wasFunction)
                {
                    _source.ExpectKeyword(PredefinedSymbol.Semicolon);
                }
				_state.NextTokenModifiers.Clear();
			}
			else if (thisToken is KeywordToken)
			{
				PredefinedSymbol keyword = ((KeywordToken)thisToken).SymbolType;
				if (keyword == PredefinedSymbol.StructDefinition)
				{
					_output.AddStruct(ProcessStructDeclaration());
				}
				else if (keyword == PredefinedSymbol.Enum)
				{
					ProcessEnumDeclaration();
				}
                else if (keyword == PredefinedSymbol.Export)
                {
                    ProcessExportCommand();
                }
                else
                {
                    RecordError(ErrorCode.InvalidUseOfKeyword, "Invalid use of '" + thisToken.Name + "'");
                }
			}
            else if (thisToken.Name.StartsWith(Constants.NEW_SCRIPT_MARKER))
			{
                _scriptName = thisToken.Name.Substring(Constants.NEW_SCRIPT_MARKER.Length, thisToken.Name.Length - Constants.NEW_SCRIPT_MARKER.Length - 1);
			}
			else
			{
				RecordError(ErrorCode.UnexpectedToken, "Unexpected '" + thisToken.Name + "'");
			}
		}

        private void ProcessExportCommand()
        {
            do
            {
                Token variableName = _source.ReadNextAsGlobalVariable();
                if (variableName.Variable.IsImported)
                {
                    RecordError(ErrorCode.VariableAlreadyImported, "The variable '" + variableName.Name + "' has already been imported");
                }
                else
                {
                    variableName.Variable.IsExported = true;
                }
            }
            while (_source.NextIsKeyword(PredefinedSymbol.Comma));

            _source.ExpectKeyword(PredefinedSymbol.Semicolon);
        }

        private void VerifyModifiersAgainstType(ModifierTargets target)
        {
            CompilerUtils.VerifyModifiersAgainstType(target, _state.NextTokenModifiers);
        }

        private ScriptStruct ProcessStructDeclaration()
		{
			VerifyModifiersAgainstType(ModifierTargets.Struct);

            Modifiers prototypeModifiers = null;
			Token structName = _source.ReadNextToken();
            if (structName.Defined)
            {
                if ((structName.Type == TokenType.StructType) &&
                    (structName.StructType.PrototypeOnly))
                {
                    prototypeModifiers = structName.StructType.Modifiers;
                }
                else
                {
                    throw new CompilerMessage(ErrorCode.TokenAlreadyDefined, "Token '" + structName.Name + "' is already defined");
                }
            }

			ScriptStruct structDefinition = new ScriptStruct(structName.Name);
            structDefinition.Modifiers = _state.NextTokenModifiers;
			structName.Define(TokenType.StructType, structDefinition);
			structName.IsVariableType = true;

            if (_state.IsModifierPresent(PredefinedSymbol.Managed))
            {
                structDefinition.IsManaged = true;
            }

            _state.NextTokenModifiers = new Modifiers();

            if (_source.NextIsKeyword(PredefinedSymbol.Semicolon))
            {
                structDefinition.PrototypeOnly = true;
                return structDefinition;
            }

            if (prototypeModifiers != null)
            {
                if (!structDefinition.Modifiers.HasSameModifiers(prototypeModifiers))
                {
                    RecordError(ErrorCode.DifferentModifierInPrototype, "This struct has different modifiers to the prototype");
                }
            }

            if (_source.NextIsKeyword(PredefinedSymbol.Extends))
			{
				Token structToExtend = _source.ReadNextAsVariableType();
				if (structToExtend.Type != TokenType.StructType)
				{
					RecordError(ErrorCode.StructNameExpected, "Struct name expected at '" + structToExtend.Name + "'");
				}
				else
				{
					ScriptStruct baseStructDefinition = ((ScriptStruct)structToExtend.Value);
                    if (baseStructDefinition.PrototypeOnly)
                    {
                        RecordError(ErrorCode.CannotExtendPrototypeStruct, "Cannot extend struct '" + baseStructDefinition.Name + "' because it is only a prototype");
                    }
					structDefinition.Extends = baseStructDefinition;
                    structDefinition.SizeInBytes = baseStructDefinition.SizeInBytes;
					structDefinition.Members.AddRange(baseStructDefinition.Members);

                    if (!structDefinition.Modifiers.HasSameModifiers(baseStructDefinition.Modifiers))
                    {
                        RecordError(ErrorCode.DifferentModifierInPrototype, "This struct has different modifiers to the base type");
                    }
				}
			}

			_source.ExpectKeyword(PredefinedSymbol.OpenBrace);

			while (!_source.NextIsKeyword(PredefinedSymbol.CloseBrace))
			{
				Token token = _source.ReadNextToken();
				if (token is ModifierToken)
				{
					_state.NextTokenModifiers.Add((ModifierToken)token);
				}
				else if (token.IsVariableType)
				{
					do
					{
						ParseAndAddMemberToStruct(token, structName, structDefinition, structName);
					}
					while (_source.NextIsKeyword(PredefinedSymbol.Comma));

					_source.ExpectKeyword(PredefinedSymbol.Semicolon);
					_state.NextTokenModifiers.Clear();
				}
				else
				{
					RecordError(ErrorCode.UnexpectedToken, "Unexpected " + token.Name);
				}
			}

			_source.ExpectKeyword(PredefinedSymbol.Semicolon);

            return structDefinition;
		}

        private void ParseFunctionParameterList(ScriptFunction func)
        {
            Modifiers modifiers;
            bool atEndOfParameterList = false;

            while ((!atEndOfParameterList) &&
                   (!_source.NextIsKeyword(PredefinedSymbol.CloseParenthesis)))
            {
                modifiers = new Modifiers();

                while (_source.PeekNextToken() is ModifierToken)
                {
                    modifiers.Add((ModifierToken)_source.ReadNextToken());
                }

                if (_source.NextIsKeyword(PredefinedSymbol.VariableArguments))
                {
                    func.VariableArguments = true;
                    _source.ExpectKeyword(PredefinedSymbol.CloseParenthesis, "Variable arguments must be the last parameter");
                    break;
                }

                Token parameterType = _source.ReadNextAsVariableType();
                Token parameterName = null;
                _source.IgnoreAsteriskIfPresent();

                CompilerUtils.VerifyModifiersAgainstType(ModifierTargets.FunctionParameter, modifiers);

                VerifyParameterTypeValidForFunction(parameterType);

                if (!_source.PeekNextToken().Defined)
                {
                    parameterName = _source.ReadNextTokenAndThrowIfAlreadyDefined();
                }
                else
                {
                    func.IsPrototypeOnly = true;
                }

                FunctionParameter parameter = new FunctionParameter(parameterType, parameterName);
                parameter.Modifiers = modifiers;

                if (_source.NextIsKeyword(PredefinedSymbol.SetEqual))
                {
                    parameter.DefaultValue = _source.ReadNextAsConstInt();
                }
                
                if (_source.NextIsKeyword(PredefinedSymbol.CloseParenthesis))
                {
                    atEndOfParameterList = true;
                }
                else
                {
                    _source.ExpectKeyword(PredefinedSymbol.Comma);
                }

                func.Parameters.Add(parameter);
            }
        }

        private void VerifyParameterTypeValidForFunction(Token parameterType)
        {
            if (parameterType.Type == TokenType.StructType)
            {
                if (!parameterType.StructType.IsManaged)
                {
                    throw new CompilerMessage(ErrorCode.CannotPassStructToFunction, "Cannot pass a non-managed struct to a function");
                }
            }
        }

        private void VerifyReturnTypeValidForFunction(Token returnType)
        {
            if (returnType.Type == TokenType.StructType)
            {
                if (!returnType.StructType.IsManaged)
                {
                    throw new CompilerMessage(ErrorCode.CannotReturnStructFromFunction, "Cannot return a non-managed struct from a function");
                }
            }
        }

        private Token GetTokenForStructMember(Token structName, Token memberName, out string mangledName)
        {
            return CompilerUtils.GetTokenForStructMember(_tokenizedScript, structName, memberName, out mangledName);
        }

		private void ParseAndAddMemberToStruct(Token variableType, Token structName, ScriptStruct structDefinition, Token parentStruct)
		{
            _source.IgnoreAsteriskIfPresent();
			Token memberName = _source.ReadNextToken();
            if ((memberName is KeywordToken) || (memberName is OperatorToken) ||
                (memberName is ModifierToken))
            {
                throw new CompilerMessage(ErrorCode.TokenAlreadyDefined, "Cannot use '" + memberName.Name + "' as variable name since it has another meaning");
            }

            string mangledName;
            memberName = GetTokenForStructMember(structName, memberName, out mangledName);
			if (memberName != null)
			{
				throw new CompilerMessage(ErrorCode.TokenAlreadyDefined, "Member '" + mangledName + "' already exists");
			}
			memberName = new Token(mangledName, true);
			// TODO: Set necessary fields on new token for this struct member
			_tokenizedScript.AddToken(memberName);

			CompilerUtils.SetArrayPropertiesOnTokenFromStream(_source, memberName);

			if (_source.NextIsKeyword(PredefinedSymbol.OpenParenthesis))
			{
                VerifyModifiersAgainstType(ModifierTargets.MemberFunction);
                VerifyReturnTypeValidForFunction(variableType);

                ScriptFunction func = new ScriptFunction(variableType, memberName);
                ParseFunctionParameterList(func);

                func.IsPrototypeOnly = true;
            }
			else
			{
                VerifyModifiersAgainstType(ModifierTargets.MemberVariable);
                ProcessVariableDeclaration(variableType, structDefinition, parentStruct, _state.NextTokenModifiers);
			}
		}

        private FixedOffsetVariable ProcessVariableDeclaration(Token variableType, DataGroup container, Token parent, Modifiers modifiers)
        {
            FixedOffsetVariable newVariable;
            int thisSize = 0;
            if (variableType.Type == TokenType.StructType)
            {
                ScriptStruct childStruct = (ScriptStruct)variableType.Value;
                if (childStruct.IsManaged)
                {
                    thisSize = FixedOffsetVariable.POINTER_SIZE_IN_BYTES;
                    newVariable = new FixedOffsetVariable(variableType, container.SizeInBytes, true);

                    if ((childStruct != container) &&
                        (parent != null) &&
                        (childStruct.HasNonImportedMemberOfType(parent)))
                    {
                        throw new CompilerMessage(ErrorCode.CircularReference, "The type '" + childStruct.Name + "' has a reference to this struct, so you cannot also have a reference this way round");
                    }
                }
                else if (childStruct == container)
                {
                    throw new CompilerMessage(ErrorCode.StructInsideItself, "A struct cannot be contained within itself");
                }
                else
                {
                    thisSize = childStruct.SizeInBytes;
                    newVariable = new FixedOffsetVariable(variableType, container.SizeInBytes, thisSize);
                }
            }
            else if (variableType is ScalarVariableTypeToken)
            {
                thisSize = ((ScalarVariableTypeToken)variableType).SizeInBytes;
                newVariable = new FixedOffsetVariable(variableType, container.SizeInBytes, thisSize);
            }
            else if (variableType.Type == TokenType.EnumType)
            {
                thisSize = FixedOffsetVariable.ENUM_SIZE_IN_BYTES;
                newVariable = new FixedOffsetVariable(variableType, container.SizeInBytes, thisSize);
            }
            else
            {
                throw new CompilerMessage(ErrorCode.CannotUseTypeInStruct, "Cannot add variable of type '" + variableType.Name + "' to struct");
            }
            newVariable.IsAttributeProperty = modifiers.HasModifier(Modifiers.ATTRIBUTE);
            newVariable.IsImported = modifiers.HasModifier(Modifiers.IMPORT);

            if ((newVariable.IsAttributeProperty) &&
                (!newVariable.IsImported))
            {
                throw new CompilerMessage(ErrorCode.AttributesMustBeImported, "Attribute types must be imported");
            }

            if ((newVariable.IsImported) &&
                (!newVariable.IsAttributeProperty) &&
                (parent != null))
            {
                throw new CompilerMessage(ErrorCode.InvalidUseOfKeyword, "'import' is invalid in this context");
            }

            if (newVariable.IsImported)
            {
                // No memory needed for imported vars
                newVariable.Offset = 0;
                thisSize = 0;
            }

            container.Members.Add(newVariable);
            container.SizeInBytes += thisSize;
            return newVariable;
        }

        private void ProcessFunctionBody()
        {
            // TODO: Declare parameters as local vars
            new CodeBlockCompiler(_state).Process(_source);
        }

        private void ReadNameOfNewGlobalVariableOrFunction(Token variableName, out bool isMemberFunctionBody)
        {
            isMemberFunctionBody = false;

            if (variableName.Type == TokenType.StructType)
            {
                // Defining the body of a member function, eg. function Struct::function()
                _source.ExpectKeyword(PredefinedSymbol.MemberOf);

                Token memberName = _source.ReadNextToken();

                string mangledName;
                Token memberFunc = GetTokenForStructMember(variableName, memberName, out mangledName);
                if (memberFunc == null)
                {
                    throw new CompilerMessage(ErrorCode.MemberFunctionNotDefined, "Struct '" + variableName.Name + "' has no member function '" + memberName.Name + "'");
                }

                // TODO: Verify that memberFunc is a function not a variable
                // TODO: Verify that function arguments match prototype

                variableName = memberFunc;
                isMemberFunctionBody = true;
            }
            else if (variableName.Defined)
            {
                throw new CompilerMessage(ErrorCode.TokenAlreadyDefined, "Token '" + variableName.Name + "' is already defined");
            }
        }

        private void ParseAndAddFunctionDefinition(Token variableType, Token variableName, bool isMemberFunction)
        {
            if (isMemberFunction)
            {
                VerifyModifiersAgainstType(ModifierTargets.MemberFunction);
            }
            else
            {
                VerifyModifiersAgainstType(ModifierTargets.GlobalFunction);
            }
            VerifyReturnTypeValidForFunction(variableType);

            ScriptFunction func = new ScriptFunction(variableType, variableName);
            ParseFunctionParameterList(func);

            if (_source.NextIsKeyword(PredefinedSymbol.Semicolon))
            {
                func.IsPrototypeOnly = true;
            }
            else
            {
                if (func.IsPrototypeOnly == true)
                {
                    throw new CompilerMessage(ErrorCode.AnonymousParameterInFunctionBody, "One or more parameters did not have a name");
                }

                func.IsPrototypeOnly = false;
                _state.NextTokenModifiers.Clear();
                ProcessFunctionBody();
            }
        }

        private void ParseAndAddGlobalVariableDefinition(Token variableType, Token variableName, FixedOffsetVariable importedVersion)
        {
            VerifyModifiersAgainstType(ModifierTargets.GlobalVariable);
            FixedOffsetVariable newVariable = ProcessVariableDeclaration(variableType, _output.GlobalData, null, _state.NextTokenModifiers);
            variableName.Define(TokenType.GlobalVariable, newVariable);
            if (_source.NextIsKeyword(PredefinedSymbol.SetEqual))
            {
                // TODO: Check types to ensure this is valid
                newVariable.DefaultValue = _source.ReadNextAsConstInt();
            }

            if (importedVersion != null)
            {
                if (importedVersion.CompareTo(newVariable) != 0)
                {
                    throw new CompilerMessage(ErrorCode.NewDefinitionIsDifferent, "New definition of '" + variableName.Name + "' does not match previous one");
                }
            }
        }

		private bool ParseAndAddGlobalVariableOrFunction(Token variableType)
		{
            bool isMemberFunctionBody = false;

            _source.IgnoreAsteriskIfPresent();
            FixedOffsetVariable importedVersion = null;
            Token variableName = _source.ReadNextToken();

            if ((variableName.Type == TokenType.GlobalVariable) &&
                (variableName.Variable.IsImported) &&
                (!variableName.Variable.IsAccessed))
            {
                importedVersion = variableName.Variable;
            }
            else
            {
                ReadNameOfNewGlobalVariableOrFunction(variableName, out isMemberFunctionBody);
            }

			if (_source.NextIsKeyword(PredefinedSymbol.OpenParenthesis))
			{
                ParseAndAddFunctionDefinition(variableType, variableName, isMemberFunctionBody);
                return true;
			}
			else
			{
                CompilerUtils.SetArrayPropertiesOnTokenFromStream(_source, variableName);
                ParseAndAddGlobalVariableDefinition(variableType, variableName, importedVersion);
			}

            return false;
		}

		private void ProcessEnumDeclaration()
		{
			Token enumName = _source.ReadNextTokenAndThrowIfAlreadyDefined();
			ScriptEnum enumDefinition = new ScriptEnum(enumName.Name);
			enumName.Define(TokenType.EnumType, enumDefinition);
			enumName.IsVariableType = true;

			_source.ExpectKeyword(PredefinedSymbol.OpenBrace);
			int nextEnumValue = 0;

			while (!_source.NextIsKeyword(PredefinedSymbol.CloseBrace))
			{
				Token enumEntry = _source.ReadNextTokenAndThrowIfAlreadyDefined();
				KeywordToken nextToken = _source.ReadNextAsKeyword();
				if (nextToken.SymbolType == PredefinedSymbol.SetEqual)
				{
                    nextEnumValue = _source.ReadNextAsConstInt();
					nextToken = _source.ReadNextAsKeyword();
				}

				enumDefinition.Values.Add(enumEntry.Name, nextEnumValue);
				enumEntry.Define(TokenType.Constant, nextEnumValue);
				nextEnumValue++;

				if (nextToken.SymbolType == PredefinedSymbol.CloseBrace)
				{
					break;
				}
				else if (nextToken.SymbolType != PredefinedSymbol.Comma)
				{
					throw new CompilerMessage(ErrorCode.UnexpectedToken, "Expected comma at '" + nextToken.Name + "'");
				}
			}

			_source.ExpectKeyword(PredefinedSymbol.Semicolon);
		}

		private void RecordError(ErrorCode errorCode, string message)
		{
			_results.Add(new Error(errorCode, message, _source.LineNumber, _scriptName));
		}
	}
}
