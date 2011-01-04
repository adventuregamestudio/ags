using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class Modifiers : List<ModifierToken>
    {
        public const string CONST = "const";
        public const string ATTRIBUTE = "attribute";
        public const string IMPORT = "import";

        public bool HasModifier(string name)
        {
            foreach (ModifierToken modifier in this)
            {
                if (modifier.Name == name)
                {
                    return true;
                }
            }
            return false;
        }

        public bool HasSameModifiers(Modifiers other)
        {
            foreach (ModifierToken modifier in this)
            {
                if (!other.Contains(modifier))
                {
                    return false;
                }
            }

            foreach (ModifierToken modifier in other)
            {
                if (!this.Contains(modifier))
                {
                    return false;
                }
            }

            return true;
        }
    }
}
