using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class CompilerState
	{
		private Modifiers _nextTokenModifiers = new Modifiers();
        private Stack<LocalScope> _scope = new Stack<LocalScope>();

		public Modifiers NextTokenModifiers
		{
			get { return _nextTokenModifiers; }
            set { _nextTokenModifiers = value; }
		}

        public LocalScope CurrentScope
        {
            get { return _scope.Peek(); }
        }

        public LocalScope[] AllScopes
        {
            get { return _scope.ToArray(); }
        }

        public void StartNewScope()
        {
            _scope.Push(new LocalScope());
        }

        public void EndScope()
        {
            _scope.Pop();
        }

		public bool IsModifierPresent(PredefinedSymbol symbol)
		{
			foreach (ModifierToken token in _nextTokenModifiers)
			{
				if (token.Symbol == symbol)
				{
					return true;
				}
			}
			return false;
		}
	}
}
