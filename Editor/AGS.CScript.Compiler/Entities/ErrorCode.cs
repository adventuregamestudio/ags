using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    public enum ErrorCode
    {
        MacroNameMissing = 1,
        MacroDoesNotExist,
        MacroAlreadyExists,
        MacroNameInvalid,
        LineTooLong,
        UnknownPreprocessorDirective,
        UserDefinedError,
        EndIfWithoutIf,
        IfWithoutEndIf,
        InvalidVersionNumber,
        UnexpectedToken = 100,
        InvalidUseOfKeyword,
        TokenAlreadyDefined,
        VariableTypeExpected,
        GlobalVariableExpected,
        StructNameExpected,
        CircularReference,
        StructInsideItself,
        CannotUseTypeInStruct,
        ArraySizeNotInteger,
        InternalError,
        CannotReturnStructFromFunction,
        CannotPassStructToFunction,
        ConstIntExpected,
        AnonymousParameterInFunctionBody,
        DifferentModifierInPrototype,
        CannotExtendPrototypeStruct,
        EndOfInputReached,
        OperatorExpectsRightHandSide,
        OperatorExpectsLeftHandSide,
        OperatorDoesNotExpectLeftHandSide,
        OperatorDoesNotExpectRightHandSide,
        AttributesMustBeImported,
        VariableAlreadyImported,
        NewDefinitionIsDifferent,
        VariableDeclarationNotAllowedHere,
        MemberFunctionNotDefined,
        ParentIsNotAStruct,
        InvalidUseOfStruct
    }
}
