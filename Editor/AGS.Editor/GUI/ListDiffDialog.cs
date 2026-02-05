using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ListDiffDialog : Form
    {
        private List<DiffListItem> OriginalList;
        private List<DiffListItem> NewList;

        public List<object> Result;

        public ListDiffDialog(List<DiffListItem> originalList, List<DiffListItem> newList)
        {
            OriginalList = originalList;
            NewList = newList;

            leftListBox.DataSource = OriginalList;
            leftListBox.DisplayMember = "Name";


            // redo with datagrid view
            rightCheckBoxList.DataSource = NewList;
            rightCheckBoxList.DisplayMember = "Name";

            InitializeComponent();
        }
    }
}
