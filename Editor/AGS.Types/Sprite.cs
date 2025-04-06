using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("SourceFile")]
    public class Sprite : ICustomTypeDescriptor, IComparable<Sprite>
    {
        public const string PROPERTY_SPRITE_NUMBER = "Number";
        public const string PROPERTY_RESOLUTION = "Resolution";

        private int _number;
        private int _width;
        private int _height;
        private int _colorDepth;
        private SpriteImportResolution _resolution;
        private bool _alphaChannel;
        private string _sourceFile = string.Empty;
        private int? _coloursLockedToRoom = null;
        private int _frame = 0;
        private SpriteImportTransparency _tranparentColour = SpriteImportTransparency.LeaveAsIs;
        private int _transparentColourIndex = 0;
        private int _offsetX;
        private int _offsetY;
        private int _importWidth;
        private int _importHeight;
        private bool _remapToGamePalette;
        private bool _remapToRoomPalette;
        private bool _importAlphaChannel;
        private bool _importAsTile;

        public Sprite(int number, int width, int height, int colorDepth, SpriteImportResolution importRes, bool alphaChannel)
        {
            _number = number;
            _width = width;
            _height = height;
            _colorDepth = colorDepth;
            _resolution = importRes;
            _alphaChannel = alphaChannel;
        }

		/// <summary>
		/// Basic constructor when you just want to pass simple sprite details
		/// around and use this as an entity.
		/// </summary>
		public Sprite(int number, int width, int height)
			: this(number, width, height, 0, SpriteImportResolution.Real, false)
		{
		}

        [Browsable(false)]
        [AGSNoSerialize]
        public static bool AllowRelativeResolution
        {
            get; set;
        }

        [Description("The sprite slot number for this sprite")]
        [Category("Design")]
        [ReadOnly(true)]
        [DisplayName(PROPERTY_SPRITE_NUMBER)]
        public int Number 
        { 
            get { return _number; } 
            set { _number = value; } 
        }

        [Description("[DEPRECATED] Native resolution of the sprite. It will be scaled up or down as appropriate at other resolutions.")]
        [Category("Appearance")]
        [TypeConverter(typeof(EnumTypeConverter))]
        [DisplayName(PROPERTY_RESOLUTION)]
        public SpriteImportResolution Resolution
        {
            get { return _resolution; }
            set { _resolution = value; }
        }

        [Description("The width of the sprite")]
        [ReadOnly(true)]
        [Category("Appearance")]
        public int Width
        {
            get { return _width; }
            set { _width = value; }
        }

        [Description("The height of the sprite")]
        [ReadOnly(true)]
        [Category("Appearance")]
        public int Height
        {
            get { return _height; }
            set { _height = value; }
        }

        [Description("The amount of disk space this sprite takes up, when uncompressed")]
        [Category("Design")]
        public string SizeOnDisk
        {
            get { return "" + ((_width * _height * ((_colorDepth + 1) / 8)) / 1024) + " KB"; }
        }

        [Description("The colour depth of the sprite, in bits per pixel")]
        [ReadOnly(true)]
        [Category("Appearance")]
        public int ColorDepth
        {
            get { return _colorDepth; }
            set { _colorDepth = value; }
        }

        [Description("Whether this sprite has an alpha channel")]
        [ReadOnly(true)]
        [Category("Appearance")]
        public bool AlphaChannel
        {
            get { return _alphaChannel; }
            set { _alphaChannel = value; }
        }

		[Description("The file location that this sprite was imported from")]
		[Category("Import")]
        [EditorAttribute(typeof(PropertyGridExtras.SpriteFileNameEditor), typeof(System.Drawing.Design.UITypeEditor))]
		public string SourceFile
		{
			get { return _sourceFile; }
            // CHECKME: should we allow to set null here?
			set { _sourceFile = value != null ? Utilities.ValidateFilePath(value) : null; }
		}

        [Description("Import the alpha channel (if one is available)")]
        [Category("Import")]
        public bool ImportAlphaChannel
        {
            get { return _importAlphaChannel; }
            set { _importAlphaChannel = value; }
        }

		[Browsable(false)]
		public int? ColoursLockedToRoom
		{
			get { return _coloursLockedToRoom; }
			set { _coloursLockedToRoom = value; }
		}

		[Description("The room number that this sprite's palette is locked against. It will look wrong if used in other rooms.")]
		[Category("Appearance")]
		[DisplayName("PaletteLockedToRoom")]
		public string ColoursLockedToRoomDescription
		{
			get { return (_coloursLockedToRoom.HasValue) ? _coloursLockedToRoom.Value.ToString() : "(None)"; }
		}

        [Description("The horizontal offset within the source file")]
        [Category("Import")]
        public int OffsetX
        {
            get { return _offsetX; }
            set { _offsetX = value; }
        }

        [Description("The vertical offset within the source file")]
        [Category("Import")]
        public int OffsetY
        {
            get { return _offsetY; }
            set { _offsetY = value; }
        }

        [Description("The width of the import")]
        [Category("Import")]
        public int ImportWidth
        {
            get { return _importWidth; }
            set { _importWidth = value; }
        }

        [Description("The height of the import")]
        [Category("Import")]
        public int ImportHeight
        {
            get { return _importHeight; }
            set { _importHeight = value; }
        }

        [Description("Import as a spritesheet tile using the specified size and offsets")]
        [Category("Import")]
        [RefreshProperties(RefreshProperties.All)]
        public bool ImportAsTile
        {
            get { return _importAsTile; }
            set { _importAsTile = value; }
        }

        [Description("The frame number of a multi-frame image within the source file")]
        [Category("Import")]
        public int Frame
        {
            get { return _frame; }
            set { _frame = value; }
        }

        [Description("The method used for processing transparent colours")]
        [Category("Import")]
        public SpriteImportTransparency TransparentColour
        {
            get { return _tranparentColour; }
            set { _tranparentColour = value; }
        }

        [Description("Palette index treated as a transparent colour")]
        [Category("Import")]
        public int TransparentColourIndex
        {
            get { return _transparentColourIndex; }
            set { _transparentColourIndex = value; }
        }

        [Description("Remap colours to game palette")]
        [Category("Import")]
        public bool RemapToGamePalette
        {
            get { return _remapToGamePalette; }
            set { _remapToGamePalette = value; }
        }

        [Description("Remap colours to room palette")]
        [Category("Import")]
        public bool RemapToRoomPalette
        {
            get { return _remapToRoomPalette; }
            set { _remapToRoomPalette = value; }
        }

        public Sprite(XmlNode node)
        {
            _number = Convert.ToInt32(node.Attributes["Slot"].InnerText);
            _width = Convert.ToInt32(node.Attributes["Width"].InnerText);
            _height = Convert.ToInt32(node.Attributes["Height"].InnerText);
            _colorDepth = Convert.ToInt32(node.Attributes["ColorDepth"].InnerText);
            _resolution = (SpriteImportResolution)Enum.Parse(typeof(SpriteImportResolution), node.Attributes["Resolution"].InnerText);

            if (node.Attributes["AlphaChannel"] != null)
            {
                _alphaChannel = Convert.ToBoolean(node.Attributes["AlphaChannel"].InnerText);
            }

            if (node.Attributes["ColoursLockedToRoom"] != null)
            {
                _coloursLockedToRoom = Convert.ToInt32(node.Attributes["ColoursLockedToRoom"].InnerText);
            }

            if (node.SelectSingleNode("Source") != null)
            {
                XmlNode sourceNode = node.SelectSingleNode("Source");

                try
                {
                    _sourceFile = SerializeUtils.GetElementString(sourceNode, "FileName");

                    // added in XML version 17
                    _offsetX = Convert.ToInt32(SerializeUtils.GetElementString(sourceNode, "OffsetX"));
                    _offsetY = Convert.ToInt32(SerializeUtils.GetElementString(sourceNode, "OffsetY"));
                    _frame = Convert.ToInt32(SerializeUtils.GetElementString(sourceNode, "Frame"));
                    _remapToGamePalette = Convert.ToBoolean(SerializeUtils.GetElementString(sourceNode, "RemapToGamePalette"));
                    _remapToRoomPalette = Convert.ToBoolean(SerializeUtils.GetElementString(sourceNode, "RemapToRoomPalette"));
                    _tranparentColour = (SpriteImportTransparency)Enum.Parse(typeof(SpriteImportTransparency), SerializeUtils.GetElementString(sourceNode, "ImportMethod"));
                }
                catch (InvalidDataException)
                {
                    // pass
                }

                // added with fixup task in XML version 20
                try
                {
                    _importAlphaChannel = Convert.ToBoolean(SerializeUtils.GetElementString(sourceNode, "ImportAlphaChannel"));
                }
                catch (InvalidDataException)
                {
                    _importAlphaChannel = true;
                }

                // added with fixup task in XML Version 23
                try
                {
                    _importWidth = Convert.ToInt32(SerializeUtils.GetElementString(sourceNode, "ImportWidth"));
                    _importHeight = Convert.ToInt32(SerializeUtils.GetElementString(sourceNode, "ImportHeight"));
                }
                catch (InvalidDataException)
                {
                    _importWidth = _width;
                    _importHeight = _height;
                }

                // added with fixup task in XML Version 24
                try
                {
                    _importAsTile = Convert.ToBoolean(SerializeUtils.GetElementString(sourceNode, "ImportAsTile"));
                }
                catch (InvalidDataException)
                {
                    _importAsTile = false;
                }

                // added in XML Version 3060300
                try
                {
                    _transparentColourIndex = Convert.ToInt32(SerializeUtils.GetElementString(sourceNode, "TransparentColorIndex"));
                }
                catch (InvalidDataException)
                {
                    _transparentColourIndex = 0;
                }
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Sprite");
            writer.WriteAttributeString("Slot", _number.ToString());
            writer.WriteAttributeString("Width", _width.ToString());
            writer.WriteAttributeString("Height", _height.ToString());
            writer.WriteAttributeString("ColorDepth", _colorDepth.ToString());
            writer.WriteAttributeString("Resolution", _resolution.ToString());
            writer.WriteAttributeString("AlphaChannel", _alphaChannel.ToString());

            if (_coloursLockedToRoom.HasValue)
            {
                writer.WriteAttributeString("ColoursLockedToRoom", _coloursLockedToRoom.Value.ToString());
            }

            writer.WriteStartElement("Source"); // start source
            writer.WriteElementString("FileName", _sourceFile);
            writer.WriteElementString("OffsetX", _offsetX.ToString());
            writer.WriteElementString("OffsetY", _offsetY.ToString());
            writer.WriteElementString("ImportHeight", _importHeight.ToString());
            writer.WriteElementString("ImportWidth", _importWidth.ToString());
            writer.WriteElementString("ImportAsTile", _importAsTile.ToString());
            writer.WriteElementString("Frame", _frame.ToString());
            writer.WriteElementString("RemapToGamePalette", _remapToGamePalette.ToString());
            writer.WriteElementString("RemapToRoomPalette", _remapToRoomPalette.ToString());
            writer.WriteElementString("ImportMethod", _tranparentColour.ToString());
            writer.WriteElementString("TransparentColorIndex", _transparentColourIndex.ToString());
            writer.WriteElementString("ImportAlphaChannel", _importAlphaChannel.ToString());
            writer.WriteEndElement(); // end source

            writer.WriteEndElement();
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

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return TypeDescriptor.GetEvents(this, attributes, true);
        }

        public EventDescriptorCollection GetEvents()
        {
            return TypeDescriptor.GetEvents(this, true);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            // if a re-import wouldn't be a spritesheet tile selected from the source
            // file, don't return the properties that specify the tile size and offsets
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();

            foreach (PropertyDescriptor property in properties)
            {
                switch(property.Name)
                {
                    case "ImportHeight":
                    case "ImportWidth":
                    case "OffsetX":
                    case "OffsetY":
                        if (!_importAsTile) break;
                        goto default;
                    case "Resolution":
                        if (!AllowRelativeResolution) break;
                        goto default;
                    default:
                        wantProperties.Add(property);
                        break;
                }
            }

            return new PropertyDescriptorCollection(wantProperties.ToArray());
        }

        public PropertyDescriptorCollection GetProperties()
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, true);
            return properties;
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }

        #endregion

		#region IComparable<Sprite> Members

		public int CompareTo(Sprite other)
		{
			return this.Number - other.Number;
		}

        #endregion

        public override bool Equals(object obj)
        {
            Sprite other = obj as Sprite;
            if (object.ReferenceEquals(other, null))
                return false;
            return Number == other.Number;
        }
        public override int GetHashCode()
        {
            return Number;
        }
    }
}
