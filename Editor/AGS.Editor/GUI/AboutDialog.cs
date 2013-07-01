using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class AboutDialog : Form
    {
        public AboutDialog()
        {
            InitializeComponent();
            pictureBox.Image = Resources.ResourceManager.GetBitmap("splash.bmp");

            txtInfo.Text = "AGS Editor .NET (Build " + AGS.Types.Version.AGS_EDITOR_VERSION + ")";
            switch (AGS.Types.Version.VERSION_TYPE)
            {
                case AGS.Types.Version.Type.Alpha:
                    txtInfo.Text += " ** ALPHA VERSION **";
                    break;
                case AGS.Types.Version.Type.Beta:
                    txtInfo.Text += " ** BETA VERSION **";
                    break;
                case AGS.Types.Version.Type.Development:
                    txtInfo.Text += " ** DEVELOPMENT VERSION **";
                    break;
                case AGS.Types.Version.Type.Release:
                    // add nothing
                    break;
                case AGS.Types.Version.Type.ReleaseCandidate:
                    txtInfo.Text += " ** RELEASE CANDIDATE **";
                    break;
            }
            txtInfo.Text += Environment.NewLine +
				"v" + AGS.Types.Version.AGS_EDITOR_FRIENDLY_VERSION + ", " + AGS.Types.Version.AGS_EDITOR_DATE +
                Environment.NewLine +
                AGS.Types.Version.AGS_EDITOR_COPYRIGHT + Environment.NewLine +
				"Scintilla (c) 1998-2003 Neil Hodgson, all rights reserved" +
				Environment.NewLine +
                "DockPanel Suite (c) 2007 Weifen Luo" +
                Environment.NewLine +
				"See the DOCS folder for copyrights of used libraries." +
				Environment.NewLine + 
				"System: " + GetOperatingSystemName() + 
				Environment.NewLine;

			GetAboutDialogTextEventArgs evArgs = new GetAboutDialogTextEventArgs(string.Empty);
			Factory.Events.OnGetAboutDialogText(evArgs);
			txtInfo.Text += evArgs.Text;
        }

        private string GetOperatingSystemName()
        {
            string osName = Environment.OSVersion.VersionString;
            if (Environment.OSVersion.Platform == PlatformID.Win32NT)
            {
                osName = "Windows 2000";
                if (Environment.OSVersion.Version.Major == 5)
                {
                    if (Environment.OSVersion.Version.Minor == 1)
                    {
                        osName = "Windows XP";
                    }
                    else if (Environment.OSVersion.Version.Minor > 1)
                    {
                        osName = "Windows Server 2003";
                    }
                }
                else if (Environment.OSVersion.Version.Major == 6)
                {
                    osName = "Windows Vista";

                    if (Environment.OSVersion.Version.Minor > 0)
                    {
                        osName = "Windows 7";
                    }
                }
                else if (Environment.OSVersion.Version.Major > 6)
                {
                    osName = "Windows";
                }
            }
            return osName + " " + Environment.OSVersion.ServicePack;
        }

		private void btnOK_Click(object sender, EventArgs e)
		{
			if (Utilities.IsShiftPressed())
			{
				pictureBox.Image = Resources.ResourceManager.GetBitmap("splash-mittens.bmp");
				this.Text = "The cast and crew of Mittens 2007 Canada";
				txtInfo.Text = "Greetings from the cast of Mittens VI. One Fop to rule them all ... One Fop to find them, One Fop to come to them all and in darkness be drowned by them.";
				this.DialogResult = DialogResult.None;
			}
		}
    }
}