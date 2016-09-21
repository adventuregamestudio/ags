using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace AGS.Editor.Components
{
    class HelpCommandsComponent : BaseComponent
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct Rect
        {
            public int Left { get; set; }
            public int Top { get; set; }
            public int Right { get; set; }
            public int Bottom { get; set; }
        }
        [StructLayout(LayoutKind.Sequential)]
        private struct MonitorInfo
        {
            public int cbSize;
            public Rect rcMonitor;
            public Rect rcWork;
            public uint dwFlags;
        }

        [DllImport("user32.dll")]
        private static extern IntPtr MonitorFromWindow(IntPtr windowHandle, uint flags);
        [DllImport("user32.dll")]
        private static extern bool GetMonitorInfo(IntPtr monitorHandle, ref MonitorInfo info);
        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool SetWindowPos(IntPtr windowHandle, IntPtr handleInsertAfter, int x, int y, int cx, int cy, uint flags);

        private const uint MONITOR_DEFAULTTONEAREST = 2;
        private const uint SWP_NOZORDER = 0x0004;

        private const string LAUNCH_HELP_COMMAND = "LaunchHelp";
        private const string HELP_CONTENTS_COMMAND = "HelpContents";
        private const string HELP_INDEX_COMMAND = "HelpIndex";
        private const string VISIT_AGS_WEBSITE = "GoToAGSWebsite";
        private const string VISIT_AGS_FORUMS = "GoToAGSForums";
        private const string CHECK_FOR_UPDATES = "CheckForSoftwareUpdates";
        private const string ABOUT_AGS_COMMAND = "AboutAGS";
        private const string CRASH_EDITOR_COMMAND = "CrashEditor";

        private const string UPDATES_URL = "http://www.adventuregamestudio.co.uk/releases/versions.xml";

        private string _helpFileName;
        private Form _dummyHelpForm;

        public HelpCommandsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _guiController.RegisterIcon("MenuIconAbout", Resources.ResourceManager.GetIcon("menu_help_about-ags.ico"));
            _guiController.RegisterIcon("MenuIconHelpContents", Resources.ResourceManager.GetIcon("menu_help_content.ico"));
            _guiController.RegisterIcon("MenuIconDynamicHelp", Resources.ResourceManager.GetIcon("menu_help_dynamic-help.ico"));
            _guiController.RegisterIcon("MenuIconHelpIndex", Resources.ResourceManager.GetIcon("menu_help_index.ico"));
            _guiController.RegisterIcon("MenuIconCheckForUpdate", Resources.ResourceManager.GetIcon("menu_help_update.ico"));
            _guiController.RegisterIcon("MenuIconVisitForums", Resources.ResourceManager.GetIcon("menu_help_visit-forums.ico"));
            _guiController.RegisterIcon("MenuIconVisitWebsite", Resources.ResourceManager.GetIcon("menu_help_visit-site.ico"));

            _guiController.AddMenu(this, GUIController.HELP_MENU_ID, "&Help");

            _guiController.OnLaunchHelp += new GUIController.LaunchHelpHandler(_guiController_OnLaunchHelp);

            MenuCommands commands = new MenuCommands(GUIController.HELP_MENU_ID, null);
            commands.Commands.Add(new MenuCommand(LAUNCH_HELP_COMMAND, "&Dynamic Help", Keys.F1, "MenuIconDynamicHelp"));
            commands.Commands.Add(new MenuCommand(HELP_CONTENTS_COMMAND, "&Contents", "MenuIconHelpContents"));
            commands.Commands.Add(new MenuCommand(HELP_INDEX_COMMAND, "&Index", "MenuIconHelpIndex"));
            commands.Commands.Add(MenuCommand.Separator);
            commands.Commands.Add(new MenuCommand(VISIT_AGS_WEBSITE, "Visit the AGS &Website", "MenuIconVisitWebsite"));
            commands.Commands.Add(new MenuCommand(VISIT_AGS_FORUMS, "Visit the AGS &Forums", "MenuIconVisitForums"));
            commands.Commands.Add(MenuCommand.Separator);
            commands.Commands.Add(new MenuCommand(CHECK_FOR_UPDATES, "Chec&k for Updates", "MenuIconCheckForUpdate"));
            _guiController.AddMenuItems(this, commands);

            commands = new MenuCommands(GUIController.HELP_MENU_ID, 1000);
#if DEBUG
            commands.Commands.Add(new MenuCommand(CRASH_EDITOR_COMMAND, "Crash editor DEBUG ONLY"));
#endif
            commands.Commands.Add(new MenuCommand(ABOUT_AGS_COMMAND, "&About AGS...", "MenuIconAbout"));
            _guiController.AddMenuItems(this, commands);

            _helpFileName = Path.Combine(_agsEditor.EditorDirectory, "ags-help.chm");
        }

        public override string ComponentID
        {
            get { return ComponentIDs.HelpCommands; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == CRASH_EDITOR_COMMAND)
            {
                throw new AGSEditorException("Crash test");
            }
            else if (controlID == ABOUT_AGS_COMMAND)
            {
                AboutDialog dialog = new AboutDialog();
                dialog.ShowDialog();
                dialog.Dispose();
                return;
            }
            else if (controlID == VISIT_AGS_WEBSITE)
            {
                LaunchBrowserAtAGSWebsite();
                return;
            }
            else if (controlID == VISIT_AGS_FORUMS)
            {
                LaunchBrowserAtAGSForums();
                return;
            }
            else if (controlID == CHECK_FOR_UPDATES)
            {
                CheckForUpdates();
                return;
            }
            else if (!File.Exists(_helpFileName))
            {
                _guiController.ShowMessage("The help file '" + _helpFileName + "' is missing. You may need to reinstall AGS.", MessageBoxIcon.Warning);
                return;
            }
            else if (Utils.AlternateStreams.GetZoneIdentifier(_helpFileName) > Utils.AlternateStreams.URLZONE_LOCAL_MACHINE)
            {
                if (_guiController.ShowQuestion("The help file '" + _helpFileName + "' has a restrictive Zone Identifier which needs to be removed." + Environment.NewLine + Environment.NewLine + "Do you want to try unblocking this file?") == DialogResult.Yes)
                {
                    if (Utils.AlternateStreams.DeleteZoneIdentifier(_helpFileName))
                    {
                        _guiController.ShowMessage("The help file was unblocked successfully.", MessageBoxIcon.Warning);
                    }
                    else
                    {
                        _guiController.ShowMessage("The help file couldn't be unblocked. Please try running the AGS editor using 'Run as administrator' and try to unblock the file again.", MessageBoxIcon.Warning);
                        return;
                    }
                }
                else
                {
                    return;
                }
            }
            if (controlID == LAUNCH_HELP_COMMAND)
            {
                string keyword = string.Empty;
                if (_guiController.ActivePane != null)
                {
                    keyword = _guiController.ActivePane.Control.HelpKeyword;
                }
                LaunchHelp(_helpFileName, HelpNavigator.KeywordIndex, keyword);
            }
            else if (controlID == HELP_CONTENTS_COMMAND)
            {
                Help.ShowHelp(GetHelpParentWindow(), _helpFileName);
                AdjustHelpWindowSize();
            }
            else if (controlID == HELP_INDEX_COMMAND)
            {
                Help.ShowHelpIndex(GetHelpParentWindow(), _helpFileName);
                AdjustHelpWindowSize();
            }
        }

        private void _guiController_OnLaunchHelp(string keyword)
        {
            LaunchHelp(_helpFileName, HelpNavigator.KeywordIndex, keyword);
        }

        private void LaunchHelp(string url, HelpNavigator command, object parameter)
        {
            Help.ShowHelp(GetHelpParentWindow(), url, command, parameter);
            AdjustHelpWindowSize();
        }

        /// <summary>
        /// Hack to get around the fact that Help.ShowHelp can make the help window
        /// modal over the application window
        /// </summary>
        private Form GetHelpParentWindow()
        {
            if (_agsEditor.Preferences.KeepHelpOnTop)
            {
                return Form.ActiveForm;
            }
            if (_dummyHelpForm == null)
            {
                _dummyHelpForm = new Form();
                _dummyHelpForm.CreateControl();
            }
            return _dummyHelpForm;
        }

        private void LaunchBrowserAtAGSWebsite()
        {
            System.Diagnostics.Process.Start("http://www.adventuregamestudio.co.uk");
        }

        private void LaunchBrowserAtAGSDownloadPage()
        {
            System.Diagnostics.Process.Start("http://www.adventuregamestudio.co.uk/site/ags/");
        }

        private void LaunchBrowserAtAGSForums()
        {
            System.Diagnostics.Process.Start("http://www.adventuregamestudio.co.uk/forums/index.php");
        }

        private void LaunchBrowserAtPage(string url)
        {
            System.Diagnostics.Process.Start(url);
        }

        private object DownloadUpdateStatusThread(object parameter)
        {
            using (System.Net.WebClient webClient = new System.Net.WebClient())
            {
                byte[] data = webClient.DownloadData(UPDATES_URL);
                return Encoding.Default.GetString(data);
            }
        }

        private void CheckForUpdates()
        {
            try
            {
                string dataDownload = (string)BusyDialog.Show("Please wait while we check for updates...", new BusyDialog.ProcessingHandler(DownloadUpdateStatusThread), null);

                XmlDocument doc = new XmlDocument();
                doc.LoadXml(dataDownload);
                string newVersionName;

                VersionCheckStatus status = CompareVersionWithXML(doc, "latest_version_number_full", "latest_version_number", out newVersionName);
                if (status == VersionCheckStatus.ServerNewer)
                {
                    if (_guiController.ShowQuestion("A newer version of AGS (" + newVersionName + ") is available on the AGS website. Would you like to go there now?") == DialogResult.Yes)
                    {
                        LaunchBrowserAtAGSDownloadPage();
                    }
                }
                else if (status == VersionCheckStatus.ThisNewer)
                {
                    // This is newer than the website version, so it must be a beta
                    // version. So, see if a newer beta is available.
                    status = CompareVersionWithXML(doc, "development_version_number_full", "development_version_number", out newVersionName);
                    if (status == VersionCheckStatus.ServerNewer)
                    {
                        if (_guiController.ShowQuestion("A newer in-development version of AGS (" + newVersionName + ") is available on the AGS forums. Would you like to go there now?") == DialogResult.Yes)
                        {
                            LaunchBrowserAtPage(GetPageURL(doc, "development_version_thread"));
                        }
                    }
                    else
                    {
                        _guiController.ShowMessage("There are no further in-development updates at this time.", MessageBoxIcon.Information);
                    }
                }
                else
                {
                    _guiController.ShowMessage("This version of AGS is up to date.", MessageBoxIcon.Information);
                }
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("Unable to check for updates. Your internet connection may be down, or server response had mistakes.\nPlease visit the AGS website to see if an updated version is available.\n\nError details: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private string GetPageURL(XmlDocument doc, string xmlElementName)
        {
            XmlNode urlNode = doc.DocumentElement.SelectSingleNode(xmlElementName);
            return urlNode.InnerText;
        }

        private VersionCheckStatus CompareVersionWithXML(XmlDocument doc, string xmlElVersionFull, string xmlElVersionFriendly, out string newVersionText)
        {
            XmlNode serverVersionNode = doc.DocumentElement.SelectSingleNode(xmlElVersionFull);
            string serverVersionText = serverVersionNode.InnerText;
            VersionCheckStatus status = CompareSoftwareVersions(serverVersionText);
            newVersionText = serverVersionText;

            serverVersionNode = doc.DocumentElement.SelectSingleNode(xmlElVersionFriendly);
            if (serverVersionNode != null && !String.IsNullOrEmpty(serverVersionNode.InnerText))
            {
                newVersionText = serverVersionNode.InnerText;
            }

            return status;
        }

        private VersionCheckStatus CompareSoftwareVersions(string serverVersionText)
        {
            string[] serverVersion = serverVersionText.Split('.');
            string[] thisVersion = AGS.Types.Version.AGS_EDITOR_VERSION.Split('.');
            VersionCheckStatus status = VersionCheckStatus.Equal;

            for (int i = 0; i < serverVersion.Length; i++)
            {
                int serverVersionPart = Convert.ToInt32(serverVersion[i]);
                int thisVersionPart = Convert.ToInt32(thisVersion[i]);
                if (serverVersionPart > thisVersionPart)
                {
                    status = VersionCheckStatus.ServerNewer;
                    break;
                }
                if (serverVersionPart < thisVersionPart)
                {
                    status = VersionCheckStatus.ThisNewer;
                    break;
                }
            }
            return status;
        }

        /// <summary>
        /// The HTML Help API's HtmlHelp method forces the help window to stretch across multiple monitors if it opens at a negative location (-x, -y).
        /// This method resizes the window to the work-space size of the monitor that it is in.
        /// This method must be called immediately after opening the Help window with the Help class.
        /// </summary>
        private bool AdjustHelpWindowSize()
        {
            IntPtr helpHandle = Process.GetCurrentProcess().MainWindowHandle; // get the Help window handle
            if (helpHandle == null)
            {
                return false;
            }
            MonitorInfo monitorInfo = new MonitorInfo();
            monitorInfo.cbSize = Marshal.SizeOf(monitorInfo);
            if (!GetMonitorInfo(MonitorFromWindow(helpHandle, MONITOR_DEFAULTTONEAREST), ref monitorInfo))
            {
                return false;
            }
            Rect workSize = monitorInfo.rcWork;
            return SetWindowPos(helpHandle, IntPtr.Zero, workSize.Left, workSize.Top, workSize.Right - workSize.Left, workSize.Bottom - workSize.Top, SWP_NOZORDER);
        }

        private enum VersionCheckStatus
        {
            Equal,
            ServerNewer,
            ThisNewer
        }
    }
}
