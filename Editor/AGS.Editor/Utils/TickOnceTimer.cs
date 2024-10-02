using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// TickOnceTimer is a Timer implementation which auto-disposes itself
    /// after the first Tick event. This class is meant for the single use
    /// "run and forget" timers.
    /// </summary>
    public class TickOnceTimer : Timer
    {
        public static TickOnceTimer CreateAndStart(int interval, EventHandler onTick)
        {
            TickOnceTimer timer = new TickOnceTimer();
            timer.Interval = interval;
            timer.Tick += onTick;
            timer.Start();
            return timer;
        }

        protected override void OnTick(EventArgs e)
        {
            Stop();
            base.OnTick(e);
            Dispose();
        }
    }
}
