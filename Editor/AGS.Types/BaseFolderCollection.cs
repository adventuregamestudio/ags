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
        public event FolderChangeEventHandler<TFolderItem> OnFolderChange;
        public abstract TFolder CreateChildFolder(string name);
        protected abstract TFolder CreateFolder(XmlNode node);
        protected abstract TFolderItem CreateItem(XmlNode node);
        protected virtual void ToXmlExtend(XmlTextWriter writer) { }

        protected delegate bool EqualsDelegate<TId>(TFolderItem folderItem, TId id);        

        private string _name;
        protected BindingListWithRemoving<TFolder> _subFolders;
        protected BindingListWithRemoving<TFolderItem> _items;

        private readonly string _xmlItemListNodeName = typeof(TFolderItem).Name + "s";

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

        [AGSNoSerialize]
        [Browsable(false)]
        public bool ShouldSkipChangeNotifications { get; set; }

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

        public bool Remove(TFolderItem item)
        {
            if (_items.Remove(item)) return true;

            foreach (TFolder subFolder in this.SubFolders)
            {
                if (subFolder.Remove(item)) return true;
            }

            return false;
        }

        private string XmlFolderNodeName
        { 
            get { return typeof(TFolder).Name; } 
        }
        
        private string XmlItemListNodeName
        {
            get { return OverrideXmlItemListNodeName() ?? _xmlItemListNodeName; }
        }

        protected virtual string OverrideXmlItemListNodeName()
        {
            return null;
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
            _items.BeforeChanging += _items_BeforeChanging;
            _items.BeforeClearing += _items_BeforeClearing;
            _subFolders.BeforeChanging += _subFolders_BeforeChanging;
            _subFolders.BeforeClearing += _subFolders_BeforeClearing;
        }

        private void FireFolderChange(object sender, FolderChangeEventArgs<TFolderItem> e)
        {
            if (ShouldSkipChangeNotifications) return;

            if (OnFolderChange != null)
            {
                OnFolderChange(sender, e);
            }
        }

        void folder_OnFolderChange(object sender, FolderChangeEventArgs<TFolderItem> e)
        {
            FireFolderChange(sender, e);
        }

        void _subFolders_BeforeClearing(object sender, EventArgs e)
        {
            foreach (TFolder folder in _subFolders)
            {
                folder.BeforeRemoving();
            }
        }

        void _subFolders_BeforeChanging(object sender, FolderChangeEventArgs<TFolder> e)
        {
            TFolder folder = e.FolderItem;
            switch (e.FolderChange)
            {
                case FolderChange.ItemRemoved:                    
                    folder.BeforeRemoving();
                    folder.OnFolderChange -= folder_OnFolderChange;
                    break;
                case FolderChange.ItemAdded:                    
                    folder.OnFolderChange += folder_OnFolderChange;
                    break;
            }
        }

        private void BeforeRemoving()
        {
            _items_BeforeClearing(this, null);

            _items.BeforeChanging -= _items_BeforeChanging;
            _items.BeforeClearing -= _items_BeforeClearing;
            _subFolders.BeforeChanging -= _subFolders_BeforeChanging;
            _subFolders.BeforeClearing -= _subFolders_BeforeClearing;
        }

        private void _items_BeforeClearing(object sender, EventArgs e)
        {
            foreach (TFolderItem folderItem in _items)
            {
                FireFolderChange(this, new FolderChangeEventArgs<TFolderItem>(folderItem,
                    FolderChange.ItemRemoved));                    
            }
        }

        private void _items_BeforeChanging(object sender, FolderChangeEventArgs<TFolderItem> e)
        {
            FireFolderChange(this, e);            
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
            _items = new BindingListWithRemoving<TFolderItem>();
            _subFolders = new BindingListWithRemoving<TFolder>();
        }

        public bool MoveFolderUp(TFolder folder)
        {
            bool skipNotification = ShouldSkipChangeNotifications;
            ShouldSkipChangeNotifications = true;
            int folderIndex = _subFolders.IndexOf(folder);
            if (folderIndex == -1)
            {
                foreach (TFolder subFolder in _subFolders)
                {
                    if (subFolder.MoveFolderUp(folder))
                    {
                        ShouldSkipChangeNotifications = skipNotification;
                        return true;
                    }
                }
                ShouldSkipChangeNotifications = skipNotification;
                return false;
            }
            if (folderIndex == 0)
            {
                ShouldSkipChangeNotifications = skipNotification;
                return true;
            }
            _subFolders.RemoveAt(folderIndex);
            _subFolders.Insert(folderIndex - 1, folder);
            ShouldSkipChangeNotifications = skipNotification;
            return true;
        }

        public bool MoveFolderDown(TFolder folder)
        {
            bool skipNotification = ShouldSkipChangeNotifications;
            ShouldSkipChangeNotifications = true;
            int folderIndex = _subFolders.IndexOf(folder);
            if (folderIndex == -1)
            {
                foreach (TFolder subFolder in _subFolders)
                {
                    if (subFolder.MoveFolderDown(folder))
                    {
                        ShouldSkipChangeNotifications = skipNotification;
                        return true;
                    }
                }
                ShouldSkipChangeNotifications = skipNotification;
                return false;
            }
            if (folderIndex == _subFolders.Count - 1)
            {
                ShouldSkipChangeNotifications = skipNotification;
                return true;
            }
            _subFolders.RemoveAt(folderIndex);
            _subFolders.Insert(folderIndex + 1, folder);
            ShouldSkipChangeNotifications = skipNotification;
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
            List<TFolderItem> tmpItems = new List<TFolderItem>(_items.Count); //Hack because binding list doesn't have Sort
            foreach (TFolderItem item in _items)
            {
                tmpItems.Add(item);
            }
            tmpItems.Sort();
            bool raiseEvents = _items.RaiseListChangedEvents;
            _items.RaiseListChangedEvents = false;
            _items.Clear();
            foreach (TFolderItem item in tmpItems)
            {
                _items.Add(item);
            }
            _items.RaiseListChangedEvents = raiseEvents;

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
