using System;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    public class Interactions : ICloneable
    {
        [NonSerialized]
        private InteractionSchema _schema;
        private string _scriptModule = string.Empty;
        private string[] _scriptFunctionNames;
        private string[] _importedScripts;

        public Interactions(InteractionSchema schema)
        {
            _schema = schema;
            _scriptModule = schema.DefaultScriptModule;
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

        public InteractionSchema Schema
        {
            get { return _schema; }
            // Schema setter is required for random data deserialization,
            // such as when reading an object from clipboard.
            set
            {
                _schema = value;
                if (_scriptFunctionNames.Length != _schema.EventNames.Length)
                {
                    var newFnNames = new string[_schema.EventNames.Length];
                    var newImpScripts = new string[_schema.EventNames.Length];
                    Array.Copy(_scriptFunctionNames, newFnNames, Math.Min(_scriptFunctionNames.Length, _schema.EventNames.Length));
                    Array.Copy(_importedScripts, newImpScripts, Math.Min(_importedScripts.Length, _schema.EventNames.Length));
                }
            }
        }

        //[Category("(Basic)")]
        //[DefaultValue(Script.GLOBAL_SCRIPT_FILE_NAME)]
        //[TypeConverter(typeof(ScriptListTypeConverter))]
        public string ScriptModule
        {
            get { return _scriptModule; }
            set { _scriptModule = value; }
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

        public string[] FunctionParameterLists
        {
            get { return _schema.FunctionParameterLists; }
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
                if (child.Name != "Event")
                {
                    if (child.Name == "ScriptModule")
                        ScriptModule = child.InnerText;
                    continue;
                } 

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
            writer.WriteElementString("ScriptModule", ScriptModule);
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

        #region IClonable Members

        public object Clone()
        {
            Interactions copy = new Interactions(_schema);
            copy._scriptModule = this._scriptModule;
            copy._scriptFunctionNames = this._scriptFunctionNames.Clone() as string[];
            copy._importedScripts = this._importedScripts.Clone() as string[];
            return copy;
        }

        #endregion
    }
}
