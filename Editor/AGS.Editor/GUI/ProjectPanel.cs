using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class ProjectPanel : DockContent
    {
        public ProjectPanel()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Load(LoadColorTheme);
        }

        public void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("project-panel/background");
            projectTree.BackColor = t.GetColor("project-panel/project-tree/background");
            projectTree.ForeColor = t.GetColor("project-panel/project-tree/foreground");
            projectTree.LineColor = t.GetColor("project-panel/project-tree/line");
        }
    }
}
