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
            this.propertyGrid.PropertyValueChanged +=
                new System.Windows.Forms.PropertyValueChangedEventHandler(this.gameSettings_PropertyValueChanged);
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

        protected override string OnGetHelpKeyword()
        {
            return "Game options";
        }

        private void DeleteAllCompiledTranslations()
        {
            foreach (Translation translation in Factory.AGSEditor.CurrentGame.Translations)
            {
                string compiledPath = Path.Combine(AGSEditor.OUTPUT_DIRECTORY,
                    Path.Combine(AGSEditor.DATA_OUTPUT_DIRECTORY, translation.CompiledFileName));
                if (File.Exists(compiledPath))
                {
                    File.Delete(compiledPath);
                }
            }
        }

        private string GetEnumValueDescription<T>(T enumValue)
        {
            foreach (System.Reflection.FieldInfo fieldInfo in typeof(T).GetFields())
            {
                if (fieldInfo.Name == Enum.GetName(typeof(T), enumValue))
                {
                    object[] attributes = fieldInfo.GetCustomAttributes(typeof(DescriptionAttribute), true);
                    if (attributes.Length > 0)
                    {
                        return ((DescriptionAttribute)attributes[0]).Description;
                    }
                }
            }

            return Enum.GetName(typeof(T), enumValue);
        }

        private void ResizeAllGUIs(Size oldResolution, Size newResolution)
        {
            int oldWidth = oldResolution.Width;
            int oldHeight = oldResolution.Height;
            int newWidth = newResolution.Width;
            int newHeight = newResolution.Height;

            foreach (GUI gui in Factory.AGSEditor.CurrentGame.RootGUIFolder.AllItemsFlat)
            {
                NormalGUI theGui = gui as NormalGUI;
                if (theGui != null)
                {
                    theGui.Width = Math.Max((theGui.Width * newWidth) / oldWidth, 1);
                    theGui.Height = Math.Max((theGui.Height * newHeight) / oldHeight, 1);
                    theGui.Left = (theGui.Left * newWidth) / oldWidth;
                    theGui.Top = (theGui.Top * newHeight) / oldHeight;

                    foreach (GUIControl control in theGui.Controls)
                    {
                        control.Width = Math.Max((control.Width * newWidth) / oldWidth, 1);
                        control.Height = Math.Max((control.Height * newHeight) / oldHeight, 1);
                        control.Left = (control.Left * newWidth) / oldWidth;
                        control.Top = (control.Top * newHeight) / oldHeight;
                    }
                }
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
                ResizeAllGUIs(oldResolution, newResolution);
            }
            Factory.Events.OnGameSettingsChanged();
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
            else if ((e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_SCALE_FONTS) ||
                     (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_ANTI_ALIAS_FONTS) ||
                     (e.ChangedItem.Label == AGS.Types.Settings.PROPERTY_RENDERATSCREENRES))
            {
                Factory.Events.OnGameSettingsChanged();
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("general-settings/background");
            ForeColor = t.GetColor("general-settings/foreground");
            propertyGrid.BackColor = t.GetColor("general-settings/property-grid/background");
            propertyGrid.LineColor = t.GetColor("general-settings/property-grid/line");
            propertyGrid.CategoryForeColor = t.GetColor("general-settings/property-grid/category-fore");
            propertyGrid.ViewBackColor = t.GetColor("general-settings/property-grid/view/background");
            propertyGrid.ViewForeColor = t.GetColor("general-settings/property-grid/view/foreground");
            propertyGrid.HelpBackColor = t.GetColor("general-settings/property-grid/help/background");
            propertyGrid.HelpForeColor = t.GetColor("general-settings/property-grid/help/foreground");
        }
    }
}
