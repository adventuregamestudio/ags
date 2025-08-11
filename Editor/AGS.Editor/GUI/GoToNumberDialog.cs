using AGS.Editor.Components;
using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class GoToNumberDialog : Form
    {
        private string _nodeTypeName;
        private List<Tuple<int, string>> _list;

        public GoToNumberDialog()
        {
            InitializeComponent();
            (this.upDownNumber.Controls[1] as TextBox).Enter += upDownNumber_Controls1_Enter;
        }

        private delegate void Action();

        private void upDownNumber_Controls1_Enter(object sender, EventArgs e)
        {
            BeginInvoke((Action)(() =>
            {
                (this.upDownNumber.Controls[1] as TextBox).SelectAll();
            }));
        }

        public int Number
        {
            get { return (int)upDownNumber.Value; }
            set { upDownNumber.Value = value; }
        }

        public int Minimum
        {
            get { return (int)upDownNumber.Minimum; }
            set 
            { 
                upDownNumber.Minimum = value;
                SetLabelText();
            }
        }

        public int Maximum
        {
            get { return (int)upDownNumber.Maximum; }
            set
            {
                upDownNumber.Maximum = value;
                SetLabelText();
            }
        }

        public string NodeTypeName
        {
            get { return _nodeTypeName; }
            set { _nodeTypeName = value; }
        }

        public List<Tuple<int, string>> List
        {
            get { return _list; }
            set 
            {
                _list = value; 
                lstNodes.Items.Clear();
                foreach (Tuple<int, string> item in _list)
                {
                    lstNodes.Items.Add(item);
                }
                int min = _list.Min(i => i.Item1);
                int max = _list.Max(i => i.Item1);
                Minimum = min;
                Maximum = max;
                Number = min;
            }
        }

        private void SetLabelText()
        {
            lblNodeNumberRange.Text = String.Format("{0} Number ({1} - {2}):", NodeTypeName, Minimum, Maximum);
        }

        private Tuple<int, string> GetItemByNumber(int number)
        {
            List<Tuple<int, string>> matches = _list.Where(itm => itm.Item1 == number).ToList();
            Tuple<int, string> match = null;
            if (matches.Count > 0)
            {
                match = matches.First();
            }
            return match;
        }

        private bool ExistInList (int number)
        {
            return GetItemByNumber(number) != null;
        }

        private void syncFromUpDownToListBox()
        {
            if (ExistInList(Number))
            {
                btnOk.Enabled = true;
                Tuple<int, string> selected = GetItemByNumber(Number);
                if (selected != lstNodes.SelectedItem)
                {
                    lstNodes.SelectedItem = selected;
                }
            }
            else
            {
                btnOk.Enabled = false;
                lstNodes.SelectedItem = null;
                lstNodes.Invalidate();
            }
        }

        private void syncFromListBoxToUpDown()
        {
            if (lstNodes.SelectedItem is Tuple<int, string>)
            {
                Tuple<int, string> item = (Tuple<int, string>)lstNodes.SelectedItem;
                if (upDownNumber.Value != item.Item1)
                {
                    upDownNumber.Value = item.Item1;
                }
            }
        }

        private void upDownNumber_ValueChanged(object sender, EventArgs e)
        {
            syncFromUpDownToListBox();
        }

        private void upDownNumber_KeyUp(object sender, KeyEventArgs e)
        {
            syncFromUpDownToListBox();
        }

        private void lstNodes_SelectedValueChanged(object sender, EventArgs e)
        {
            syncFromListBoxToUpDown();
        }

        private void lstNodes_SelectedIndexChanged(object sender, EventArgs e)
        {
            syncFromListBoxToUpDown();
        }

        private void lstNodes_Format(object sender, ListControlConvertEventArgs e)
        {
            if (e.ListItem is Tuple<int, string>)
            {
                Tuple<int, string> item = (Tuple<int, string>)e.ListItem;
                e.Value = string.Format("{0}: {1}", item.Item1, item.Item2);
            }
            else
            {
                e.Value = "Unknown item!";
            }
        }

        private void lstNodes_DoubleClick(object sender, EventArgs e)
        {
            // as far as I can tell, when a double click event triggers it already
            // select the thing the mouse was over in the first of the two clicks
            // so we will sync the upDown control to match the number and then click OK.
            syncFromListBoxToUpDown();
            if (btnOk.Enabled)
            {
                btnOk.PerformClick();
            }
        }
    }
}
