using System;
using System.ComponentModel;
using System.Xml;

namespace AGS.Types
{
    public class FontFile
    {
        private string _filename = string.Empty;
        private string _sourceFilename = string.Empty;
        private FontFileFormat _fileFormat = FontFileFormat.Unknown;
        private string _familyName = string.Empty;

        public FontFile()
        {
        }

        public FontFile(string fileName)
        {
            _filename = fileName;
        }

        [Description("The font's filename in the project")]
        [Category("Design")]
        [ReadOnly(true)]
        public string FileName
        {
            get { return _filename; }
            set { _filename = value; }
        }

        [Description("The file path that this font was imported from")]
        [Category("Design")]
        [ReadOnly(true)]
        public string SourceFilename
        {
            get { return _sourceFilename; }
            set { _sourceFilename = value; }
        }

        [AGSNoSerialize]
        [Description("This font's format type")]
        [Category("Design")]
        [ReadOnly(true)]
        public FontFileFormat FileFormat
        {
            get { return _fileFormat; }
            set { _fileFormat = value; }
        }

        [AGSNoSerialize]
        [Description("The name of a font's family, if available")]
        [Category("Design")]
        [ReadOnly(true)]
        public string FamilyName
        {
            get { return _familyName; }
            set { _familyName = value; }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "FontFile: " + this.FileName; }
        }

        public FontFile(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }
    }
}
