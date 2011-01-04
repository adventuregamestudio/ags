using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Base class for items which are maintained in a folder structure,
    /// like Views and AudioClips.
    /// </summary>
    /// <typeparam name="T">type of the item, eg. View</typeparam>
    /// <typeparam name="U">type of the folder, eg. ViewFolder</typeparam>
    public abstract class BaseFolderCollection<T,U> : IToXml where T : IToXml where U : IToXml
    {
        protected abstract U CreateFolder(XmlNode node);
        protected abstract T CreateItem(XmlNode node);
        protected virtual void ToXmlExtend(XmlTextWriter writer) { }

        private string _name;
        protected List<U> _subFolders;
        protected List<T> _items;

        public BaseFolderCollection(string name)
        {
            _name = name;
            _items = new List<T>();
            _subFolders = new List<U>();
        }

        [AGSNoSerialize]
        public string Name
        {
            get { return _name; }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("Name");
                }
                _name = value;
            }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public IList<U> SubFolders
        {
            get { return _subFolders; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public IList<T> Items
        {
            get { return _items; }
        }

        private string XmlFolderNodeName
        { 
            get { return typeof(U).Name; } 
        }

        private string XmlItemListNodeName
        {
            get { return typeof(T).Name + "s"; }
        }

        public BaseFolderCollection(XmlNode node)
        {
            if (node.Name != this.XmlFolderNodeName)
            {
                throw new InvalidDataException("Incorrect node passed to " + this.XmlFolderNodeName);
            }
            _name = node.Attributes["Name"].InnerText;
            _items = new List<T>();
            _subFolders = new List<U>();

            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, "SubFolders"))
            {
                _subFolders.Add(CreateFolder(childNode));
            }

            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, this.XmlItemListNodeName))
            {
                _items.Add(CreateItem(childNode));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement(this.XmlFolderNodeName);
            writer.WriteAttributeString("Name", _name);

            writer.WriteStartElement("SubFolders");
            foreach (U folder in _subFolders)
            {
                folder.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteStartElement(this.XmlItemListNodeName);
            foreach (T view in _items)
            {
                view.ToXml(writer);
            }
            writer.WriteEndElement();

            ToXmlExtend(writer);

            writer.WriteEndElement();
        }
    }
}
