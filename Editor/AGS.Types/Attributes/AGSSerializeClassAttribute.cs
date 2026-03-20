using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Marks the property for serialization mechanism,
    /// tells that this property should be serialized using its Type's
    /// IToXml interface, and constructor accepting XmlNode.
    /// </summary>
    public class AGSSerializeClassAttribute : Attribute
    {
        public AGSSerializeClassAttribute()
            : base()
        {
        }
    }
}
