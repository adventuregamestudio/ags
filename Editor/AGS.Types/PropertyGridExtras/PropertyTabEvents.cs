using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Linq;
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
            // Get common event tab properties, marked with AGSEventsTabPropertyAttribute
            PropertyDescriptorCollection eventProps =
                TypeDescriptor.GetProperties(component, new Attribute[] { new AGSEventsTabPropertyAttribute() });
            List<PropertyDescriptor> eventList = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor p in eventProps)
                eventList.Add(p);
            // Then optionally add Interaction event (generated from the InteractionSchema)
            eventList.AddRange(GetInteractionProperties(component));
            // Return combined collection to the property grid
            return new PropertyDescriptorCollection(eventList.ToArray());
        }

        private List<PropertyDescriptor> GetInteractionProperties(object component)
        {
            List<PropertyDescriptor> propList = new List<PropertyDescriptor>();
            PropertyInfo interactionsProperty = component.GetType().GetProperty("Interactions");
            if (interactionsProperty != null)
            {
                // "Interactions" property must NOT have a Obsolete attribute,
                // otherwise it's considered to be disabled.
                if (interactionsProperty.GetCustomAttribute(typeof(ObsoleteAttribute)) != null)
                    return propList;

                Interactions interactions = (Interactions)interactionsProperty.GetValue(component, null);
                string category = string.Empty;
                string functionParameters = string.Empty;
                var categoryAttr = interactionsProperty.GetCustomAttribute(typeof(CategoryAttribute));
                if (categoryAttr != null)
                    category = (categoryAttr as CategoryAttribute).Category;
                var scriptFunctionAttr = interactionsProperty.GetCustomAttribute(typeof(ScriptFunctionAttribute));
                if (scriptFunctionAttr != null)
                    functionParameters = (scriptFunctionAttr as ScriptFunctionAttribute).Parameters;
                var schema = interactions.Schema;
                var events = schema.Events;
                foreach (var evt in events)
                {
                    propList.Add(new InteractionEventPropertyDescriptor(component, evt.UID,
                            evt.EventName, evt.DisplayName, category, evt.FunctionSuffix, functionParameters));
                }
            }
            return propList;
        }
    }
}
