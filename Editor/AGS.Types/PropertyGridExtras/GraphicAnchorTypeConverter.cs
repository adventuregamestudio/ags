using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Drawing;
using System.Globalization;
using System.Reflection;
using System.Runtime.Remoting.Contexts;
using System.Text.RegularExpressions;

namespace AGS.Types
{
    /// <summary>
    /// Type converter for the GraphicAnchor struct.
    /// This allows to create struct instances from formatted string, and, among other things,
    /// allows to create a DefaultValue attribute for this type by passing a string literal.
    /// </summary>
    public class GraphicAnchorTypeConverter : TypeConverter
    {
        public GraphicAnchorTypeConverter()
        {
        }

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string)
            {
                string text = (value as string).Trim();
                if (text.Length == 0)
                {
                    return null;
                }

                if (culture == null)
                {
                    culture = CultureInfo.CurrentCulture;
                }

                string[] values = text.Split(culture.TextInfo.ListSeparator[0]);
                if (values.Length == 2)
                {
                    float x = 0F, y = 0F;
                    float.TryParse(values[0], out x);
                    float.TryParse(values[1], out y);
                    return new GraphicAnchor(x, y);
                }

                throw new ArgumentException();
            }

            return base.ConvertFrom(context, culture, value);
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (value is GraphicAnchor &&
                destinationType == typeof(string))
            {
                if (culture == null)
                {
                    culture = CultureInfo.CurrentCulture;
                }

                string separator = culture.TextInfo.ListSeparator;

                GraphicAnchor a = (GraphicAnchor)value;
                return $"{a.X}{separator} {a.Y}";
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override bool GetPropertiesSupported(ITypeDescriptorContext context)
        {
            return true;
        }

        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes)
        {
            return TypeDescriptor.GetProperties(typeof(GraphicAnchor), attributes);
        }

        public override bool GetCreateInstanceSupported(ITypeDescriptorContext context)
        {
            return true;
        }

        public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues)
        {
            // Depending on which property's change instigated this instance creation,
            // some fields may be valid, and others not, because their values might depend
            // on the changed property.
            GraphicAnchor oldValue = (GraphicAnchor)context.PropertyDescriptor.GetValue(context.Instance);
            FrameAlignment newType = (FrameAlignment)propertyValues["Type"];
            if (oldValue.Type != newType)
            {
                if (newType != FrameAlignment.None)
                {
                    return new GraphicAnchor(newType);
                }
            }

            return new GraphicAnchor((float)propertyValues["X"], (float)propertyValues["Y"]);
        }
    }
}
