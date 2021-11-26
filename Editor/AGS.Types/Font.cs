using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("OutlineStyle")]
    public class Font : ICustomTypeDescriptor
	{
        private int _id;
        private string _name;
        private string _familyName;
        private int _pointSize;
        private int _fontHeight;
        private int _outlineFont;
        private FontOutlineStyle _outlineStyle;
		private string _sourceFilename = string.Empty;
        private int _sizeMultiplier = 1;
        private int _verticalOffset;
        private int _lineSpacing;
        private int _autoOutlineThickness = 1;
        private FontAutoOutlineStyle _autoOutlineStyle = FontAutoOutlineStyle.Squared;
        private FontMetricsFixup _ttfMetricsFixup = FontMetricsFixup.None;

        public Font()
        {
            _name = string.Empty;
            _pointSize = 0;
            _outlineFont = 0;
            _outlineStyle = FontOutlineStyle.None;
            _fontHeight = 0;
            _lineSpacing = 0;
        }

        [Description("The ID number of the font")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
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

        [AGSNoSerialize]
        [Description("The actual height of a font, in pixels")]
        [Category("Appearance")]
        [DisplayName("Font Height")]
        [ReadOnly(true)]
        public int Height
        {
            get { return _fontHeight; }
            set { _fontHeight = value; }
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

        [Description("Font to use as an outline for this one")]
        [Category("Appearance")]
        public int OutlineFont
        {
            get { return _outlineFont; }
            set { _outlineFont = value; }
        }

        [Description("Whether this font should be drawn with an outline")]
        [Category("Appearance")]
        [RefreshProperties(RefreshProperties.All)]
        public FontOutlineStyle OutlineStyle
        {
            get { return _outlineStyle; }
            set { _outlineStyle = value; }
        }

        [Description("Thickness of the automatic outline. WARNING: automatic outlines are generated at runtime and large sizes may negatively affect your game performance.")]
        [Category("Appearance")]
        [DefaultValue(1)]
        public int AutoOutlineThickness
        {
            get { return _autoOutlineThickness; }
            set
            {
                if (value < 1)
                    throw new ArgumentException("AutoOutlineThickness must be 1 or greater.");
                _autoOutlineThickness = value;
            }
        }

        [Description("Style of the automatic outline")]
        [Category("Appearance")]
        [DefaultValue(FontAutoOutlineStyle.Squared)]
        public FontAutoOutlineStyle AutoOutlineStyle
        {
            get { return _autoOutlineStyle; }
            set { _autoOutlineStyle = value; }
        }

		[Description("The file path that this font was imported from")]
		[Category("Design")]
		[ReadOnly(true)]
		public string SourceFilename
		{
			get { return _sourceFilename; }
			set { _sourceFilename = value; }
		}

        [Description("Font's size multiplier; primarily for bitmap fonts that don't scale on their own")]
        [Category("Appearance")]
        [DefaultValue(1)]
        public int SizeMultiplier
        {
            get { return _sizeMultiplier; }
            set
            {
                if (value < 1)
                    throw new ArgumentException("ScalingMultiplier must be 1 or greater.");
                _sizeMultiplier = value;
            }
        }

        [DisplayName("TTF font adjustment")]
        [Description("Automatic adjustment of the true-type font metrics; primarily for backward compatibility.")]
        [DefaultValue(FontMetricsFixup.None)]
        [Category("Appearance")]
        [TypeConverter(typeof(EnumTypeConverter))]
        public FontMetricsFixup TTFMetricsFixup
        {
            get { return _ttfMetricsFixup; }
            set { _ttfMetricsFixup = value; }
        }

        [Description("Vertical offset to render font letters at, in pixels (can be negative)")]
        [Category("Appearance")]
        public int VerticalOffset
        {
            get { return _verticalOffset; }
            set { _verticalOffset = value; }
        }

        [Description("Default step between successive lines of text, in pixels. Setting it lower than font's height will make lines partially overlap. Put 0 to use default spacing (usually - font height).")]
        [Category("Appearance")]
        public int LineSpacing
        {
            get { return _lineSpacing; }
            set { _lineSpacing = value; }
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

        #region ICustomTypeDescriptor Members
        public AttributeCollection GetAttributes()
        {
            return TypeDescriptor.GetAttributes(this, true);
        }

        public string GetClassName()
        {
            return TypeDescriptor.GetClassName(this, true);
        }

        public string GetComponentName()
        {
            return TypeDescriptor.GetComponentName(this, true);
        }

        public TypeConverter GetConverter()
        {
            return TypeDescriptor.GetConverter(this, true);
        }

        public EventDescriptor GetDefaultEvent()
        {
            return TypeDescriptor.GetDefaultEvent(this, true);
        }

        public PropertyDescriptor GetDefaultProperty()
        {
            return TypeDescriptor.GetDefaultProperty(this, true);
        }

        public object GetEditor(Type editorBaseType)
        {
            return TypeDescriptor.GetEditor(this, editorBaseType, true);
        }

        public EventDescriptorCollection GetEvents()
        {
            return TypeDescriptor.GetEvents(this, true);
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return TypeDescriptor.GetEvents(this, attributes, true);
        }

        public PropertyDescriptorCollection GetProperties()
        {
            return TypeDescriptor.GetProperties(this, true);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantedProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                if (property.Name == "AutoOutlineStyle" ||
                    property.Name == "AutoOutlineThickness")
                {
                    if (_outlineStyle != FontOutlineStyle.Automatic)
                        continue;
                }
                else if (property.Name == "OutlineFont")
                {
                    if (_outlineStyle != FontOutlineStyle.UseOutlineFont)
                        continue;
                }

                wantedProperties.Add(property);
            }
            return new PropertyDescriptorCollection(wantedProperties.ToArray());
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }
        #endregion
	}
}
