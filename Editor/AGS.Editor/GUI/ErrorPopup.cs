using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ErrorPopup : Form
    {
        public ErrorPopup(string errorMessage)
        {
            InitializeComponent();
            lblErrorMessage.Text = errorMessage;
        }

        private void closeBox_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

        private void ErrorPopup_Paint(object sender, PaintEventArgs e)
        {
        }

        private void btnCloseGame_Click(object sender, EventArgs e)
        {
            Factory.AGSEditor.Debugger.Resume();
        }

        private void ErrorPopup_FormClosed(object sender, FormClosedEventArgs e)
        {
            Factory.AGSEditor.Debugger.Resume();
        }
    }
}