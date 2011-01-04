using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    public class PaletteEntry
    {
        public const string PROPERTY_COLOR_TYPE = "ColourType";
        public const string PROPERTY_COLOR_RGB = "Colour";

        private Color _color;
        private PaletteColourType _type;
        private int _index;

        public PaletteEntry(int index, Color color)
        {
            _color = color;
            _type = PaletteColourType.Background;
            _index = index;
        }

        [Description("The colour represented by this palette index")]
        [Category("Appearance")]
        [DisplayName(PROPERTY_COLOR_RGB)]
        public Color Colour
        {
            get { return _color; }
            set { _color = value; }
        }

        [Description("Whether this colour is game-wide, or dependant on the room background")]
        [Category("Appearance")]
        [DisplayName(PROPERTY_COLOR_TYPE)]
        public PaletteColourType ColourType
        {
            get { return _type; }
            set { _type = value; }
        }

        [Description("The colour number of this colour")]
        [Category("Design")]
        public int Index
        {
            get { return _index; }
        }
    }
}
