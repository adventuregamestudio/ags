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
            InitializeComponent();
        }

        protected override string OnGetHelpKeyword()
        {
            return "Default Setup";
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

        private void InitializeComponent()
        {
            this.SuspendLayout();
            // 
            // DefaultRuntimeSetupPane
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 14F);
            this.Name = "DefaultRuntimeSetupPane";
            this.Load += new System.EventHandler(this.DefaultRuntimeSetupPane_Load);
            this.ResumeLayout(false);
        }

        private void DefaultRuntimeSetupPane_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
