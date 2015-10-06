using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace AGS.Editor.Components
{
    class HelpCommandsComponent : BaseComponent
    {
        private const string LAUNCH_HELP_COMMAND = "LaunchHelp";
        private const string HELP_CONTENTS_COMMAND = "HelpContents";
        private const string HELP_INDEX_COMMAND = "HelpIndex";
        private const string VISIT_AGS_WEBSITE = "GoToAGSWebsite";
        private const string VISIT_AGS_FORUMS = "GoToAGSForums";
        private const string CHECK_FOR_UPDATES = "CheckForSoftwareUpdates";
        private const string ABOUT_AGS_COMMAND = "AboutAGS";
        private const string CRASH_EDITOR_COMMAND = "CrashEditor";

        private const string UPDATES_URL = "http://www.adventuregamestudio.co.uk/updatecheck.php";

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
            if (controlID == ABOUT_AGS_COMMAND)
            {
                AboutDialog dialog = new AboutDialog();
                dialog.ShowDialog();
                dialog.Dispose();
            }
            else if (controlID == VISIT_AGS_WEBSITE)
            {
                LaunchBrowserAtAGSWebsite();
            }
            else if (controlID == VISIT_AGS_FORUMS)
            {
                LaunchBrowserAtAGSForums();
            }
            else if (controlID == CHECK_FOR_UPDATES)
            {
                CheckForUpdates();
            }
            else if (!File.Exists(_helpFileName))
            {
                _guiController.ShowMessage("The help file '" + _helpFileName + "' is missing. You may need to reinstall AGS.", MessageBoxIcon.Warning);
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
                    }
                }  
            }
            else if (controlID == LAUNCH_HELP_COMMAND)
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
            }
            else if (controlID == HELP_INDEX_COMMAND)
            {
                Help.ShowHelpIndex(GetHelpParentWindow(), _helpFileName);
            }
            else if (controlID == CRASH_EDITOR_COMMAND)
            {
                throw new AGSEditorException("Crash test");
            }
        }

        private void _guiController_OnLaunchHelp(string keyword)
        {
            LaunchHelp(_helpFileName, HelpNavigator.KeywordIndex, keyword);
        }

        private void LaunchHelp(string url, HelpNavigator command, object parameter)
        {
            Help.ShowHelp(GetHelpParentWindow(), url, command, parameter);
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

        private void LaunchBrowserAtAGSForums()
        {
            System.Diagnostics.Process.Start("http://www.adventuregamestudio.co.uk/forums/index.php");
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
                VersionCheckStatus status = CompareVersionWithXML(doc, "CurrentVersion", out newVersionName);

                if (status == VersionCheckStatus.ServerNewer)
                {
                    if (_guiController.ShowQuestion("A newer version of AGS (" + newVersionName + ") is available on the AGS website. Would you like to go there now?") == DialogResult.Yes)
                    {
                        LaunchBrowserAtAGSWebsite();
                    }
                }
                else if (status == VersionCheckStatus.ThisNewer)
                {
                    // This is newer than the website version, so it must be a beta
                    // version. So, see if a newer beta is available.
                    status = CompareVersionWithXML(doc, "BetaVersion", out newVersionName);
                    if (status == VersionCheckStatus.ServerNewer)
                    {
                        if (_guiController.ShowQuestion("A newer beta version of AGS (" + newVersionName + ") is available on the AGS forums. Would you like to go there now?") == DialogResult.Yes)
                        {
                            LaunchBrowserAtAGSForums();
                        }
                    }
                    else
                    {
                        _guiController.ShowMessage("There are no further beta updates at this time.", MessageBoxIcon.Information);
                    }
                }
                else
                {
                    _guiController.ShowMessage("This version of AGS is up to date.", MessageBoxIcon.Information);
                }
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("Unable to check for updates. Your internet connection may be down.\nPlease visit the AGS website to see if an updated version is available.\n\nError details: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private VersionCheckStatus CompareVersionWithXML(XmlDocument doc, string xmlElementName, out string newVersionText)
        {
            XmlNode serverVersionNode = doc.DocumentElement.SelectSingleNode(xmlElementName);
            string serverVersionText = serverVersionNode.InnerText;
            VersionCheckStatus status = CompareSoftwareVersions(serverVersionText);
            newVersionText = serverVersionText;

            if (serverVersionNode.Attributes["Name"] != null)
            {
                newVersionText = serverVersionNode.Attributes["Name"].Value;
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

        private enum VersionCheckStatus
        {
            Equal,
            ServerNewer,
            ThisNewer
        }
    }
}
