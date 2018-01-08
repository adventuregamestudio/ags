using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace AGS.Editor
{
    class DefaultRuntimeSetupPane : BigPropertySheet
    {
        public DefaultRuntimeSetupPane()
            : base(Factory.AGSEditor.CurrentGame.DefaultSetup)
        {
            Factory.GUIController.ColorThemes.Load(LoadColorTheme);
        }

        protected override string OnGetHelpKeyword()
        {
            return "Default setup";
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
