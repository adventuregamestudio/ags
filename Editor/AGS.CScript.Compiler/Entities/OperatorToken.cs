using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class OperatorToken : Token
	{
		private int _precedence;
		private Opcodes _correspondingOpcode;
        private RequiredState _hasLeftHandSide;

        public OperatorToken(string name, int precedence, Opcodes correspondingOpcode, RequiredState hasLeftHandSide)
			: base(name, true)
		{
			_precedence = precedence;
			_correspondingOpcode = correspondingOpcode;
            _hasLeftHandSide = hasLeftHandSide;
		}

        public OperatorToken(string name, int precedence, Opcodes correspondingOpcode)
            : this(name, precedence, correspondingOpcode, RequiredState.Required)
        { }

		public OperatorToken(string name) : base(name)
		{
			_precedence = -1;
			_correspondingOpcode = Opcodes.None;
		}

        public int Precedence
        {
            get { return _precedence; }
        }

        public RequiredState HasLeftHandSide
        {
            get { return _hasLeftHandSide; }
        }
	}
}
