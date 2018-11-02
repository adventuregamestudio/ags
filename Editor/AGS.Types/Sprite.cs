using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("Resolution")]
    public class Sprite : IComparable<Sprite>
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
        private int _frame = 1;
        private SpriteImportTransparency _importMethod = SpriteImportTransparency.LeaveAsIs;
        private int _offsetX;
        private int _offsetY;
        private bool _remapToGamePalette;

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
			: this(number, width, height, 0, SpriteImportResolution.LowRes, false)
		{
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

        [Description("Native resolution of the sprite. It will be scaled up or down as appropriate at other resolutions.")]
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

        [Description("The amount of disk space this sprite takes up")]
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
		[Category("Design")]
		public string SourceFile
		{
			get { return _sourceFile; }
			set { _sourceFile = value; }
		}

		[Browsable(false)]
		public int? ColoursLockedToRoom
		{
			get { return _coloursLockedToRoom; }
			set { _coloursLockedToRoom = value; }
		}

		[Description("The room number that this sprite's palette is locked against. It will look wrong if used in other rooms.")]
		[Category("Design")]
		[DisplayName("PaletteLockedToRoom")]
		public string ColoursLockedToRoomDescription
		{
			get { return (_coloursLockedToRoom.HasValue) ? _coloursLockedToRoom.Value.ToString() : "(None)"; }
		}

        [Description("The horizontal offset within the source file")]
        [Category("Design")]
        public int OffsetX
        {
            get { return _offsetX; }
            set { _offsetX = value; }
        }

        [Description("The vertical offset within the source file")]
        [Category("Design")]
        public int OffsetY
        {
            get { return _offsetY; }
            set { _offsetY = value; }
        }

        [Description("The frame number of a multi-frame image within the source file")]
        [Category("Design")]
        public int Frame
        {
            get { return _frame; }
            set { _frame = value; }
        }

        [Description("The import method used for processing transparent colours")]
        [Category("Design")]
        public SpriteImportTransparency ImportMethod
        {
            get { return _importMethod; }
            set { _importMethod = value; }
        }

        [Description("Remap palette colours on import")]
        [Category("Design")]
        public bool RemapToGamePalette
        {
            get { return _remapToGamePalette; }
            set { _remapToGamePalette = value; }
        }

        public Sprite(XmlNode node)
        {
            _number = Convert.ToInt32(node.Attributes["Slot"].InnerText);
            _width = Convert.ToInt32(node.Attributes["Width"].InnerText);
            _height = Convert.ToInt32(node.Attributes["Height"].InnerText);
            _colorDepth = Convert.ToInt32(node.Attributes["ColorDepth"].InnerText);
            if (node.Attributes["AlphaChannel"] != null)
            {
                _alphaChannel = Convert.ToBoolean(node.Attributes["AlphaChannel"].InnerText);
            }
			if (node.SelectSingleNode("Source") != null)
			{
				XmlNode sourceNode = node.SelectSingleNode("Source");
				_sourceFile = SerializeUtils.GetElementString(sourceNode, "FileName");
			}
			if (node.Attributes["ColoursLockedToRoom"] != null)
			{
				_coloursLockedToRoom = Convert.ToInt32(node.Attributes["ColoursLockedToRoom"].InnerText);
			}
			_resolution = (SpriteImportResolution)Enum.Parse(typeof(SpriteImportResolution), node.Attributes["Resolution"].InnerText);
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
			if (!string.IsNullOrEmpty(_sourceFile))
			{
				writer.WriteStartElement("Source");
				writer.WriteElementString("FileName", _sourceFile);
				writer.WriteEndElement();
			}
			writer.WriteEndElement();
        }

		#region IComparable<Sprite> Members

		public int CompareTo(Sprite other)
		{
			return this.Number - other.Number;
		}

		#endregion
	}
}
