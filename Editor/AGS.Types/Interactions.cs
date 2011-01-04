using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Interactions
    {
        private InteractionSchema _schema;
        private string[] _scriptFunctionNames;
        private string[] _importedScripts;

        public Interactions(InteractionSchema schema)
        {
            _schema = schema;
            _scriptFunctionNames = new string[schema.EventNames.Length];
            _importedScripts = new string[schema.EventNames.Length];
            Reset();
        }

        public string GetScriptFunctionNameForInteractionSuffix(string suffix)
        {
            for (int i = 0; i < _schema.FunctionSuffixes.Length; i++)
            {
                if (_schema.FunctionSuffixes[i] == suffix)
                {
                    return _scriptFunctionNames[i];
                }
            }
            return null;
        }

        public void SetScriptFunctionNameForInteractionSuffix(string suffix, string scriptFunctionName)
        {
            for (int i = 0; i < _schema.FunctionSuffixes.Length; i++)
            {
                if (_schema.FunctionSuffixes[i] == suffix)
                {
                    _scriptFunctionNames[i] = scriptFunctionName;
                    break;
                }
            }
        }

        private void Reset()
        {
            for (int i = 0; i < _scriptFunctionNames.Length; i++)
            {
                _scriptFunctionNames[i] = null;
                _importedScripts[i] = null;
            }
        }

        public string[] ScriptFunctionNames
        {
            get { return _scriptFunctionNames; }
        }

        public string[] ImportedScripts
        {
            get { return _importedScripts; }
        }

        public string[] FunctionSuffixes
        {
            get { return _schema.FunctionSuffixes; }
        }

        public string[] DisplayNames
        {
            get { return _schema.EventNames; }
        }

        public void FromXml(XmlNode node)
        {
            Reset();
            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "Interactions"))
            {
                int index = SerializeUtils.GetAttributeInt(child, "Index");
                _scriptFunctionNames[index] = child.InnerText;
                if (_scriptFunctionNames[index] == string.Empty)
                {
                    _scriptFunctionNames[index] = null;
                }
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Interactions");
            for (int i = 0; i < _scriptFunctionNames.Length; i++)
            {
                writer.WriteStartElement("Event");
                writer.WriteAttributeString("Index", i.ToString());
                if (_scriptFunctionNames[i] != null)
                {
                    writer.WriteString(_scriptFunctionNames[i]);
                }
                writer.WriteEndElement();
            }
            writer.WriteEndElement();
        }

    }
}
