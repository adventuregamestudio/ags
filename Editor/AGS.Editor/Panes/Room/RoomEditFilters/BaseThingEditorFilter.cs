using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// BaseThingEditorFilter is a parent filter for room layers, which manage
    /// individual "things": something that has exact coordinates, simple visual
    /// representation, and may be moved around.
    /// </summary>
    public abstract class BaseThingEditorFilter<TThing> : IRoomEditorFilter
    {
        private bool _isOn = false;
        protected Room _room;
        protected Panel _panel;
        protected RoomSettingsEditor _editor;
        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;

        public BaseThingEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _editor = editor;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
        }

        #region IDisposable implementation

        public void Dispose()
        {
        }

        #endregion // IDisposable

        #region IRoomEditorFilter implementation

        public abstract string Name { get; }
        public abstract string DisplayName { get; }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public abstract int ItemCount { get; }

        public int SelectedArea
        {
            get { return 0; } // not supported
        }

        public abstract string HelpKeyword { get; }

        public bool ShowTransparencySlider
        {
            get { return false; }
        }

        public bool SupportVisibleItems
        {
            get { return true; }
        }

        public bool Modified { get; set; }
        public bool Enabled { get { return _isOn; } }
        public bool Visible { get; set; }
        public bool Locked { get; set; }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; protected set; }

        public void Invalidate()
        {
            _panel.Invalidate();
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
            _isOn = true;
            FilterActivated();
        }

        public void FilterOff()
        {
            FilterDeactivated();
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            _isOn = false;
        }

        // Following IRoomEditorFilter members are left for descendants to implement
        public abstract void PaintToHDC(IntPtr hdc, RoomEditorState state);
        public abstract void Paint(Graphics graphics, RoomEditorState state);
        public abstract bool MouseDown(MouseEventArgs e, RoomEditorState state);
        public abstract bool MouseUp(MouseEventArgs e, RoomEditorState state);
        public abstract bool DoubleClick(RoomEditorState state);
        public abstract bool MouseMove(int x, int y, RoomEditorState state);
        public abstract void CommandClick(string command);
        public abstract bool KeyPressed(Keys keyData);
        public abstract bool KeyReleased(Keys keyData);
        public abstract string GetItemName(string id);
        public abstract void SelectItem(string id);
        public abstract Cursor GetCursor(int x, int y, RoomEditorState state);
        public abstract bool AllowClicksInterception();

        public abstract event EventHandler OnItemsChanged;
        public abstract event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public abstract event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

        #endregion // IRoomEditorFilter

        protected virtual void SetPropertyGridList()
        {
        }

        protected virtual void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
        }

        protected virtual void FilterActivated()
        {
        }

        protected virtual void FilterDeactivated()
        {
        }
    }
}
