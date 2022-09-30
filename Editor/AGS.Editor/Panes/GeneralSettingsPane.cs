using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public class GeneralSettingsPane : BigPropertySheet
    {
        public GeneralSettingsPane()
            : base(Factory.AGSEditor.CurrentGame.Settings)
        {
            InitializeComponent();
            this.propertyGrid.PropertyValueChanged +=
                new System.Windows.Forms.PropertyValueChangedEventHandler(this.gameSettings_PropertyValueChanged);
        }

        protected override string OnGetHelpKeyword()
        {
            return "General settings";
        }

        private void DeleteAllCompiledTranslations()
        {
            foreach (Translation translation in Factory.AGSEditor.CurrentGame.Translations)
            {
                string compiledPath = Path.Combine(AGSEditor.OUTPUT_DIRECTORY,
                    Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, translation.CompiledFileName));
                Utilities.TryDeleteFile(compiledPath);
            }
        }

        private void HandleGameResolutionChange(Size oldResolution, Size newResolution)
        {
            if (newResolution == oldResolution)
            {
                return;
            }

            string oldResolutionText = Types.Utilities.ResolutionToUserString(oldResolution);
            string newResolutionText = Types.Utilities.ResolutionToUserString(newResolution);
            if (Factory.GUIController.ShowQuestion(string.Format("You've changed your game resolution from '{0}' to '{1}'.{2}You will need to import a new background of the correct size for all your rooms.{2}{2}Would you like AGS to automatically resize all your GUIs to the new resolution?", oldResolutionText, newResolutionText, Environment.NewLine)) == DialogResult.Yes)
            {
                Factory.AGSEditor.Tasks.ResizeAllGUIs(oldResolution, newResolution);
            }
            Factory.Events.OnGameSettingsChanged();
        }

        private void HandleGameTextFormatChange(string oldFormat, string newFormat)
        {
            if (oldFormat == newFormat)
                return;
            if (Factory.GUIController.ShowQuestion("Changing the game text format will make the editor and engine treat all the text in all the game files in accordance to the new setting.\n\n"
                + "IMPORTANT: the Editor will now convert the game files and scripts to a new format. This may take a while, depending on your game's size.\n\n"
                + "IMPORTANT: the Translation files will remain unaffected by this setting, as they have their own individual encoding setting.\n\n"
                + "Are you sure you want to continue?",
                MessageBoxIcon.Warning) == DialogResult.No)
            {
                Factory.AGSEditor.CurrentGame.Settings.GameTextEncoding = oldFormat;
            }
            else
            {
                Factory.Events.OnGameSettingsChanged();
                BusyDialog.Show("Please wait while we convert game files to the new text format...",
                    (IWorkProgress progress, object o) => {
                        Factory.AGSEditor.Tasks.ConvertAllGameTexts(
                            Types.Utilities.EncodingFromName(oldFormat),
                            Types.Utilities.EncodingFromName(newFormat));
                        return null;
                    }, null);
            }
        }

        private void gameSettings_PropertyValueChanged(object s, PropertyValueChangedEventArgs e)
        {
            // TODO: I find it kind of wrong that general settings pane class decides which
            // properties need to call OnGameSettingsChanged, and which do not need. Perhaps
            // call it all the time?
            // TODO: find a better solution rather than detecting property by the title of the label
            if (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_GAME_NAME)
            {
                Factory.AGSEditor.CurrentGame.Settings.SaveGameFolderName = Factory.AGSEditor.CurrentGame.Settings.GameName;
                Factory.GUIController.GameNameUpdated();

                DeleteAllCompiledTranslations();
            }
            else if (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_COLOUR_DEPTH)
            {
                if (Factory.GUIController.ShowQuestion("Changing the game colour depth can invalidate your existing sprites and room backgrounds. Are you sure you want to continue?") == DialogResult.No)
                {
                    Factory.AGSEditor.CurrentGame.Settings.ColorDepth = (GameColorDepth)e.OldValue;
                }
                else
                {
                    Factory.Events.OnGameSettingsChanged();
                }
            }
            else if (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_RESOLUTION)
            {
                HandleGameResolutionChange((Size)e.OldValue, Factory.AGSEditor.CurrentGame.Settings.CustomResolution);
            }
            else if ((e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_TEXT_FORMAT))
            {
                HandleGameTextFormatChange((string)e.OldValue, Factory.AGSEditor.CurrentGame.Settings.GameTextEncoding);
            }
            else if ((e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_ANTI_ALIAS_FONTS) ||
                     (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_FONT_HEIGHT_IN_LOGIC) ||
                     (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_CLIPGUICONTROLS) ||
                     (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_RENDERATSCREENRES))
            {
                Factory.Events.OnGameSettingsChanged();
            }
            else if (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_DIALOG_SCRIPT_SAYFN ||
                e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_DIALOG_SCRIPT_NARRATEFN)
            {
                // Force rebuild all dialog scripts
                foreach (Dialog d in Factory.AGSEditor.CurrentGame.Dialogs)
                    d.ScriptChangedSinceLastConverted = true;
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "general-settings");
            t.PropertyGridHelper(propertyGrid, "general-settings/property-grid");
        }

        private void InitializeComponent()
        {
            this.SuspendLayout();
            // 
            // GeneralSettingsPane
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 14F);
            this.Name = "GeneralSettingsPane";
            this.Load += new System.EventHandler(this.GeneralSettingsPane_Load);
            this.ResumeLayout(false);
        }

        private void GeneralSettingsPane_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
