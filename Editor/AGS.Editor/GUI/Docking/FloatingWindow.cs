using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;
using System.Drawing;

namespace AGS.Editor
{
    public class FloatingWindow : IFloatWindow
    {
        private FloatWindow _floatWindow;

        public FloatingWindow(FloatWindow floatWindow)
        {
            _floatWindow = floatWindow;
        }

        public Point Location { get { return _floatWindow.Location; } }

        public Size Size { get { return _floatWindow.Size; } }
    }
}
