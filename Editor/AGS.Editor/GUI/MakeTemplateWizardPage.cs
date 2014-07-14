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
    public partial class MakeTemplateWizardPage : WizardPage
    {
        private string _createInDirectory;
        private string _templateFileExtension;

        public MakeTemplateWizardPage(string createInDirectory, string templateFileExtension)
        {
            _createInDirectory = createInDirectory;
            _templateFileExtension = templateFileExtension;
            InitializeComponent();
        }

        public string GetFullPath()
        {
            return Path.Combine(_createInDirectory, txtFileName.Text + _templateFileExtension);
        }

        public override bool NextButtonPressed()
        {
            if (txtFileName.Text.Length > 0)
            {
                if (!Utilities.DoesFileNameContainOnlyValidCharacters(txtFileName.Text))
                {
                    Factory.GUIController.ShowMessage("The file name you have specified includes some invalid characters. Please use just letters and numbers.", MessageBoxIcon.Warning);
                    return false;
                }
                if (File.Exists(GetFullPath()))
                {
                    if (Factory.GUIController.ShowQuestion("A template with this name already exists. Do you want to overwrite it?") == DialogResult.No)
                    {
                        return false;
                    }
                }
                return true;
            }
            Factory.GUIController.ShowMessage("You must type in a file name to continue.", MessageBoxIcon.Information);
            return false;
        }

        public override string TitleText
        {
            get
            {
                return "Decide how to name your template.";
            }
        }

        public override void PageShown()
        {
            txtFileName.Focus();
        }
    }
}
