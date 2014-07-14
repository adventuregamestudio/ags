using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class Macros
    {
        private List<Macro> _macros = new List<Macro>();
        private Dictionary<string, Macro> _macrosKeyed = new Dictionary<string, Macro>();

        public void Add(Macro newMacro)
        {
            _macrosKeyed.Add(newMacro.Name, newMacro);
            _macros.Add(newMacro);
        }

        public bool Contains(string macroName)
        {
            return _macrosKeyed.ContainsKey(macroName);
        }

        public void Remove(string macroName)
        {
            Macro toRemove = _macrosKeyed[macroName];
            _macros.Remove(toRemove);
            _macrosKeyed.Remove(macroName);
        }

        public string this[string macroName]
        {
            get { return _macrosKeyed[macroName].Value; }
        }
    }
}
