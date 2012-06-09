using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// ToolStripExtended extends ToolStrip and adds Click-Through
    /// functionality, based on this article:
    /// http://blogs.msdn.com/b/rickbrew/archive/2006/01/09/511003.aspx
    /// </summary>
    public class ToolStripExtended : ToolStrip
    {
        protected override void WndProc(ref Message m)
        {
            base.WndProc(ref m);

            if (m.Msg == NativeProxy.WM_MOUSEACTIVATE &&
                m.Result == (IntPtr)NativeProxy.MA_ACTIVATEANDEAT)
            {
                m.Result = (IntPtr)NativeProxy.MA_ACTIVATE;
            }
        }
    }
}
