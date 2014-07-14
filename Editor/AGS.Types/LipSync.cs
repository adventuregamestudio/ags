using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class LipSync
    {
        public const int MAX_LIP_SYNC_FRAMES = 20;

        private LipSyncType _type;
        private int _defaultFrame;
        private string[] _lipSyncCharsForFrame = new string[MAX_LIP_SYNC_FRAMES];

        public LipSync()
        {
            for (int i = 0; i < _lipSyncCharsForFrame.Length; i++)
            {
                _lipSyncCharsForFrame[i] = string.Empty;
            }
            // Set up some defaults
            _lipSyncCharsForFrame[0] = "A/I";
            _lipSyncCharsForFrame[1] = "E";
            _lipSyncCharsForFrame[2] = "O";
            _lipSyncCharsForFrame[3] = "U";
            _lipSyncCharsForFrame[4] = "M/B/P";
            _lipSyncCharsForFrame[5] = "C/D/G/K/N/R/S/Th/Y/Z";
            _lipSyncCharsForFrame[6] = "L";
            _lipSyncCharsForFrame[7] = "F/V";
            _lipSyncCharsForFrame[8] = "W/Q";
        }

        [Description("Is lip-sync enabled in this game?")]
        [Category("Design")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public LipSyncType Type
        {
            get { return _type; }
            set { _type = value; }
        }

        [Description("The default frame to use for letters that are not listed")]
        [Category("Design")]
        public int DefaultFrame
        {
            get { return _defaultFrame; }
            set { _defaultFrame = value; }
        }

        [Browsable(false)]
        public string[] CharactersPerFrame
        {
            get { return _lipSyncCharsForFrame; }
        }

        public void FromXml(XmlNode node)
        {
            XmlNode lipSyncNode = node.SelectSingleNode("LipSync");
            this.Type = (LipSyncType)Enum.Parse(typeof(LipSyncType), SerializeUtils.GetElementString(lipSyncNode, "Type"));
            this.DefaultFrame = Convert.ToInt32(SerializeUtils.GetElementString(lipSyncNode, "DefaultFrame"));
            int nodeIndex = 0;
            foreach (XmlNode frameNode in SerializeUtils.GetChildNodes(lipSyncNode, "Frames"))
            {
                _lipSyncCharsForFrame[nodeIndex] = frameNode.InnerText;
                nodeIndex++;
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("LipSync");
            writer.WriteElementString("Type", this.Type.ToString());
            writer.WriteElementString("DefaultFrame", this.DefaultFrame.ToString());
            writer.WriteStartElement("Frames");
            foreach (string lipSyncForFrame in _lipSyncCharsForFrame)
            {
                writer.WriteElementString("CharsForFrame", lipSyncForFrame);
            }
            writer.WriteEndElement();
            writer.WriteEndElement();
        }

    }
}
