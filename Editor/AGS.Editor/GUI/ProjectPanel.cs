﻿using System;
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
            this.LoadColorTheme();
        }

        private void LoadColorTheme()
        {
            ColorTheme colorTheme = Factory.GUIController.UserColorTheme;
            colorTheme.Color_TreeView(this.projectTree);
        }
    }
}
