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

        private int _number;
        private int _width;
        private int _height;
        private int _colorDepth;
        private bool _alphaChannel;
		private string _sourceFile = string.Empty;
		private int? _coloursLockedToRoom = null;

        public Sprite(int number, int width, int height, int colorDepth, bool alphaChannel)
        {
            _number = number;
            _width = width;
            _height = height;
            _colorDepth = colorDepth;
            _alphaChannel = alphaChannel;
        }

		/// <summary>
		/// Basic constructor when you just want to pass simple sprite details
		/// around and use this as an entity.
		/// </summary>
		public Sprite(int number, int width, int height)
			: this(number, width, height, 0, false)
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
		[ReadOnly(true)]
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
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Sprite");
            writer.WriteAttributeString("Slot", _number.ToString());
            writer.WriteAttributeString("Width", _width.ToString());
            writer.WriteAttributeString("Height", _height.ToString());
            writer.WriteAttributeString("ColorDepth", _colorDepth.ToString());
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
