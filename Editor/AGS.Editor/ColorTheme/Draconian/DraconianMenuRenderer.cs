using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class DraconianMenuRenderer : ToolStripProfessionalRenderer
    {
        public DraconianMenuRenderer(ProfessionalColorTable pct) : base(pct) { }

        protected override void OnRenderItemCheck(ToolStripItemImageRenderEventArgs e)
        {
            if (e.Item.Selected)
                e.Graphics.FillRectangle(new SolidBrush(Color.FromArgb(62, 62, 64)), new Rectangle(3, 1, 20, 20));
            else
                e.Graphics.FillRectangle(new SolidBrush(Color.FromArgb(45, 45, 48)), new Rectangle(3, 1, 20, 20));
            e.Graphics.DrawImage(this.InvertImage(e.Image), new Point(5, 3));
        }

        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
        {
            e.TextColor = Color.FromArgb(241, 241, 241);
            base.OnRenderItemText(e);
        }

        private Image InvertImage(Image image)
        {
            Bitmap result = new Bitmap(image);
            int A, R, G, B;
            for (int y = 0; y < result.Height; y++)
            {
                for (int x = 0; x < result.Width; x++)
                {
                    Color pixel = result.GetPixel(x, y);
                    A = pixel.A;
                    R = 255 - pixel.R;
                    G = 255 - pixel.G;
                    B = 255 - pixel.B;
                    result.SetPixel(x, y, Color.FromArgb(A, R, G, B));
                }
            }
            return result;
        }
    }
}