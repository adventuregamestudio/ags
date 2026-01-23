using System;

namespace AGS.Types
{
    [AttributeUsage(AttributeTargets.Property)]
    public class SerializeAsHex : Attribute
    {
        public SerializeAsHex() : base()
        {
        }
    }
}