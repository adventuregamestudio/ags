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

            splashPage.ConstructSimple();

            txtInfo.Text = "AGS Editor .NET (Build " + AGS.Types.Version.AGS_EDITOR_VERSION + ")";
            if (AGS.Types.Version.IS_BETA_VERSION)
            {
                txtInfo.Text += " ** BETA VERSION **";
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
                    switch (Environment.OSVersion.Version.Minor)
                    {
                        case 3:
                            osName = "Windows 8.1";
                            break;
                        case 2:
                            osName = "Windows 8";
                            break;
                        case 1:
                            osName = "Windows 7";
                            break;
                        default:
                            osName = "Windows Vista";
                            break;
                    }
                }
                else if (Environment.OSVersion.Version.Major > 6)
                {
                    osName = "Windows " + Environment.OSVersion.Version.Major + (Environment.OSVersion.Version.Minor > 0 ? "." + Environment.OSVersion.Version.Minor : "");
                }
            }
            return osName + " " + Environment.OSVersion.ServicePack;
        }

		private void btnOK_Click(object sender, EventArgs e)
		{
			if (Utilities.IsShiftPressed())
			{
				splashPage.ConstructSpecial("splash-mittens.bmp", ImageLayout.Zoom, "", Color.White);
				this.Text = "The cast and crew of Mittens 2007 Canada";
				txtInfo.Text = "Greetings from the cast of Mittens VI. One Fop to rule them all ... One Fop to find them, One Fop to come to them all and in darkness be drowned by them.";
				this.DialogResult = DialogResult.None;
			}
		}
    }
}