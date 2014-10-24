using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// DeserializeIgnore attribute is applied to serializeable class to make
    /// it ignore non-existing property name found in the loaded XML.
    /// Use this attribute when you removed an obsolete property from the class
    /// to prevent errors on older project import.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public class DeserializeIgnoreAttribute : Attribute
    {
        public DeserializeIgnoreAttribute(string propertyName)
            : base()
        {
            this.PropertyName = propertyName;
        }

        public string PropertyName { get; private set; }

        public static Predicate<DeserializeIgnoreAttribute> MatchesProperty(PropertyInfo prop)
        {
            return delegate(DeserializeIgnoreAttribute attribute)
            {
                return attribute.PropertyName == prop.Name;
            };
        }

        public static Predicate<DeserializeIgnoreAttribute> MatchesPropertyName(string propertyName)
        {
            return delegate(DeserializeIgnoreAttribute attribute)
            {
                return attribute.PropertyName == propertyName;
            };
        }
    }
}
