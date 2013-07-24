using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;

namespace AGS.Types
{
    /// <summary>
    /// An implementation of Binding List that also fires a removing event before an item is removed.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class BindingListWithRemoving<T> : BindingList<T>
    {
        public event FolderChangeEventHandler<T> BeforeChanging;
        public event EventHandler BeforeClearing;        

        protected void OnChange(FolderChangeEventArgs<T> e)
        {
            if (BeforeChanging != null)
            {
                BeforeChanging(this, e);
            }
        }

        protected override void RemoveItem(int index)
        {
            if (index > -1 && index < this.Count)
            {
                OnChange(new FolderChangeEventArgs<T>(this[index], FolderChange.ItemRemoved));
                base.RemoveItem(index);
            }
        }

        protected override void InsertItem(int index, T item)
        {
            OnChange(new FolderChangeEventArgs<T>(item, FolderChange.ItemAdded));
            base.InsertItem(index, item);
        }

        protected override void ClearItems()
        {
            if (BeforeClearing != null)
            {
                BeforeClearing(this, null);
            }
            base.ClearItems();
        }
    }
}
