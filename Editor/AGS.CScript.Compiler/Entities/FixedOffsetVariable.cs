using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    /// <summary>
    /// Represents a non-local variable (ie. a global var or struct member)
    /// </summary>
	internal class FixedOffsetVariable : ScriptVariable, IComparable<FixedOffsetVariable>
	{
        public bool IsAttributeProperty = false;
        public bool IsImported = false;
        public bool IsExported = false;
        public object DefaultValue = null;

		public FixedOffsetVariable(Token token, int offset, int size)
		{
			VariableTypeToken = token;
			Offset = offset;
			Size = size;
			IsPointer = false;
		}

        public FixedOffsetVariable(Token token, int offset, bool isPointer)
		{
			VariableTypeToken = token;
			Offset = offset;
			IsPointer = isPointer;

			if (isPointer)
			{
				Size = POINTER_SIZE_IN_BYTES;
			}
		}

        public int CompareTo(FixedOffsetVariable other)
        {
            return ((other.VariableTypeToken == this.VariableTypeToken) &&
                (other.IsAttributeProperty == this.IsAttributeProperty) &&
                (other.IsPointer == this.IsPointer) &&
                (other.Size == this.Size)) ? 0 : 1;
        }
    }
}
