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
            StringBuilder sb = new StringBuilder();
            sb.Append($"AGS Editor .NET (Build {Types.Version.AGS_EDITOR_VERSION})");

            if (Types.Version.IS_BETA_VERSION)
            {
                sb.AppendLine(" ** BETA VERSION **");
            }
            else
            {
                sb.AppendLine();
            }

            sb.AppendLine($"v{Types.Version.AGS_EDITOR_FRIENDLY_VERSION}, {Types.Version.AGS_EDITOR_DATE}");
            sb.AppendLine(Types.Version.AGS_EDITOR_COPYRIGHT);
            sb.AppendLine("See the Licenses folder for copyrights of used libraries.");
            sb.AppendLine("DockPanel Suite © 2007 Weifen Luo");
            sb.AppendLine("irrKlang © 2001-2018 Nikolaus Gebhardt / Ambiera");
            sb.AppendLine("Magick.NET © 2013-2019 Dirk Lemstra");
            sb.AppendLine("Newtonsoft JSON.NET © 2007 James Newton-King");
            sb.AppendLine("Scintilla © 1998-2003 Neil Hodgson");
            sb.AppendLine("System: " + GetOperatingSystemName());

            // pickup extra information which might be set (i.e. loaded plug-ins)
			GetAboutDialogTextEventArgs evArgs = new GetAboutDialogTextEventArgs(string.Empty);
			Factory.Events.OnGetAboutDialogText(evArgs);

            // ...value is either completely empty or already includes a newline
            if (!string.IsNullOrWhiteSpace(evArgs.Text))
            {
                sb.Append(evArgs.Text);
            }

            txtInfo.Text = sb.ToString().TrimEnd();
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