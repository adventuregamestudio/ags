using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class Token
    {
        public const int ARRAY_SIZE_DYNAMIC = -1;

        protected string _name;
        private bool _defined = false;
        private bool _isVariableType = false;
        private bool _isArray = false;
        private int _arraySize = ARRAY_SIZE_DYNAMIC;
        private TokenType _type = TokenType.Unknown;
        private object _value = null;

        public Token(string name, bool defined, bool isVariableType)
        {
            _name = name;
            _defined = defined;
            _isVariableType = isVariableType;
        }

        public Token(string name, bool defined) : this(name, defined, false)
        {
        }

        public Token(string name) : this(name, false, false)
        {
        }

        public Token Clone()
        {
            Token copy = new Token(_name);
            copy._defined = this._defined;
            copy._isVariableType = this._isVariableType;
            copy._isArray = this._isArray;
            copy._arraySize = this._arraySize;
            copy._type = this._type;
            copy._value = this._value;
            return copy;
        }

        public void Define(TokenType type, object value)
        {
            _type = type;
            _value = value;
            _defined = true;
        }

        public string Name
        {
            get { return _name; }
        }

        public bool Defined
        {
            get { return _defined; }
            set { _defined = value; }
        }

        public TokenType Type
        {
            get { return _type; }
            set { _type = value; }
        }

        public bool IsArray
        {
            get { return _isArray; }
            set { _isArray = value; }
        }

        public int ArraySize
        {
            get { return _arraySize; }
            set { _arraySize = value; }
        }

        public object Value
        {
            get { return _value; }
            set { _value = value; }
        }

        public ScriptStruct StructType
        {
            get
            {
                if (_type == TokenType.StructType)
                {
                    return (ScriptStruct)_value;
                }
                throw new InternalError("Token is not a struct");
            }
        }

        public FixedOffsetVariable Variable
        {
            get
            {
                if (_type == TokenType.GlobalVariable)
                {
                    return (FixedOffsetVariable)_value;
                }
                throw new InternalError("Token is not a variable");
            }
        }

        public bool IsVariableType
        {
            get { return _isVariableType; }
            set { _isVariableType = value; }
        }

        public override string ToString()
        {
            return "TOKEN: " + _name;
        }
    }
}
