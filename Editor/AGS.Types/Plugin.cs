using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Plugin
    {
        private string _fileName;
        private byte[] _serializedData;

        public Plugin(string fileName, byte[] serializedData)
        {
            _fileName = fileName;
            _serializedData = serializedData;
        }

        public string FileName
        {
            get { return _fileName; }
        }

        public byte[] SerializedData
        {
            get { return _serializedData; }
        }

        public Plugin(XmlNode pluginNode)
        {
            _fileName = SerializeUtils.GetElementString(pluginNode, "FileName");
            _serializedData = Convert.FromBase64String(SerializeUtils.GetElementString(pluginNode, "Data"));
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Plugin");
            writer.WriteElementString("FileName", _fileName);
            writer.WriteStartElement("Data");
            writer.WriteBase64(_serializedData, 0, _serializedData.Length);
            writer.WriteEndElement();
            writer.WriteEndElement();
        }
    }
}
