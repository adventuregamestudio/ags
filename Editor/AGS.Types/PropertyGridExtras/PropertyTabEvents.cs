using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Reflection;
using System.Text;
using System.Windows.Forms.Design;

namespace AGS.Types
{
    public class PropertyTabEvents : PropertyTab
    {
        private static Bitmap _image;

        public PropertyTabEvents()
        {
            if (_image == null)
            {
                _image = new Bitmap(this.GetType(), "PropertyGridExtras.event.bmp");
            }
        }

        public override string TabName
        {
            get
            {
                return "Events";
            }
        }

        public override Bitmap Bitmap
        {
            get
            {
                return _image;
            }
        }

        public override PropertyDescriptorCollection GetProperties(object component, Attribute[] attrs)
        {
            return GetProperties(null, component, attrs);
        }

        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object component, Attribute[] attrs)
        {
            return TypeDescriptor.GetProperties(component, new Attribute[] { new AGSEventPropertyAttribute() });
        }

    }
}
