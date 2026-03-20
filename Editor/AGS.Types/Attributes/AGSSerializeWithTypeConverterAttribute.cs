using System;

namespace AGS.Types
{
    /// <summary>
    /// Marks the property for serialization mechanism,
    /// tells that this property should be serialized using its Type's
    /// TypeConverter.
    /// This is essentially a temporary hack, because we cannot use
    /// any property's TypeConverter freely at the moment, because of
    /// how our serialization is implemented.
    /// </summary>
    public class AGSSerializeWithTypeConverterAttribute : Attribute
    {
    }
}
