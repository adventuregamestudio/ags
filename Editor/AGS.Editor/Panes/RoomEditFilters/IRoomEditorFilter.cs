using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public interface IRoomEditorFilter : IDisposable
    {
        RoomAreaMaskType MaskToDraw { get; }
        int ItemCount { get; }
		int SelectedArea { get; }
		string HelpKeyword { get; }
		bool ShowTransparencySlider { get; }
        void PaintToHDC(IntPtr hDC, RoomEditorState state);
        void Paint(Graphics graphics, RoomEditorState state);
        void MouseDown(MouseEventArgs e, RoomEditorState state);
        void MouseUp(MouseEventArgs e, RoomEditorState state);
		void DoubleClick(RoomEditorState state);
        bool MouseMove(int x, int y, RoomEditorState state);
        void FilterOn();
        void FilterOff();
        void CommandClick(string command);
        bool KeyPressed(Keys keyData);
    }
}
