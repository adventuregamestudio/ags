using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;
using System.Windows.Forms;

namespace AGS.Types
{
    /// <summary>
    /// Base class for items which are maintained in a folder structure,
    /// like Views and AudioClips.
    /// </summary>
    /// <typeparam name="TFolderItem">type of the item, eg. View</typeparam>
    /// <typeparam name="TFolder">type of the folder, eg. ViewFolder</typeparam>
    public abstract class BaseFolderCollection<TFolderItem, TFolder> : IToXml
        where TFolderItem : IToXml
        where TFolder : BaseFolderCollection<TFolderItem, TFolder>
    {
        public abstract TFolder CreateChildFolder(string name);
        protected abstract TFolder CreateFolder(XmlNode node);
        protected abstract TFolderItem CreateItem(XmlNode node);
        protected virtual void ToXmlExtend(XmlTextWriter writer) { }

        protected delegate bool EqualsDelegate<TId>(TFolderItem folderItem, TId id);        

        private string _name;
        protected List<TFolder> _subFolders;
        protected List<TFolderItem> _items;

        public BaseFolderCollection(string name)
        {
            Init(name);            
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
        public IList<TFolder> SubFolders
        {
            get { return _subFolders; }
        }

        [AGSNoSerialize]
        [Browsable(false)]
        public IList<TFolderItem> Items
        {
            get { return _items; }
        }

        protected virtual string RootNodeName { get { return null; } }

        public IEnumerable<TFolderItem> AllItemsFlat
        {
            get
            {
                foreach (TFolder subFolder in this.SubFolders)
                {
                    foreach (TFolderItem item in subFolder.AllItemsFlat)
                    {
                        yield return item;
                    }
                }
 
                foreach (TFolderItem item in _items)
                {
                    yield return item;
                }               
            }
        }

        private string XmlFolderNodeName
        { 
            get { return typeof(TFolder).Name; } 
        }

        private string XmlItemListNodeName
        {
            get { return typeof(TFolderItem).Name + "s"; }
        }

        public BaseFolderCollection(XmlNode node)
        {
            if (node.Name != this.XmlFolderNodeName)
            {
                throw new InvalidDataException("Incorrect node passed to " + this.XmlFolderNodeName);
            }
            FromXml(node);
        }

        public BaseFolderCollection(XmlNode node, XmlNode parentNodeForBackwardsCompatability)
        {
            if (node == null || node.Name != this.XmlFolderNodeName)
            {
                FromXmlBackwardsCompatability(parentNodeForBackwardsCompatability);                
            }
            else FromXml(node);
        }

        protected virtual void FromXmlBackwardsCompatability(XmlNode parentNodeForBackwardsCompatability)
        {
            //Either override this method in your Folder class to support backwards compatability,
            //or use the constructor with the backwards support.
            throw new InvalidDataException("Backwards compatabilty not supported on this type!");
        }

        protected void Init(string name)
        {
            _name = name;
            Clear();
        }

        private void FromXml(XmlNode node)
        {
            Init(node.Attributes["Name"].InnerText);            

            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, "SubFolders"))
            {
                _subFolders.Add(CreateFolder(childNode));
            }

            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, this.XmlItemListNodeName))
            {
                _items.Add(CreateItem(childNode));
            }
        }

        public void Clear()
        {
            _items = new List<TFolderItem>();
            _subFolders = new List<TFolder>();
        }

        public bool MoveFolderUp(TFolder folder)
        {
            int folderIndex = _subFolders.IndexOf(folder);
            if (folderIndex == -1)
            {
                foreach (TFolder subFolder in _subFolders)
                {
                    if (subFolder.MoveFolderUp(folder)) return true;
                }
                return false;
            }
            if (folderIndex == 0) return true;
            _subFolders.RemoveAt(folderIndex);
            _subFolders.Insert(folderIndex - 1, folder);
            return true;
        }

        public bool MoveFolderDown(TFolder folder)
        {
            int folderIndex = _subFolders.IndexOf(folder);
            if (folderIndex == -1)
            {
                foreach (TFolder subFolder in _subFolders)
                {
                    if (subFolder.MoveFolderDown(folder)) return true;
                }
                return false;
            }
            if (folderIndex == _subFolders.Count - 1) return true;
            _subFolders.RemoveAt(folderIndex);
            _subFolders.Insert(folderIndex + 1, folder);
            return true;
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement(this.XmlFolderNodeName);
            writer.WriteAttributeString("Name", _name);

            writer.WriteStartElement("SubFolders");
            foreach (TFolder folder in _subFolders)
            {
                folder.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteStartElement(this.XmlItemListNodeName);
            foreach (TFolderItem view in _items)
            {
                view.ToXml(writer);
            }
            writer.WriteEndElement();

            ToXmlExtend(writer);

            writer.WriteEndElement();
        }

        public int GetAllItemsCount()
        {
            int count = _items.Count;
            foreach (TFolder subFolder in _subFolders)
            {
                count += subFolder.GetAllItemsCount();
            }
            return count;
        }

        public void Sort(bool recursive)
        {
            _items.Sort();
            if (recursive)
            {
                foreach (TFolder subFolder in _subFolders)
                {
                    subFolder.Sort(recursive); 
                }
            }
        }
        
        protected TFolderItem FindItem<TId>(EqualsDelegate<TId> isItem, TId id, bool recursive)
        {
            foreach (TFolderItem item in _items)
            {
                if (isItem(item, id))                
                {
                    return item;
                }
            }

            if (recursive)
            {
                foreach (TFolder subFolder in this.SubFolders)
                {
                    TFolderItem found = subFolder.FindItem<TId>(isItem, id, recursive);
                    if (found != null)
                    {
                        return found;
                    }
                }
            }
            return default(TFolderItem);
        }

        public void RunActionOnAllFolderItems(Action<TFolderItem> action)
        {
            foreach (TFolderItem item in AllItemsFlat)            
            {
                action(item);
            }            
        }
    }
}
