using System;
using System.ComponentModel;
using System.Drawing;

namespace AGS.Types
{
    public abstract class ColorSubPropertyDescriptor : PropertyDescriptor
    {
        private readonly ITypeDescriptorContext _context;

        public ColorSubPropertyDescriptor(string name, Attribute[] attrs,
                ITypeDescriptorContext context)
            : base(name, attrs)
        {
            _context = context;
        }

        public override bool IsReadOnly { get { return Attributes.Contains(ReadOnlyAttribute.Yes); } }

        public override Type ComponentType { get { return typeof(int); } }

        public override Type PropertyType { get { return typeof(int); } }

        public override bool CanResetValue(object component)
        {
            return _context.PropertyDescriptor.CanResetValue(component);
        }

        public override object GetValue(object component)
        {
            return _context.PropertyDescriptor.GetValue(_context.Instance);
        }

        public override void ResetValue(object component)
        {
            _context.PropertyDescriptor.ResetValue(_context.Instance);
        }

        public override void SetValue(object component, object value)
        {
            _context.PropertyDescriptor.SetValue(_context.Instance, value);
        }

        public override bool ShouldSerializeValue(object component)
        {
            return _context.PropertyDescriptor.ShouldSerializeValue(component);
        }
    }

    public sealed class ColorNumberDescriptor : ColorSubPropertyDescriptor
    {
        public ColorNumberDescriptor(ITypeDescriptorContext context, Attribute[] attrs)
            : base("ColorNumber", attrs, context)
        {
        }
    }

    public sealed class ColorRGBDescriptor : ColorSubPropertyDescriptor
    {
        public ColorRGBDescriptor(ITypeDescriptorContext context, Attribute[] attrs)
            : base("ColorRGB", attrs, context)
        {
        }
    }
}
