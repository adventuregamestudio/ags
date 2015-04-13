using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    [DefaultProperty("Baseline")]
    public class RoomWalkBehind
    {
        private int _id;
        private int _baseline;

        [Description("The ID number of the walk-behind area")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("Characters standing above this baseline will be drawn behind the walk-behind")]
        [Category("Design")]
        public int Baseline
        {
            get { return (_baseline < 0) ? 0 : _baseline; }
            set { _baseline = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return "Walk-behind area ID " + _id; }
        }

    }
}
