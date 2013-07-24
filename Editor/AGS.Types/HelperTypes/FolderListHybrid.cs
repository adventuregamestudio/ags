using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// This is a class that can be viewed both as a list, and as a folder collection.
    /// </summary>
    public class FolderListHybrid<TFolderItem, TFolder> : IList<TFolderItem>
        where TFolderItem : IToXml
        where TFolder : BaseFolderCollection<TFolderItem, TFolder>
    {
        private IList<TFolderItem> _items;
        private TFolder _folder;

        public FolderListHybrid(TFolder folder)
        {
            _folder = folder;
            List<TFolderItem> items = new List<TFolderItem>();
            foreach (TFolderItem item in _folder.AllItemsFlat)
            {
                items.Add(item);
            }
            if (items.Count > 0 && items[0] is IComparable<TFolderItem>)
            {
                items.Sort();
            }
            _items = items;
            RegisterFolderChange();
        }

        public TFolder RootFolder { get { return _folder; } }

        public void ToXml(XmlTextWriter writer)
        {
            RootFolder.ToXml(writer);
        }

        private void RegisterFolderChange()
        {
            _folder.OnFolderChange += _folder_OnFolderChange;
        }

        private void UnregisterFolderChange()
        {
            _folder.OnFolderChange -= _folder_OnFolderChange;
        }

        void _folder_OnFolderChange(object sender, FolderChangeEventArgs<TFolderItem> e)
        {
            switch (e.FolderChange)
            {
                case FolderChange.ItemAdded:
                    _items.Add(e.FolderItem);
                    break;
                case FolderChange.ItemRemoved:
                    _items.Remove(e.FolderItem);
                    break;
                default:
                    throw new NotSupportedException(string.Format("{0} is not supported!", e.FolderChange));
            }
        }
        
        #region IList<TFolderItem> Members

        int IList<TFolderItem>.IndexOf(TFolderItem item)
        {
            return _items.IndexOf(item);
        }

        void IList<TFolderItem>.Insert(int index, TFolderItem item)
        {
            _items.Insert(index, item);
            UnregisterFolderChange();
            _folder.Items.Add(item);
            RegisterFolderChange();
        }
        
        void IList<TFolderItem>.RemoveAt(int index)
        {
            TFolderItem item = _items[index];
            _items.RemoveAt(index);
            UnregisterFolderChange();
            _folder.Items.Remove(item);
            RegisterFolderChange();
        }

        TFolderItem IList<TFolderItem>.this[int index]
        {
            get
            {
                return _items[index];
            }
            set
            {
                TFolderItem item = _items[index];
                _items[index] = value;
                UnregisterFolderChange();
                _folder.Remove(item);
                _folder.Items.Add(value);
                RegisterFolderChange();
            }
        }

        #endregion

        #region ICollection<TFolderItem> Members

        void ICollection<TFolderItem>.Add(TFolderItem item)
        {
            _items.Add(item);
            UnregisterFolderChange();
            _folder.Items.Add(item);
            RegisterFolderChange();
        }

        void ICollection<TFolderItem>.Clear()
        {
            _items.Clear();
            UnregisterFolderChange();
            _folder.Clear();
            RegisterFolderChange();
        }

        bool ICollection<TFolderItem>.Contains(TFolderItem item)
        {
            return _items.Contains(item);
        }

        void ICollection<TFolderItem>.CopyTo(TFolderItem[] array, int arrayIndex)
        {
            _items.CopyTo(array, arrayIndex);
        }

        int ICollection<TFolderItem>.Count
        {
            get { return _items.Count; }
        }

        bool ICollection<TFolderItem>.IsReadOnly
        {
            get { return _items.IsReadOnly; }
        }

        bool ICollection<TFolderItem>.Remove(TFolderItem item)
        {
            if (_items.Remove(item))
            {
                UnregisterFolderChange();
                _folder.Items.Remove(item);
                RegisterFolderChange();
                return true;
            }
            return false;
        }

        #endregion

        #region IEnumerable<TFolderItem> Members

        IEnumerator<TFolderItem> IEnumerable<TFolderItem>.GetEnumerator()
        {
            return _items.GetEnumerator();
        }

        #endregion

        #region IEnumerable Members

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return _items.GetEnumerator();
        }

        #endregion
    }
}
