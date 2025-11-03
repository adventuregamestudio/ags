using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("Image")]
    public class ViewFrame
    {
        private int _id;
        private int _image = 0;
        private SpriteFlipStyle _flip = SpriteFlipStyle.None;
        private int _sound = AudioClip.FixedIndexNoValue;
        private int _speed = 0;
        private int _xoff = 0;
        private int _yoff = 0;
        private string _eventName = string.Empty;

        public ViewFrame()
        {
        }

		public ViewFrame(int id)
		{
			_id = id;
		}

        [Description("The ID number of the frame")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The sprite to be displayed in this frame")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set { _image = Math.Max(0, value); }
        }

        [Description("Whether the sprite should be flipped")]
        [Category("Appearance")]
        public SpriteFlipStyle Flip
        {
            get { return _flip; }
            set { _flip = value; }
        }

        [Description("This sprite's X offset relative to the object's position")]
        [Category("Appearance")]
        public int XOffset
        {
            get { return _xoff; }
            set { _xoff = value; }
        }

        [Description("This sprite's Y offset relative to the object's position")]
        [Category("Appearance")]
        public int YOffset
        {
            get { return _yoff; }
            set { _yoff = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public bool Flipped
        {
            get; set;
        }

        [Description("The delay after this before the next frame is displayed, relative to the overall animation speed")]
        [Category("Appearance")]
        public int Delay
        {
            get { return _speed; }
            set { _speed = value; }
        }

        [Description("Sound to be played when the frame becomes visible")]
        [Category("Design")]
        [TypeConverter(typeof(AudioClipTypeConverter))]
        [DefaultValue(AudioClip.FixedIndexNoValue)]
        public int Sound
        {
            get { return _sound; }
            set { _sound = value; }
        }

        [Category("Design")]
        public string EventName
        {
            get { return _eventName; }
            set { _eventName = value; }
        }

        public ViewFrame(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public ViewFrame Clone()
        {
            return (ViewFrame)MemberwiseClone();
        }
        /// <summary>
        /// Copies properties to another ViewFrame, *except* for an ID
        /// </summary>
        public void CopyTo(ViewFrame other)
        {
            other.Image = Image;
            other.Flipped = Flipped;
            other.Delay = Delay;
            other.Sound = Sound;
        }
    }
}
