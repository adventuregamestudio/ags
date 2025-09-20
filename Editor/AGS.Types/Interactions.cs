using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Interactions is a table of script function names mapped to the
    /// object events. The list of events is defined by InteractionSchema.
    /// While InteractionSchema is attached to the object *type*,
    /// Interactions object is attached to individual object instances.
    /// </summary>
    public class Interactions
    {
        private InteractionSchema _schema;
        private string _scriptModule = string.Empty;
        // Map event name to a script function
        private Dictionary<string, string> _scriptFunctionNames;

        public Interactions(InteractionSchema schema)
        {
            _schema = schema;
            _scriptFunctionNames = new Dictionary<string, string>();
            _schema.Changed += Schema_Changed;
        }

        private void Schema_Changed(object sender, InteractionSchemaChangedEventArgs args)
        {
            SyncWithSchema(args.EventNameRemap);
        }

        private void SyncWithSchema(Dictionary<string, string> eventNameRemap)
        {
            var oldFunctions = _scriptFunctionNames;
            _scriptFunctionNames = new Dictionary<string, string>();
            if (eventNameRemap != null)
            {
                foreach (var remap in eventNameRemap)
                {
                    if (oldFunctions.ContainsKey(remap.Key))
                    {
                        _scriptFunctionNames.Add(remap.Value, oldFunctions[remap.Key]);
                    }
                }
            }
        }

        public InteractionSchema Schema
        {
            get { return _schema; }
        }

        public Dictionary<string, string> ScriptFunctionNames
        {
            get { return _scriptFunctionNames; }
        }

        public void FromXml(XmlNode node)
        {
            _scriptFunctionNames = new Dictionary<string, string>();
            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "Interactions"))
            {
                /*
                if (child.Name != "Event")
                {
                    if (child.Name == "ScriptModule")
                        ScriptModule = child.InnerText;
                    continue;
                }
                */

                /*
                int index = SerializeUtils.GetAttributeInt(child, "Index");
                if (index < 0 || index >= _scriptFunctionNames.Length)
                    continue; // FIXME: throw? need compat handle first
                _scriptFunctionNames[index] = child.InnerText;
                if (_scriptFunctionNames[index] == string.Empty)
                {
                    _scriptFunctionNames[index] = null;
                }
                */
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Interactions");
            //writer.WriteElementString("ScriptModule", ScriptModule);
            /*
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
            */
            writer.WriteEndElement();
        }
    }
}
