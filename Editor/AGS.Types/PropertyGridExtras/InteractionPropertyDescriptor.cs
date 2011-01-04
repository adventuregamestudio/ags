using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using System.Text;

namespace AGS.Types
{
    public class InteractionPropertyDescriptor : PropertyDescriptor
    {
        private Type _componentType;
        private int _eventIndex;

        public InteractionPropertyDescriptor(object component, int eventIndex, string eventName, string displayName)
            :
            base(eventName, new Attribute[]{new DisplayNameAttribute(displayName), 
                new EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor)),
                new CategoryAttribute("Events"),
                new DefaultValueAttribute(string.Empty),
                new ScriptFunctionParametersAttribute(string.Empty)})
        {
            _componentType = component.GetType();
            _eventIndex = eventIndex;
        }

        public override bool CanResetValue(object component)
        {
            return false;
        }

        public override Type ComponentType
        {
            get { return _componentType; }
        }

        public override object GetValue(object component)
        {
            PropertyInfo interactionsProperty = component.GetType().GetProperty("Interactions");
            Interactions interactions = (Interactions)interactionsProperty.GetValue(component, null);
            return interactions.ScriptFunctionNames[_eventIndex];
        }

        public override bool IsReadOnly
        {
            get { return false; }
        }

        public override Type PropertyType
        {
            get { return typeof(string); }
        }

        public override void ResetValue(object component)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override void SetValue(object component, object value)
        {
            PropertyInfo interactionsProperty = component.GetType().GetProperty("Interactions");
            Interactions interactions = (Interactions)interactionsProperty.GetValue(component, null);
            interactions.ScriptFunctionNames[_eventIndex] = value.ToString();
        }

        public override bool ShouldSerializeValue(object component)
        {
            return false;
        }
    }

}
