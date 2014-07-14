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
    public partial class StartNewGameWizardPage : WizardPage
    {
        private List<GameTemplate> _templates;
        private ImageList _imageList = new ImageList();

        public StartNewGameWizardPage(List<GameTemplate> templates)
        {
            InitializeComponent();
            _templates = templates;
            _imageList.ImageSize = new Size(32, 32);
            _imageList.Images.Add("_default", Resources.ResourceManager.GetIcon("template_noicon.ico"));

            foreach (GameTemplate template in _templates)
            {
                string imageKey = "_default";
                if (template.Icon != null)
                {
                    imageKey = template.FileName;
                    _imageList.Images.Add(imageKey, template.Icon);
                }

                ListViewItem newItem = lstTemplates.Items.Add(template.FriendlyName);
                newItem.ImageKey = imageKey;
            }
            lstTemplates.LargeImageList = _imageList;
        }

        public GameTemplate SelectedTemplate
        {
            get { return _templates[lstTemplates.SelectedIndices[0]]; }
        }

        public override bool NextButtonPressed()
        {
            if (lstTemplates.SelectedItems.Count == 1)
            {
                return true;
            }
            Factory.GUIController.ShowMessage("You must select a template before continuing.", MessageBoxIcon.Information);
            return false;
        }

        public override string TitleText
        {
            get
            {
                return "Select the template to base your new game on.";
            }
        }

        public override void PageShown()
        {
            lstTemplates.Focus();
        }
    }
}
