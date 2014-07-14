using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class GlobalVariables
    {
        private Dictionary<string, GlobalVariable> _variables = new Dictionary<string, GlobalVariable>();

        public GlobalVariables() { }

        public GlobalVariable this[string varName]
        {
            get
            {
                if (_variables.ContainsKey(varName))
                {
                    return _variables[varName];
                }
                return null;
            }
        }

        public void Add(GlobalVariable variable)
        {
            _variables.Add(variable.Name, variable);
        }

        public void Remove(GlobalVariable variable)
        {
            _variables.Remove(variable.Name);
        }

        public void VariableRenamed(GlobalVariable variable, string oldName)
        {
            _variables.Remove(oldName);
            _variables.Add(variable.Name, variable);
        }

        public IList<GlobalVariable> ToList()
        {
            List<GlobalVariable> list = new List<GlobalVariable>();
            list.AddRange(_variables.Values);
            return list;
        }

        public GlobalVariables(XmlNode node)
        {
            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, "Variables"))
            {
                GlobalVariable newVariable = new GlobalVariable(childNode);
                _variables.Add(newVariable.Name, newVariable);
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Variables");
            foreach (GlobalVariable variable in _variables.Values)
            {
                variable.ToXml(writer);
            }
            writer.WriteEndElement();
        }
    }
}
