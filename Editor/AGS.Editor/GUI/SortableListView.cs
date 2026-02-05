using System;
using System.Collections;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// SortableListView implements standard sorting of items using subitem
    /// texts when the corresponding column is clicked.
    /// 
    /// TODO: indicate currently selected column & sort order (asc/dsc).
    /// </summary>
    public class SortableListView : ListView
    {
        private int _selectedColumn = -1;
        private bool _selectedColumnAscending = false;

        private class ItemComparer : IComparer
        {
            public ItemComparer(int column, bool ascending)
            {
                _subItem = column;
                _sortFactor = ascending ? 1 : -1;
            }

            public int Compare(object x, object y)
            {
                return (x as ListViewItem).SubItems[_subItem].Text.CompareTo((y as ListViewItem).SubItems[_subItem].Text)
                    * _sortFactor;
            }

            private int _subItem;
            private int _sortFactor;
        }

        public SortableListView()
            : base()
        {
            ApplySort(0, true);
        }

        public void ApplySort(int column, bool ascending)
        {
            ListViewItemSorter = new ItemComparer(column, ascending);
            _selectedColumn = column;
            _selectedColumnAscending = ascending;
            Sort();

            if (Created)
            {
                this.SetSortIcon(_selectedColumn, _selectedColumnAscending ? SortOrder.Ascending : SortOrder.Descending);
            }
        }

        protected override void OnCreateControl()
        {
            base.OnCreateControl();

            this.SetSortIcon(_selectedColumn, _selectedColumnAscending ? SortOrder.Ascending : SortOrder.Descending);
        }

        protected override void OnColumnClick(ColumnClickEventArgs e)
        {
            ApplySort(e.Column, e.Column != _selectedColumn || !_selectedColumnAscending);

            base.OnColumnClick(e);
        }
    }
}
