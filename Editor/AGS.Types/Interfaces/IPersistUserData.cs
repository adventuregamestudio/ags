using System;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Interface to persist user-specific data to/from disk. This data
    /// will not be checked into source control.
    /// </summary>
    public interface IPersistUserData
    {
        void Serialize(XmlTextWriter writer);
        void DeSerialize(XmlNode node);
    }
}
