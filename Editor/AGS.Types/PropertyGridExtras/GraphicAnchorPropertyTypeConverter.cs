using System;
using System.Collections;
using System.ComponentModel;
using System.Globalization;

namespace AGS.Types
{
    /// <summary>
    /// A specialized type converter for displaying GraphicAnchor property of another class.
    /// Enforces a readonly, one direction conversion from object to string, and prints
    /// extra info into the displayed string.
    /// </summary>
    public class GraphicAnchorPropertyTypeConverter : GraphicAnchorTypeConverter
    {

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return false;
            }
            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (value is GraphicAnchor &&
                destinationType == typeof(string))
            {
                GraphicAnchor a = (GraphicAnchor)value;
                return $"{a.X}, {a.Y}; ({a.Type})";
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
