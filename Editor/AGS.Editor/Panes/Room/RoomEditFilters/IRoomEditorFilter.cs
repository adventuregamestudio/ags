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
        string DisplayName { get; }
        RoomAreaMaskType MaskToDraw { get; }
        int ItemCount { get; }
        int SelectedArea { get; }
        string HelpKeyword { get; }
        bool ShowTransparencySlider { get; }
        bool VisibleByDefault { get; }
        bool SupportVisibleItems { get; }

        /// <summary>
        /// The dictionary that maps an object ID to its design-time properties.
        /// </summary>
        Dictionary<string, DesignTimeProperties> DesignItems { get; }
        void PaintToHDC(IntPtr hDC, RoomEditorState state);
        void Paint(Graphics graphics, RoomEditorState state);
        void MouseDownAlways(MouseEventArgs e, RoomEditorState state);
        bool MouseDown(MouseEventArgs e, RoomEditorState state);
        bool MouseUp(MouseEventArgs e, RoomEditorState state);
        bool DoubleClick(RoomEditorState state);
        bool MouseMove(int x, int y, RoomEditorState state);
        void FilterOn();
        void FilterOff();
        void CommandClick(string command);
        bool KeyPressed(Keys keyData);
        List<string> GetItemsNames();
        void SelectItem(string name);
        void Invalidate();
        Cursor GetCursor(int x, int y, RoomEditorState state);
        bool AllowClicksInterception();

        event EventHandler OnItemsChanged;
        event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
    }
}
