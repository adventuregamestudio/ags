using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.IO;

namespace AGS.Types
{
    public class ScriptAndHeader : IToXml
    {
        private Script _script, _header;
        private const string SCRIPT_AND_HEADER_NODE = "ScriptAndHeader";
        private const string SCRIPT_NODE = "ScriptAndHeader_Script";
        private const string HEADER_NODE = "ScriptAndHeader_Header";

        public ScriptAndHeader(XmlNode node)
        {                        
            _header = new Script(node.SelectSingleNode(HEADER_NODE).FirstChild);
            _script = new Script(node.SelectSingleNode(SCRIPT_NODE).FirstChild);                       
        }

        public ScriptAndHeader(Script header, Script script)
        {
            _header = header;
            _script = script;
        }

        public string Name { get { return Path.GetFileNameWithoutExtension(_script.FileName); } }

        public Script Header { get { return _header; } }

        public Script Script { get { return _script; } }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement(SCRIPT_AND_HEADER_NODE);
            writer.WriteStartElement(HEADER_NODE);
            _header.ToXml(writer);
            writer.WriteEndElement();
            writer.WriteStartElement(SCRIPT_NODE);
            _script.ToXml(writer);
            writer.WriteEndElement();
            writer.WriteEndElement();
        }

        public override bool Equals(object obj)
        {
            ScriptAndHeader scriptAndHeader = (obj as ScriptAndHeader);
            if (scriptAndHeader == null) return false;
            return (object.Equals(Header, scriptAndHeader.Header) && object.Equals(Script, scriptAndHeader.Script));
        }

        public override int GetHashCode()
        {
            int hash = 13;
            if (Header != null) hash = (hash * 7) + Header.GetHashCode();
            if (Script != null) hash = (hash * 7) + Script.GetHashCode();
            return hash;
        }
    }
}
