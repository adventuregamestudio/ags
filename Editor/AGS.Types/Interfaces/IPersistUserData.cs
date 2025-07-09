using System;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Interface to persist user-specific data to/from disk.
    /// </summary>
    public interface IPersistUserData
    {
        void Serialize(XmlTextWriter writer);
        void DeSerialize(XmlNode node);
    }
}
