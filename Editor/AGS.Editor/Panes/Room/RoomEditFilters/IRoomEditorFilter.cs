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
        /// Gets/sets whether the design-time properties of the layer or items were modified.
        /// </summary>
        bool Modified { get; set; }
        /// <summary>
        /// Tells whether filter is currently on (enabled for editing)
        /// </summary>
        bool Enabled { get; }

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
        /// <summary>
        /// Paint filter contents using .NET functionality.
        /// </summary>
        /// <param name="graphics"></param>
        /// <param name="state"></param>
        void Paint(Graphics graphics, RoomEditorState state);
        /// <summary>
        /// Notifies mouse down event. Returns whether event is handled by this filter.
        /// </summary>
        bool MouseDown(MouseEventArgs e, RoomEditorState state);
        /// <summary>
        /// Notifies mouse up event. Returns whether event is handled by this filter.
        /// </summary>
        bool MouseUp(MouseEventArgs e, RoomEditorState state);
        /// <summary>
        /// Notifies double click event. Returns whether event is handled by this filter.
        /// </summary>
        bool DoubleClick(RoomEditorState state);
        /// <summary>
        /// Notifies mouse move event. Returns whether event is handled by this filter.
        /// </summary>
        bool MouseMove(int x, int y, RoomEditorState state);
        void FilterOn();
        void FilterOff();
        void CommandClick(string command);
        /// <summary>
        /// Notifies key press event. Returns whether event is handled by this filter.
        /// </summary>
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
