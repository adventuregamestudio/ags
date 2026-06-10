using System;
using System.Drawing;

namespace AGS.Types
{
    public class FontMetrics
    {
        private static FontMetrics _empty = null;

        public int FirstCharCode { get; set; }
        public int LastCharCode { get; set; }
        public Rectangle CharBBox { get; set; }

        public static FontMetrics Empty
        {
            get
            {
                if (_empty == null)
                    _empty = new FontMetrics();
                return _empty;
            }
        }

        private FontMetrics()
        {
            FirstCharCode = -1;
            LastCharCode = -1;
            CharBBox = Rectangle.Empty;
        }

        public FontMetrics(int first_char, int last_char, Rectangle bbox)
        {
            FirstCharCode = first_char;
            LastCharCode = last_char;
            CharBBox = bbox;
        }
    }
}
