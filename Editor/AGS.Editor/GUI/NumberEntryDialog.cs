using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class NumberEntryDialog : Form
    {
        private NumberEntryDialog(string titleText, string headerText, int initialValue, int minValue, int maxValue)
        {
            InitializeComponent();
            this.Text = titleText;
            lblHeader.Text = headerText;
            udNumber.Minimum = minValue;
            udNumber.Maximum = maxValue;
            udNumber.Value = initialValue;
            udNumber.Select();
            udNumber.Select(0, ((int)udNumber.Value).ToString().Length);
        }

        public int Number
        {
            get { return (int)udNumber.Value; }
            set { udNumber.Value = value; }
        }

        public static int Show(string titleBar, string headerText, int currentValue, int minValue = Int32.MinValue, int maxValue = Int32.MaxValue)
        {
            int result = -1;
            NumberEntryDialog dialog = new NumberEntryDialog(titleBar, headerText, currentValue, minValue, maxValue);
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                result = dialog.Number;
            }
            dialog.Dispose();
            return result;
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            this.Close();
        }
    }
}