using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class TextEntryDialog : Form
    {
        private TextEntryDialog(string titleText, string headerText, string initialValue)
        {
            InitializeComponent();
            this.Text = titleText;
            lblHeader.Text = headerText;
            txtEntry.Text = initialValue;
        }

        public string TextEntered
        {
            get { return txtEntry.Text; }
            set { txtEntry.Text = value; }
        }

        public static string Show(string titleBar, string headerText, string currentValue)
        {
            string result = null;
            TextEntryDialog dialog = new TextEntryDialog(titleBar, headerText, currentValue);
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                result = dialog.TextEntered;
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

        private void TextEntryDialog_Load(object sender, EventArgs e)
        {
            txtEntry.Focus();
        }
    }
}