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
    public partial class Progress : Form
    {
        public Progress(int max, string text)
        {
            InitializeComponent();
            progressBar.Maximum = max;
            lblTask.Text = text;
        }

        public void SetProgressValue(int value)
        {
            progressBar.Value = Math.Min(value, progressBar.Maximum);
        }

        private void Progress_Activated(object sender, EventArgs e)
        {
            lblTask.Refresh();
        }
    }
}
