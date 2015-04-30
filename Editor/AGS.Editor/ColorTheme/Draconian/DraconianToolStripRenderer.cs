﻿using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class DraconianToolStripRenderer : ToolStripSystemRenderer
    {
        public DraconianToolStripRenderer() : base() { }

        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e)
        {
            //Do nothing, we don't want a border.
        }
    }
}