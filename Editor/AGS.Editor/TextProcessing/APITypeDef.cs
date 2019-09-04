using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    /// <summary>
    /// Describes script API struct type.
    /// </summary>
    public class APITypeDef : IComparable<APITypeDef>
    {
        public string Name;
        public bool Autoptr;
        public bool Managed;
        public bool String;

        public APITypeDef(string name, bool autoptr, bool managed, bool is_string)
        {
            Name = name;
            Autoptr = autoptr;
            Managed = managed;
            String = is_string;
        }

        public int CompareTo(APITypeDef other)
        {
            return Name.CompareTo(other.Name);
        }
    }
}
