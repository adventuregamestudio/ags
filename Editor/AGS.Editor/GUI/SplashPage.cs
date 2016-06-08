using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// This control implements the AGS splash, based on original design
    /// by abstauber. The splash may be constructed in one of the several
    /// modes, each of which display different number of elements.
    /// </summary>
    public partial class SplashPage : UserControl
    {
        // Defines the contents of the splash page
        public enum SplashLooks
        {
            Full, // display logo, lblTitle, version and taglines
            Simple, // display logo, lblTitle and version
            Special // display custom image with bottom tagline
        };

        public SplashLooks GeneralLooks
        {
            get;
            private set;
        }

        // Internal splash parameters
        private static class Design
        {
            // The list of font families to choose from, in the order of priority
            public static string[] FontNames = new string[] { "Segoe UI Light", "Segoe UI", "Tahoma" };
            // The chosen font name
            public static string FontName;
            // Font sizes of various textual elements
            public const float VersionFontPt = 28.0F;
            public const float TitleFontPt = 24.0F;
            public const float TagLineFontPt = 12.0F;
            public const float BuildFontPt = 9.0F;

            static Design()
            {
                FontFamily fontfam = Utilities.FindExistingFont(Design.FontNames);
                FontName = fontfam.Name;
            }
        }

        public void ConstructFull(string tagLine)
        {
            GeneralLooks = SplashLooks.Full;
            ArrangeLogo();
            ArrangeTagLine(tagLine);
            ArrangeBuildInfo();
        }

        public void ConstructSimple()
        {
            GeneralLooks = SplashLooks.Simple;
            lblSeparator.Visible = false;
            lblTagLine.Visible = false;
            lblBottomTagLine.Visible = false;
            ArrangeLogo();
        }

        public void ConstructSpecial(string bitmapResName, ImageLayout bitmapLayout, string tagLine, Color tagColor)
        {
            GeneralLooks = SplashLooks.Special;
            lblMainVersion.Visible = false;
            picCup.Visible = false;
            lblTitle.Visible = false;
            lblSeparator.Visible = false;
            lblTagLine.Visible = false;
            ArrangeSpecial(bitmapResName, bitmapLayout, tagLine, tagColor);
        }

        private void ArrangeLogo()
        {
            lblMainVersion.Visible = true;
            lblMainVersion.Font = new Font(Design.FontName, Design.VersionFontPt, FontStyle.Bold);
            lblMainVersion.Text = AGS.Types.Version.AGS_EDITOR_FRIENDLY_VERSION;
            // Have main version frame be located 2 pixel away from the top-right control border
            lblMainVersion.Left = this.Width - lblMainVersion.Width - this.Padding.Right;
            lblMainVersion.Top = this.Padding.Top;

            picCup.Visible = true;
            picCup.Image = Resources.ResourceManager.GetBitmap("ags_cup.png");
            picCup.Left = (this.Width - picCup.Width) / 2;
            // In the Full mode Cup's bottom is aligned to the vertical center,
            // this way the middle line of the splash goes right between
            // the Cup and the Title.
            if (GeneralLooks == SplashLooks.Full)
                picCup.Top = this.Height / 2 - picCup.Height;
            // In the simple mode, Cup is centered on the form,
            // thus the Title is seen distinctively in the lower part
            // of the splash.
            else
                picCup.Top = this.Height / 2 - picCup.Height / 2;

            lblTitle.Visible = true;
            lblTitle.Font = new Font(Design.FontName, Design.TitleFontPt);
            lblTitle.Left = (this.Width - lblTitle.Width) / 2;
            lblTitle.Top = picCup.Bottom + picCup.Height / 8;
        }

        private void ArrangeTagLine(string tagLine)
        {
            lblSeparator.Visible = true;
            lblSeparator.Width = lblTitle.Width;
            lblSeparator.Left = (this.Width - lblSeparator.Width) / 2;
            lblSeparator.Top = lblTitle.Bottom + lblTitle.Height / 4;

            lblTagLine.Visible = true;
            lblTagLine.Font = new Font(Design.FontName, Design.TagLineFontPt, FontStyle.Bold);
            lblTagLine.Text = tagLine;
            lblTagLine.Left = (this.Width - lblTagLine.Width) / 2;
            lblTagLine.Top = lblSeparator.Bottom + lblTagLine.Height / 4;
        }

        private void ArrangeBuildInfo()
        {
            lblBottomTagLine.Visible = true;
            lblBottomTagLine.Font = new Font(Design.FontName, Design.BuildFontPt, FontStyle.Bold);
            lblBottomTagLine.Text = "Build " + AGS.Types.Version.AGS_EDITOR_VERSION + ", " + AGS.Types.Version.AGS_EDITOR_DATE;
            lblBottomTagLine.TextAlign = ContentAlignment.MiddleLeft;
        }

        private void ArrangeSpecial(string bitmapResName, ImageLayout bitmapLayout, string tagLine, Color tagColor)
        {
            BackgroundImage = Resources.ResourceManager.GetBitmap(bitmapResName);
            if (bitmapLayout == ImageLayout.None)
            {
                // Changing control size to the constant bitmap size here,
                // we have to apply proportions factor based on system font scaling
                float proportion = Utilities.CalculateGraphicsProportion(this);
                Size = new Size((int)((float)BackgroundImage.Size.Width * proportion),
                    (int)((float)BackgroundImage.Size.Height * proportion));
                BackgroundImageLayout = ImageLayout.Stretch;
            }
            else
            {
                BackgroundImageLayout = bitmapLayout;
            }
            lblBottomTagLine.Font = new Font("Tahoma", 9.75F, System.Drawing.FontStyle.Bold);
            lblBottomTagLine.ForeColor = tagColor;
            lblBottomTagLine.Text = tagLine;
            lblBottomTagLine.TextAlign = ContentAlignment.MiddleRight;
        }

        public SplashPage()
        {
            InitializeComponent();
            ConstructFull("Sample tagline text");
        }
    }
}
