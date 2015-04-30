﻿using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class DraconianMenuColors : ProfessionalColorTable
    {
        public override Color MenuStripGradientBegin { get { return Color.FromArgb(45, 45, 48); } }
        public override Color MenuStripGradientEnd { get { return Color.FromArgb(45, 45, 48); } }
        public override Color ToolStripDropDownBackground { get { return Color.FromArgb(27, 27, 28); } }
        public override Color ImageMarginGradientBegin { get { return Color.FromArgb(27, 27, 28); } }
        public override Color ImageMarginGradientMiddle { get { return Color.FromArgb(27, 27, 28); } }
        public override Color ImageMarginGradientEnd { get { return Color.FromArgb(27, 27, 28); } }
        public override Color ButtonSelectedBorder { get { return Color.FromArgb(41, 49, 52); } }
        public override Color CheckBackground { get { return Color.FromArgb(45, 45, 48); } }
        public override Color MenuItemPressedGradientBegin { get { return Color.FromArgb(27, 27, 28); } }
        public override Color MenuItemPressedGradientMiddle { get { return Color.FromArgb(27, 27, 28); } }
        public override Color MenuItemPressedGradientEnd { get { return Color.FromArgb(27, 27, 28); } }
        public override Color MenuItemBorder { get { return Color.FromArgb(51, 51, 52); } }
        public override Color MenuItemSelected { get { return Color.FromArgb(51, 51, 52); } }
        public override Color MenuItemSelectedGradientBegin { get { return Color.FromArgb(51, 51, 52); } }
        public override Color MenuItemSelectedGradientEnd { get { return Color.FromArgb(51, 51, 52); } }
        public override Color CheckSelectedBackground { get { return Color.FromArgb(62, 62, 64); } }
        public override Color MenuBorder { get { return Color.FromArgb(51, 51, 55); } }
        public override Color SeparatorDark { get { return Color.FromArgb(63, 63, 70); } }
        public override Color SeparatorLight { get { return Color.FromArgb(63, 63, 70); } }
    }
}