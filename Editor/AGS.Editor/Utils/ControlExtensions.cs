using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace AGS.Editor
{
    public static class ControlExtensions
    {
        // Pinvoke:
        private const int TVM_SETEXTENDEDSTYLE = 0x1100 + 44;
        private const int TVM_GETEXTENDEDSTYLE = 0x1100 + 45;
        private const int TVS_EX_DOUBLEBUFFER = 0x0004;

        /// <summary>
        /// Enables double buffering so that the control does not flicker.
        /// 
        /// See <see href="https://stackoverflow.com/a/10364283/20494">this StackOverflow answer</see> for more details.
        /// </summary>
        /// <param name="control"></param>
        public static void EnableDoubleBuffering(this Control control)
        {
            SendMessage(control.Handle, TVM_SETEXTENDEDSTYLE, (IntPtr)TVS_EX_DOUBLEBUFFER, (IntPtr)TVS_EX_DOUBLEBUFFER);
        }
        
        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wp, IntPtr lp);
    }
}
