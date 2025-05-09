using System;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class WelcomePane : EditorContentPanel
    {
        private readonly string[] TIPS_OF_THE_DAY = new string[] { 
            "You can right-click some selected sprites in the Sprite Manager, and use the Assign to View option to quickly add them to a view.",
            "You can right-click on a variable in the script editor and choose 'Go To Definition' to see where it was defined.",
            "In 256-colour games, you can right-click on the palette to export or replace it.",
            "Use the drop-down list at the top of the script editor to easily navigate through the script.",
            "The <a href=\"help:Global variables\">Global Variables</a> pane allows you to easily create variables that are shared between all your scripts.",
            "Alpha-channel sprites allow you to have much smoother edges, but only work in 32-bit colour games.",
            "The <a href=\"https://www.adventuregamestudio.co.uk/forums/\">AGS Forums</a> has several script modules and plugins that you can download to easily implement features in your game.",
            "The <a href=\"https://www.adventuregamestudio.co.uk/wiki/\">AGS Wiki</a> has lots of scripting tips. Why not contribute some yourself?",
            "To contribute to AGS development, check out <a href=\"https://github.com/adventuregamestudio\">AGS on GitHub.</a>",
            "Use the 'F' key to quickly flip frames in the view editor.",
            "Characters can talk and move between different rooms; objects cannot.",
            "You can select multiple sprites and move/delete them all in one go.",
            "Use the <a href=\"help:Character.ActiveInventory\">player.ActiveInventory property</a> to find out which item the player used in Use Inventory events.",
            "If your room background is continually flashing, make sure you didn't accidentally import a second background." };

        private int _currentTipIndex;
        private string _currentTipLinkTarget;
        private GUIController _guiContoller;

        public WelcomePane(GUIController guiContoller)
        {
            _guiContoller = guiContoller;
            InitializeComponent();
            lnkUpgrading.Links.Add(60, 18, "Upgrading from a previous version");
            lnkUpgrading.Links.Add(104, 23, "Upgrading to AGS 3.6.1");
            lnkUpgrading.Links.Add(133, 23, "Upgrading to AGS 3.6.2");
        }

        protected override string OnGetHelpKeyword()
        {
            return "Starting off";
        }

        private void lnkTutorial_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            _guiContoller.LaunchHelpForKeyword("Starting off");
        }

        private void lnkUpgrading_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            string keyword = e.Link.LinkData as string;
            if (keyword != null)
                _guiContoller.LaunchHelpForKeyword(keyword);
        }

        private void WelcomePane_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }

            _currentTipIndex = new Random().Next(0, TIPS_OF_THE_DAY.Length);
            ShowTipOfTheDay(_currentTipIndex);
        }

        private void lnkNextTip_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            _currentTipIndex++;
            if (_currentTipIndex >= TIPS_OF_THE_DAY.Length)
            {
                _currentTipIndex = 0;
            }
            ShowTipOfTheDay(_currentTipIndex);
        }

        private void ShowTipOfTheDay(int tipIndex)
        {
            pnlTipOfTheDay.SuspendDrawing();
            pnlTipOfTheDay.SuspendLayout();
            string tipText = TIPS_OF_THE_DAY[tipIndex];
            lnkTipText.LinkArea = new LinkArea(0, 0);
            _currentTipLinkTarget = null;

            int linkOffset = tipText.IndexOf("<a href=\"");
            if (linkOffset >= 0)
            {
                string linkTarget = tipText.Substring(linkOffset + 9);
                _currentTipLinkTarget = linkTarget.Substring(0, linkTarget.IndexOf('"'));
                tipText = tipText.Substring(0, linkOffset) + tipText.Substring(tipText.IndexOf('"', linkOffset + 9) + 2);

                int linkEnd = tipText.IndexOf("</a>");
                if (linkEnd >= 0)
                {
                    tipText = tipText.Substring(0, linkEnd) + tipText.Substring(linkEnd + 4);
                }

                lnkTipText.LinkArea = new LinkArea(linkOffset, linkEnd - linkOffset);
            }
            lnkTipText.Text = tipText;
            pnlTipOfTheDay.ResumeLayout();
            pnlTipOfTheDay.ResumeDrawing();
            pnlTipOfTheDay.Refresh();
            // we refresh the entire flowpanel because the pnl may have reduced in size because the tip is very small and we are at a very big OS scaling
            flowLayoutPanelLeft.Refresh();
        }

        private void lnkTipText_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            if (_currentTipLinkTarget.StartsWith("help:"))
            {
                _guiContoller.LaunchHelpForKeyword(_currentTipLinkTarget.Substring(5));
            }
            else
            {
                System.Diagnostics.Process.Start(_currentTipLinkTarget);
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("welcome/background", c => BackColor = c);
            t.SetColor("welcome/foreground", c => ForeColor = c);
            t.SetColor("welcome/panel1/background", c => panelTitle.BackColor = c);
            t.SetColor("welcome/panel1/foreground", c => panelTitle.ForeColor = c);
            t.SetColor("welcome/panel2/background", c => panelWelcomeToAgs.BackColor = c);
            t.SetColor("welcome/panel2/foreground", c => panelWelcomeToAgs.ForeColor = c);
            t.SetColor("welcome/pnlTipOfTheDay/background", c => pnlTipOfTheDay.BackColor = c);
            t.SetColor("welcome/pnlTipOfTheDay/foreground", c => pnlTipOfTheDay.ForeColor = c);
            t.SetColor("welcome/pnlRight/background", c => pnlRight.BackColor = c);
            t.SetColor("welcome/pnlRight/foreground", c => pnlRight.ForeColor = c);

            // [ivan-mogilko] had to do this try/catch hack for new entries
            // because ColorTheme methods do not have fallback mechanism atm
            try
            {
                Color linkColor = t.GetColor("welcome/panel2/link");
                foreach (LinkLabel link in tableWelcomeToAgs.Controls.OfType<LinkLabel>())
                {
                    link.LinkColor = linkColor;
                }
                linkColor = t.GetColor("welcome/pnlTipOfTheDay/link");
                foreach (LinkLabel link in tableTipOfTheDay.Controls.OfType<LinkLabel>())
                {
                    link.LinkColor = linkColor;
                }
                linkColor = t.GetColor("welcome/pnlRight/link");
                foreach (LinkLabel link in tableRight.Controls.OfType<LinkLabel>())
                {
                    link.LinkColor = linkColor;
                }
            }
            catch (Exception)
            {
            }
        }
    }
}
