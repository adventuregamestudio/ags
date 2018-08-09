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
        /// <summary>
        /// Internal ID of the filter.
        /// </summary>
        string Name { get; }
        /// <summary>
        /// Displayed name of the filter.
        /// </summary>
        string DisplayName { get; }
        RoomAreaMaskType MaskToDraw { get; }
        int ItemCount { get; }
        int SelectedArea { get; }
        string HelpKeyword { get; }
        bool ShowTransparencySlider { get; }
        bool SupportVisibleItems { get; }

        /// <summary>
        /// Gets/sets if this layer is visible.
        /// </summary>
        bool Visible { get; set; }
        /// <summary>
        /// Gets/sets if this layer is locked (cannot be modified).
        /// </summary>
        bool Locked { get; set; }

        /// <summary>
        /// The dictionary that maps an object ID to its design-time properties.
        /// </summary>
        SortedDictionary<string, DesignTimeProperties> DesignItems { get; }
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
        /// <summary>
        /// Gets a human-readable area name.
        /// </summary>
        /// <param name="id"></param>
        string GetItemName(string id);
        /// <summary>
        /// Selects room item by its ID.
        /// </summary>
        /// <param name="id"></param>
        void SelectItem(string id);
        void Invalidate();
        Cursor GetCursor(int x, int y, RoomEditorState state);
        bool AllowClicksInterception();

        event EventHandler OnItemsChanged;
        event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
    }
}
