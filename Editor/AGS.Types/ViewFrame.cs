using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class ViewFrame
    {
        private int _id;
        private int _image = 0;
        private bool _flipped = false;
        private int _sound = 0;
        private int _speed = 0;

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
            set { _image = value; }
        }

        [Description("Whether the sprite should be flipped left-to-right")]
        [Category("Appearance")]
        public bool Flipped
        {
            get { return _flipped; }
            set { _flipped = value; }
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
        public int Sound
        {
            get { return _sound; }
            set { _sound = value; }
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
            return Clone(false);
        }

        public ViewFrame Clone(bool flip)
        {
            return new ViewFrame
            {
                ID = ID,
                Image = Image,
                Flipped = flip ? !Flipped : Flipped,
                Delay = Delay,
                Sound = Sound
            };
        }

    }
}
