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
    /// THIS CLASS IS TEMPORARY ... NEED TO REFACTOR IT TO BECOME A PROPERLY
    /// FLEXIBLE WIZARD CLASS. For now it's just for the import game wizard
    /// </summary>
    public partial class WizardDialog : Form
    {
        private int _pageNumber;
        private List<WizardPage> _pages;

        public WizardDialog(string wizardName, string introText, List<WizardPage> pages)
        {
            InitializeComponent();
            this.Text = wizardName;
            this.lblTitle.Text = "Welcome to the " + wizardName + " Wizard";
            this.lblIntroText.Text = introText;
            this.lblHeader1.Text = wizardName;
            this.pnlMainPages.Visible = false;
            this.btnBack.Enabled = false;
            _pageNumber = 0;
            _pages = pages;
            Utilities.CheckLabelWidthsOnForm(this);
        }

        private void btnNext_Click(object sender, EventArgs e)
        {
            if (_pageNumber == 0)
            {
                this.pnlMainPages.Visible = true;
                this.btnBack.Enabled = true;
            }
            else
            {
                if (!_pages[_pageNumber - 1].NextButtonPressed())
                {
                    return;
                }
            }

            if (_pageNumber >= _pages.Count)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
                return;
            }

            _pageNumber++;
            UpdatePanels();
        }

        private void UpdatePanels()
        {
            if (_pageNumber >= _pages.Count)
            {
                btnNext.Text = "&Finish";
            }
            else
            {
                btnNext.Text = "&Next >";
            }

            if (_pageNumber > 0)
            {
                this.pnlMainPages.Controls.Clear();
                this.pnlMainPages.Controls.Add(this.pnlHeader);
                this.pnlMainPages.Controls.Add(_pages[_pageNumber - 1]);
                _pages[_pageNumber - 1].BringToFront();
                _pages[_pageNumber - 1].PageShown();
                this.lblHeader2.Text = _pages[_pageNumber - 1].TitleText;
                Utilities.CheckLabelWidthsOnForm(this);
            }
        }

        private void btnBack_Click(object sender, EventArgs e)
        {
            _pageNumber--;
            if (_pageNumber == 0)
            {
                this.pnlMainPages.Visible = false;
                this.btnBack.Enabled = false;
            }
            UpdatePanels();
        }
    }
}