using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("OutlineStyle")]
    public class Font
    {
        private int _id;
        private string _name;
        private int _pointSize;
        private int _outlineFont;
        private FontOutlineStyle _outlineStyle;
		private string _sourceFilename = string.Empty;

        public Font()
        {
            _outlineStyle = FontOutlineStyle.None;
            _outlineFont = 0;
            _name = string.Empty;
            _pointSize = 0;
        }

        [Description("The ID number of the font")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Browsable(false)]
        public int PointSize
        {
            get { return _pointSize; }
            set { _pointSize = value; }
        }

        [Description("The point size that this TTF font was imported at")]
        [Category("Appearance")]
        [DisplayName("Point size")]
        public string PointSizeDescription
        {
            get { return (_pointSize < 1) ? "N/A" : "" + _pointSize + " pt"; }
        }

        [Description("The name of the font")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

		[Description("The name with which the script will access this font")]
		[Category("Design")]
		public string ScriptID
		{
			get
			{
				if (Name.Length < 1)
				{
					return string.Empty;
				}
				string scriptName = "eFont" + Name;
				for (int i = 0; i < scriptName.Length; i++)
				{
					if (!Char.IsLetterOrDigit(scriptName[i]))
					{
						scriptName = scriptName.Replace(scriptName[i].ToString(), string.Empty);
					}
				}
				return scriptName;
			}
		}

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "Font: " + this.Name; }
        }

        [Description("Font to use as an outline for this one (only if FontOutlineStyle is OutlineFont)")]
        [Category("Appearance")]
        public int OutlineFont
        {
            get { return _outlineFont; }
            set { _outlineFont = value; }
        }

        [Description("Whether this font should be drawn with an outline")]
        [Category("Appearance")]
        public FontOutlineStyle OutlineStyle
        {
            get { return _outlineStyle; }
            set { _outlineStyle = value; }
        }

		[Description("The file path that this font was imported from")]
		[Category("Design")]
		[ReadOnly(true)]
		public string SourceFilename
		{
			get { return _sourceFilename; }
			set { _sourceFilename = value; }
		}

		[Browsable(false)]
		public string WFNFileName
		{
			get { return "agsfnt" + _id + ".wfn"; }
		}

		[Browsable(false)]
		public string TTFFileName
		{
			get { return "agsfnt" + _id + ".ttf"; }
		}

        public Font(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

    }
}
