using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// InteractionEventPropertyDescriptor defines a programmatically generated property
    /// associated with a certain Interactions event element.
    /// This descriptor "tricks" its user (such as PropertyGrid) in thinking that the
    /// property belongs to another type. This type must be passed to class constructor as "component".
    /// This allows to have this event pseudo-property displayed for a type like e.g. Character,
    /// while values are read and written not in Character type directly, but in its
    /// child Interactions member.
    /// </summary>
    public class InteractionEventPropertyDescriptor : PropertyDescriptor
    {
        private Type _componentType;
        private string _eventName;

        public InteractionEventPropertyDescriptor(object component,
            string eventName, string displayName, string category, string functionSuffix, string parameterList)
            :
            base(eventName, new Attribute[]{new DisplayNameAttribute(displayName), 
                new EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor)),
                new CategoryAttribute(category),
                new DefaultValueAttribute(string.Empty),
                new ScriptFunctionAttribute(functionSuffix, parameterList)})
        {
            _componentType = component.GetType();
            _eventName = eventName;
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
            string value = null;
            interactions.ScriptFunctionNames.TryGetValue(_eventName, out value);
            return value;
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
            interactions.ScriptFunctionNames[_eventName] = value.ToString();
        }

        public override bool ShouldSerializeValue(object component)
        {
            return false;
        }
    }

}
