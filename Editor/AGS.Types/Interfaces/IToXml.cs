using System.Xml;

namespace AGS.Types
{
    public interface IToXml
    {
        void ToXml(XmlTextWriter writer);
    }
}
