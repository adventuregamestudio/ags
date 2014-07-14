using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class SplashScreen : Form
    {
        private readonly string[] TagLines = new string[] {
            "Our clocks stopped in '91",
            "Because quality doesn't have a price",
            "If Carlsberg made adventures, they'd probably use AGS",
            "Larry Vales needs YOU",
            "Rebuilding the past since 1997",
            "Sierra and Lucasarts' lovechild",
            "Adventure developers do it with an IDE",
            "A new paradigm in interactive multimedia",
            "We point, you click",
            "Yesterday's technology, today!",
            "Indulge your EGO",
            "\"800x600 ought to be enough for anybody\"",
            "Party like it's 1991!",
            "You can't spell handbags without AGS",
            "Objects in inventory are lighter than they appear",
            "In association with Monolith Burger",
            "Best served with nostalgia",
            "Please do not feed the scripters",
            "This area intentionally left blank",
            "More fun than the girl next door",
            "Insert Disk 2 to continue",
            "The partner that keeps you up all night",
            "Can help weight loss only as part of a balanced diet",
            "Something your mother would be proud of",
            "Our woodchuck can chuck wood",
            "Blue cup not included",
            "For best results, use in your parents' basement",
            "Winner: Best AGS of the year, 2007",
            "A lower carbon footprint than Wintermute",
			"In stereo (where available)",
			"May contain traces of nuts",
			"Made with 100% recycled pixels",
			"In Soviet Russia, game makes you",
			"Because you'll never be Ron Gilbert",
			"The practical alternative to Warcraft",
			"Always in your inventory",
			"Advertise here; competitive rates available",
			"Procrastination personified",
			"Rated 12-A (contains scenes of an amateur nature)",
			"Qué haces será",
            "Now available on the NHS",
            "Satisfaction guaranteed, or your money back",
            "Chuck Norris made it happen",
            "\"Scary, but exciting!\"" // quoting Remigiusz Michalski :)
        };
        /* Never quite finished implementing this april fool
        private readonly string[] TagLinesChristian = new string[] {
            "Thou shalt not commit adultery"
        };
        private readonly string[] TagLinesCatholic = new string[] {
            "Thou shalt not commit adultery"
        };*/


        public SplashScreen()
        {
            InitializeComponent();
            if ((DateTime.Now.Month == 2) && (DateTime.Now.Day == 14))
            {
                this.BackgroundImage = Resources.ResourceManager.GetBitmap("splash-valentine-2.jpg");
                lblTagLine.Text = "Happy Valentine's Day!";
                lblTagLine.ForeColor = Color.Yellow;
            }
            else if ((DateTime.Now.Month == 12) && (DateTime.Now.Day == 25))
            {
                this.BackgroundImage = Resources.ResourceManager.GetBitmap("splash-xmas.jpg");
                lblTagLine.Text = "Happy Christmas!";
                lblTagLine.ForeColor = Color.LightGreen;
            }
            else
            {
                this.BackgroundImage = Resources.ResourceManager.GetBitmap("splash.bmp");
                int tickCount = Math.Abs(Environment.TickCount);
                lblTagLine.Text = TagLines[(tickCount / 1000) % TagLines.Length];
            }
        }

        private void SplashScreen_Load(object sender, EventArgs e)
        {
            // For some reason the automatic .NET DPI-stretching doesn't
            // resize this form itself, so do it manually instead
            Graphics graphics = this.CreateGraphics();
            float proportion = (float)(graphics.DpiY / 96.0);
            graphics.Dispose();

            this.Size = new Size((int)(this.Width * proportion), (int)(this.Height * proportion));
            this.Left -= (int)(this.Left * proportion) - this.Left;
            this.Top -= (int)(this.Top * proportion) - this.Top;
        }

        private void SplashScreen_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.DrawRectangle(Pens.Black, 0, 0, this.ClientSize.Width - 1, this.ClientSize.Height - 1);
        }
    }
}