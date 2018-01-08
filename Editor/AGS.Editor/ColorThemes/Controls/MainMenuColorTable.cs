using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    internal class MainMenuColorTable : ProfessionalColorTable
    {
        public MainMenuColorTable(ColorTheme theme, string root)
        {
            MenuBorder = theme.GetColor(root + "/border");
            CheckBackground = theme.GetColor(root + "/check/background");
            ButtonSelectedBorder = theme.GetColor(root + "/check/border");
            ImageMarginGradientBegin = theme.GetColor(root + "/margin/gradient/begin");
            ImageMarginGradientMiddle = theme.GetColor(root + "/margin/gradient/middle");
            ImageMarginGradientEnd = theme.GetColor(root + "/margin/gradient/end");
            MenuItemBorder = theme.GetColor(root + "/item/border");
            ToolStripDropDownBackground = theme.GetColor(root + "/background-dropdown");
            MenuItemSelected = theme.GetColor(root + "/item/selected");
            MenuItemSelectedGradientBegin = theme.GetColor(root + "/selected/gradient/begin");
            MenuItemSelectedGradientEnd = theme.GetColor(root + "/selected/gradient/end");
            MenuItemPressedGradientBegin = theme.GetColor(root + "/pressed/gradient/begin");
            MenuItemPressedGradientMiddle = theme.GetColor(root + "/pressed/gradient/middle");
            MenuItemPressedGradientEnd = theme.GetColor(root + "/pressed/gradient/end");
            SeparatorDark = theme.GetColor(root + "/separator");
            CheckSelectedBackground = theme.GetColor(root + "/check/selected");
            CheckPressedBackground = theme.GetColor(root + "/check/pressed");
        }

        public override Color MenuBorder { get; }
        public override Color CheckBackground { get; }
        public override Color ButtonSelectedBorder { get; }
        public override Color ImageMarginGradientBegin { get; }
        public override Color ImageMarginGradientMiddle { get; }
        public override Color ImageMarginGradientEnd { get; }
        public override Color MenuItemBorder { get; }
        public override Color ToolStripDropDownBackground { get; }
        public override Color MenuItemSelected { get; }
        public override Color MenuItemSelectedGradientBegin { get; }
        public override Color MenuItemSelectedGradientEnd { get; }
        public override Color MenuItemPressedGradientBegin { get; }
        public override Color MenuItemPressedGradientMiddle { get; }
        public override Color MenuItemPressedGradientEnd { get; }
        public override Color SeparatorDark { get; }
        public override Color CheckSelectedBackground { get; }
        public override Color CheckPressedBackground { get; }
    } 
}
