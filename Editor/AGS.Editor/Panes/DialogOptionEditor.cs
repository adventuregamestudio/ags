using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class DialogOptionEditor : UserControl
    {
        public DialogOptionEditor(DialogOption option, EventHandler onOptionChanged)
        {
            InitializeComponent();
            if (onOptionChanged != null)
            {
                chkSay.CheckedChanged += onOptionChanged;
                txtOptionText.TextChanged += onOptionChanged;
            }
            chkSay.DataBindings.Add("Checked", option, "Say", false, DataSourceUpdateMode.OnPropertyChanged);
            chkShow.DataBindings.Add("Checked", option, "Show", false, DataSourceUpdateMode.OnPropertyChanged);
            txtOptionText.DataBindings.Add("Text", option, "Text", false, DataSourceUpdateMode.OnPropertyChanged);
            lblOptionID.Text = option.ID.ToString() + ":";
            txtOptionText.Focus();
        }
    }
}
