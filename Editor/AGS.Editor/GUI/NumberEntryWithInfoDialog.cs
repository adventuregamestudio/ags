using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class NumberEntryWithInfoDialog : Form
    {
        private NumberEntryWithInfoDialog(string titleText, string headerText, string infoText,
            int initialValue, int minValue, int maxValue)
        {
            InitializeComponent();
            this.Text = titleText;
            lblHeader.Text = headerText;
            lblInfo.Text = infoText;
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

        public static int Show(string titleBar, string headerText, string infoText,
            int currentValue, int minValue = Int32.MinValue, int maxValue = Int32.MaxValue)
        {
            int result = -1;
            NumberEntryWithInfoDialog dialog = new NumberEntryWithInfoDialog(titleBar, headerText, infoText,
                currentValue, minValue, maxValue);
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