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
            Text = wizardName;
            lblHeader1.Text = wizardName;
            btnBack.Enabled = false;
            _pageNumber = 0;
            _pages = pages;
            Size needPanelSize = pnlMainPages.Size;
            // Scan all pages and remember the minimal necessary size of the panel,
            // assuming that there will be a "header panel" on top too.
            foreach (var page in pages)
            {
                if (page.MinimumSize.Width > needPanelSize.Width)
                    needPanelSize.Width = page.MinimumSize.Width;
                if (page.MinimumSize.Height > needPanelSize.Height - pnlHeader.Height)
                    needPanelSize.Height = page.MinimumSize.Height + pnlHeader.Height;
            }
            // After we're done with the regular pages,
            // insert the "Wizard Intro Page" as the starting page,
            // (it will have "header panel" hidden while its displayed).
            _pages.Insert(0, new WizardIntroPage(wizardName, introText));
            needPanelSize.Width = Math.Max(needPanelSize.Width, _pages[0].MinimumSize.Width);
            needPanelSize.Height = Math.Max(needPanelSize.Height, _pages[0].MinimumSize.Height);
            if (pnlMainPages.Width < needPanelSize.Width)
                Width += needPanelSize.Width - pnlMainPages.Width;
            if (pnlMainPages.Height < needPanelSize.Height)
                Height += needPanelSize.Height - pnlMainPages.Height;

            Utilities.CheckLabelWidthsOnForm(this);
        }

        private void WizardDialog_Load(object sender, EventArgs e)
        {
            UpdatePanels();
        }

        private void btnNext_Click(object sender, EventArgs e)
        {
            if (!_pages[_pageNumber].NextButtonPressed())
            {
                return;
            }

            _pageNumber++;
            if (_pageNumber >= _pages.Count)
            {
                DialogResult = DialogResult.OK;
                Close();
                return;
            }

            
            UpdatePanels();
        }

        private void btnBack_Click(object sender, EventArgs e)
        {
            _pageNumber--;
            UpdatePanels();
        }

        private void UpdatePanels()
        {
            if (_pageNumber == _pages.Count - 1)
            {
                btnNext.Text = "&Finish";
            }
            else
            {
                btnNext.Text = "&Next >";
            }

            btnBack.Enabled = (_pageNumber > 0);
            pnlMainPages.Controls.Clear();
            if (_pageNumber == 0)
            {
            }
            else
            {
                pnlMainPages.Controls.Add(pnlHeader);
                lblHeader2.Text = _pages[_pageNumber].TitleText;
            }
            pnlMainPages.Controls.Add(_pages[_pageNumber]);
            _pages[_pageNumber].BringToFront();
            _pages[_pageNumber].PageShown();
            Utilities.CheckLabelWidthsOnForm(this);
        }
    }
}