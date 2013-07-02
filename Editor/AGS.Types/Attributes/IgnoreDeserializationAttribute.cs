using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace AGS.Types
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
    public class IgnoreDeserializationAttribute : Attribute
    {
        public IgnoreDeserializationAttribute(string propertyName)
            : base()
        {
            this.PropertyName = propertyName;
        }

        public string PropertyName { get; private set; }

        public static Predicate<IgnoreDeserializationAttribute> MatchesProperty(PropertyInfo prop)
        {
            return delegate(IgnoreDeserializationAttribute attribute)
            {
                return attribute.PropertyName == prop.Name;
            };
        }

        public static Predicate<IgnoreDeserializationAttribute> MatchesPropertyName(string propertyName)
        {
            return delegate(IgnoreDeserializationAttribute attribute)
            {
                return attribute.PropertyName == propertyName;
            };
        }
    }
}
