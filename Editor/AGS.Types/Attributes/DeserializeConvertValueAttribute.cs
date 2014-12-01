using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// Defines a conversion for particular values of the property type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate,
                    AllowMultiple = true)]
    public class DeserializeConvertValueAttribute : Attribute
    {
        public DeserializeConvertValueAttribute(string convertFrom, string convertTo)
            : base()
        {
            this.ConvertFrom = convertFrom;
            this.ConvertTo   = convertTo;
        }

        public string ConvertFrom { get; private set; }
        public string ConvertTo   { get; private set; }

        public string Convert(string value)
        {
            return (ConvertFrom == value) ? ConvertTo : value;
        }
    }
}
