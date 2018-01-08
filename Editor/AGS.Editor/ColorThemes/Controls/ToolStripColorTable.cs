using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    internal class ToolStripColorTable : ProfessionalColorTable
    {
        public ToolStripColorTable(ColorTheme theme, string root)
        {
            ToolStripGradientBegin = theme.GetColor(root + "/gradient/begin");
            ToolStripGradientMiddle = theme.GetColor(root + "/gradient/middle");
            ToolStripGradientEnd = theme.GetColor(root + "/gradient/end");
            ToolStripBorder = theme.GetColor(root + "/border");
            SeparatorLight = theme.GetColor(root + "/separator");
            OverflowButtonGradientBegin = theme.GetColor(root + "/overflow-gradient/begin");
            OverflowButtonGradientMiddle = theme.GetColor(root + "/overflow-gradient/middle");
            OverflowButtonGradientEnd = theme.GetColor(root + "/overflow-gradient/end");
            ButtonSelectedGradientBegin = theme.GetColor(root + "/selected-gradient/begin");
            ButtonSelectedGradientEnd = theme.GetColor(root + "/selected-gradient/end");
            GripLight = theme.GetColor(root + "/grip/light");
            GripDark = theme.GetColor(root + "/grip/dark");
        }

        public override Color ToolStripGradientBegin { get; }
        public override Color ToolStripGradientMiddle { get; }
        public override Color ToolStripGradientEnd { get; }
        public override Color ToolStripBorder { get; }
        public override Color SeparatorLight { get; }
        public override Color OverflowButtonGradientBegin { get; }
        public override Color OverflowButtonGradientMiddle { get; }
        public override Color OverflowButtonGradientEnd { get; }
        public override Color ButtonSelectedGradientBegin { get; }
        public override Color ButtonSelectedGradientEnd { get; }
        public override Color GripLight { get; }
        public override Color GripDark { get; }
    }
}
