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
            t.ControlHelper(this, "general-settings");
            t.PropertyGridHelper(propertyGrid, "general-settings/property-grid");
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
