using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    [AttributeUsage(AttributeTargets.All, AllowMultiple = true)]
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
