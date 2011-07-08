using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Reflection;
using System.Text;
using System.Windows.Forms.Design;

namespace AGS.Types
{
    public class PropertyTabInteractions : PropertyTab
    {
        public delegate string UpdateEventNameHandler(string eventName);
        public static UpdateEventNameHandler UpdateEventName;

        private static Bitmap _image;

        public PropertyTabInteractions()
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
            List<PropertyDescriptor> propList = new List<PropertyDescriptor>();

            PropertyInfo interactionsProperty = component.GetType().GetProperty("Interactions");
            if (interactionsProperty != null)
            {
                Interactions interactions = (Interactions)interactionsProperty.GetValue(component, null);
                for (int i = 0; i < interactions.FunctionSuffixes.Length; i++)
                {
                    string eventName = interactions.DisplayNames[i];
                    if (UpdateEventName != null)
                    {
                        eventName = UpdateEventName(eventName);
                    }
					if (eventName.IndexOf("$$") < 0)
					{
						// Only add the event if the cursor mode exists
						propList.Add(new InteractionPropertyDescriptor(component, i, interactions.FunctionSuffixes[i], eventName));
					}
                }
            }

            return new PropertyDescriptorCollection(propList.ToArray());
        }

    }
}
