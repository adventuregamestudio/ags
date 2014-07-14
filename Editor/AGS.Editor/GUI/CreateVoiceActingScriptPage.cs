using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class CreateVoiceActingScriptPage : WizardPage
    {
        public CreateVoiceActingScriptPage()
        {
            InitializeComponent();
        }

        public string SelectedFilePath
        {
            get { return txtFilePath.Text; }
            set { txtFilePath.Text = value; }
        }

        public override string TitleText
        {
            get
            {
                return "Specify where to generate the script.";
            }
        }

        public override bool NextButtonPressed()
        {
            return true;
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            string selectedPath = Factory.GUIController.ShowSaveFileDialog("Select output file", "Text files (*.txt)|*.txt");
            if (selectedPath != null)
            {
                txtFilePath.Text = selectedPath;
            }
        }

    }
}
