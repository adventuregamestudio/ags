using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("HandleImage")]
    public class GUISlider : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "Slider";
        public const string SCRIPT_CLASS_TYPE = "Slider";

        public GUISlider(int x, int y, int width, int height)
            : base(x, y, width, height)
        {
            _min = 0;
            _max = 10;
            _value = 0;
        }

        public GUISlider(XmlNode node) : base(node)
        {
        }

        public GUISlider() { }

        private int _min;
        private int _max;
        private int _value;
        private int _handleImage;
        private int _handleOffset;
        private int _backgroundImage;
        private string _changeEventHandler = string.Empty;

        [Description("Script function to run when the slider is moved")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventProperty()]
        [ScriptFunctionParameters("GUIControl *control")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnChange
        {
            get { return _changeEventHandler; }
            set
            {
                if (value.Length > MAX_EVENT_HANDLER_LENGTH)
                {
                    _changeEventHandler = value.Substring(0, MAX_EVENT_HANDLER_LENGTH);
                }
                else
                {
                    _changeEventHandler = value;
                }
            }
        }

        [Description("Image to use for the grabbable handle on the slider")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int HandleImage
        {
            get { return _handleImage; }
            set { _handleImage = value; }
        }

        [Description("Pixel offset to draw the handle image at")]
        [Category("Appearance")]
        public int HandleOffset
        {
            get { return _handleOffset; }
            set { _handleOffset = value; }
        }

        [Description("Tile this image as the background for the slider")]
        [Category("Appearance")]
		[EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
		public int BackgroundImage
        {
            get { return _backgroundImage; }
            set { _backgroundImage = value; }
        }

        [Description("Minimum value that the slider can represent")]
        [Category("Setup")]
        public int MinValue
        {
            get { return _min; }
            set
            {
                _min = value;
                if (value > _max)
                {
                    _max = _min;
                }
            }
        }

        [Description("Maximum value that the slider can represent")]
        [Category("Setup")]
        public int MaxValue
        {
            get { return _max; }
            set
            {
                _max = value;
                if (value < _min)
                {
                    _min = _max;
                }
            }
        }

        [Description("Current value that the slider is showing")]
        [Category("Setup")]
        public int Value
        {
            get { return _value; }
            set 
            {
                _value = value;
                if (_value < _min)
                {
                    _value = _min;
                }
                if (_value > _max)
                {
                    _value = _max;
                }
            }
        }

        public override string ControlType
        {
            get
            {
                return CONTROL_DISPLAY_NAME;
            }
        }

        public override string ScriptClassType
        {
            get
            {
                return SCRIPT_CLASS_TYPE;
            }
        }

        protected override void GetSpritesForControl(List<int> list)
        {
            if (_handleImage > 0) list.Add(_handleImage);
            if (_backgroundImage > 0) list.Add(_backgroundImage);
        }

        public override void UpdateSpritesWithMapping(Dictionary<int, int> spriteMapping)
        {
            if (_handleImage > 0) _handleImage = spriteMapping[_handleImage];
            if (_backgroundImage > 0) _backgroundImage = spriteMapping[_backgroundImage];
        }
    }
}