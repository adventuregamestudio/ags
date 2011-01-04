using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// April Fool Edition
    /// </summary>
    public partial class SelectReligion : Form
    {
        public SelectReligion()
        {
            InitializeComponent();
        }

        private void btnContinue_Click(object sender, EventArgs e)
        {
            if (lstReligion.SelectedItem == null)
            {
                return;
            }
            Utilities.SelectedReligion = lstReligion.SelectedItem.ToString();
            this.Close();
        }
    }
}
