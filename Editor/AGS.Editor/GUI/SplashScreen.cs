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
                splashPage.ConstructSpecial("splash-valentine-2.jpg", ImageLayout.None, "Happy Valentine's Day!", Color.Yellow);
            }
            else if ((DateTime.Now.Month == 12) && (DateTime.Now.Day == 25))
            {
                splashPage.ConstructSpecial("splash-xmas.jpg", ImageLayout.None, "Happy Christmas!", Color.LightGreen);
            }
            else
            {
                int tickCount = Math.Abs(Environment.TickCount);
                splashPage.ConstructFull(TagLines[(tickCount / 1000) % TagLines.Length]);
            }

            Size = splashPage.Size;
        }
    }
}