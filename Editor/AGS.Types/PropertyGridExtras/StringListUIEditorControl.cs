using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;

// Based on an idea by Ozcan Degirmenci:
// <http://www.ozcandegirmenci.com/post/2008/08/How-to-permit-multiple-selections-for-Enum-properties.aspx>

namespace AGS.Types
{
    public partial class StringListUIEditorControl : UserControl
    {
        private IWindowsFormsEditorService _service = null;
        private string _value;
        private bool _cancelFlag = false;

        public event ItemCheckEventHandler ItemCheck;

        public StringListUIEditorControl()
        {
            InitializeComponent();
        }

        // begin edit operation
        public void Begin(IWindowsFormsEditorService service, string value, IList<string> valueList)
        {
            _service = service;
            _value = value;
            clbItems.Items.Clear();
            List<string> selectedValues = new List<string>(value.Split(StringListUIEditor.Separators, StringSplitOptions.RemoveEmptyEntries));
            foreach (string s in valueList)
            {
                clbItems.Items.Add(s, selectedValues.Contains(s));
            }
        }

        // end edit operation
        public void End()
        {
            _cancelFlag = false;
            _service = null;
            _value = null;
        }

        public string Value
        {
            get
            {
                if (_cancelFlag) return _value;
                string value = "";
                foreach (object item in clbItems.CheckedItems)
                {
                    if (value.Length != 0) value += StringListUIEditor.Separators[0] + item.ToString();
                    else value = item.ToString();
                }
                return value;
            }
        }

        protected override bool ProcessDialogKey(Keys keyData)
        {
            if (((keyData & Keys.KeyCode) == Keys.Return)
                && ((keyData & (Keys.Alt | Keys.Control)) == Keys.None))
            {
                _service.CloseDropDown();
                return true;
            }
            if (((keyData & Keys.KeyCode) == Keys.Escape)
                && ((keyData & (Keys.Alt | Keys.Control)) == Keys.None))
            {
                _cancelFlag = true;
                _service.CloseDropDown();
                return true;
            }
            return base.ProcessDialogKey(keyData);
        }

        private void clbItems_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (ItemCheck != null) ItemCheck(sender, e);
        }
    }
}
