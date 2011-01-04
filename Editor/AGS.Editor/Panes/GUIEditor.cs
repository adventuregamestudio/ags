using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor
{
    public partial class GUIEditor : EditorContentPanel
    {
        public delegate void ControlsChanged(GUI editingGui);
        public event ControlsChanged OnControlsChanged;
        public delegate void GuiNameChanged();
        public event GuiNameChanged OnGuiNameChanged;

        private GUI _gui = null;
        private ImageList _imageList = new ImageList();
        private bool _addingControl = false;
        private bool _movingControl = false;
        private bool _resizingControl = false;
        private bool _inResizingArea = false;
        private int _addingControlX, _addingControlY;
        private int _currentMouseX, _currentMouseY;
        private int _mouseXOffset, _mouseYOffset;
        private Pen _drawRectanglePen = new Pen(Color.Yellow, 2);
        private GUIControl _selectedControl = null;
        private GUIAddType _controlAddMode = GUIAddType.None;
        private List<MenuCommand> _toolbarIcons;

        public GUIEditor(GUI guiToEdit, List<MenuCommand> toolbarIcons) : this() 
        {
            _gui = guiToEdit;
            _toolbarIcons = toolbarIcons;
        }

        public GUIEditor()
        {
            InitializeComponent();

            Factory.GUIController.OnPropertyObjectChanged += new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
        }

        void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is GUIControl)
            {
                _selectedControl = (GUIControl)newPropertyObject;
            }
            else if (newPropertyObject is GUI)
            {
                DeSelectControl();
            }
            bgPanel.Invalidate();
        }

        protected override string OnGetHelpKeyword()
        {
            return "GUI";
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                object objectBeingChanged = _gui;
                string newName = _gui.Name;
                if (_selectedControl != null)
                {
                    objectBeingChanged = _selectedControl;
                    newName = _selectedControl.Name;
                }
                Game game = Factory.AGSEditor.CurrentGame;
                bool nameInUse = game.IsScriptNameAlreadyUsed(newName, objectBeingChanged);
                if (newName.StartsWith("g") && newName.Length > 1)
                {
                    nameInUse |= game.IsScriptNameAlreadyUsed(newName.Substring(1).ToUpper(), objectBeingChanged);
                }

                if (nameInUse)
                {
                    Factory.GUIController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    if (_selectedControl != null)
                    {
                        _selectedControl.Name = (string)oldValue;
                    }
                    else
                    {
                        _gui.Name = (string)oldValue;
                    }
                }
                else if (_selectedControl == null)
                {
                    RaiseOnGuiNameChanged();
                }
            }
			else if (propertyName == "Image")
			{
				if ((_selectedControl != null) && (_selectedControl is GUIButton))
				{
					GUIButton selectedButton = (GUIButton)_selectedControl;
					if (selectedButton.Image > 0)
					{
                        int newWidth, newHeight;
                        Utilities.GetSizeSpriteWillBeRenderedInGame(selectedButton.Image, out newWidth, out newHeight);
                        selectedButton.Width = newWidth;
                        selectedButton.Height = newHeight;
					}
				}
			}
        }

        public GUI GuiToEdit
        {
            get { return _gui; }
        }

        private void GetSelectionRectangle(out int left, out int top, out int width, out int height)
        {
            left = _addingControlX;
            top = _addingControlY;
            width = _currentMouseX - _addingControlX;
            height = _currentMouseY - _addingControlY;
            if (_currentMouseX < _addingControlX)
            {
                left = _currentMouseX;
                width = -width;
            }
            if (_currentMouseY < _addingControlY)
            {
                top = _currentMouseY;
                height = -height;
            }
        }

        private void DrawSelectionRectangle(Graphics graphics)
        {
            int left, top, width, height;
            GetSelectionRectangle(out left, out top, out width, out height);
            graphics.DrawRectangle(_drawRectanglePen, left, top, width, height);
        }

        protected override void OnCommandClick(string command)
        {
            _toolbarIcons[(int)_controlAddMode].Checked = false;

            switch (command) 
            {
                case Components.GuiComponent.MODE_SELECT_CONTROLS:
                    _controlAddMode = GUIAddType.None;
                    break;
                case Components.GuiComponent.MODE_ADD_BUTTON:
                    _controlAddMode = GUIAddType.Button;
                    break;
                case Components.GuiComponent.MODE_ADD_INVENTORY:
                    _controlAddMode = GUIAddType.InvWindow;
                    break;
                case Components.GuiComponent.MODE_ADD_LABEL:
                    _controlAddMode = GUIAddType.Label;
                    break;
                case Components.GuiComponent.MODE_ADD_LISTBOX:
                    _controlAddMode = GUIAddType.ListBox;
                    break;
                case Components.GuiComponent.MODE_ADD_SLIDER:
                    _controlAddMode = GUIAddType.Slider;
                    break;
                case Components.GuiComponent.MODE_ADD_TEXTBOX:
                    _controlAddMode = GUIAddType.TextBox;
                    break;
            }

            _toolbarIcons[(int)_controlAddMode].Checked = true;
            Factory.ToolBarManager.RefreshCurrentPane();
        }

        private void bgPanel_Paint(object sender, PaintEventArgs e)
        {
            if (_gui != null)
            {
                IntPtr hdc = e.Graphics.GetHdc();
                Factory.NativeProxy.DrawGUI(hdc, 0, 0, _gui, Factory.AGSEditor.CurrentGame.GUIScaleFactor, (_selectedControl == null) ? -1 : _selectedControl.ID);
                e.Graphics.ReleaseHdc(hdc);
                
                if (_addingControl)
                {
                    DrawSelectionRectangle(e.Graphics);
                }
            }
            base.OnPaint(e);
        }

        private bool AboutToAddControl()
        {
            return (_controlAddMode > 0);
        }

        private void GetCoordinatesAtGameResolution(MouseEventArgs e, out int mouseX, out int mouseY)
        {
            mouseX = e.X / Factory.AGSEditor.CurrentGame.GUIScaleFactor;
            mouseY = e.Y / Factory.AGSEditor.CurrentGame.GUIScaleFactor;
        }

        private void bgPanel_MouseDown(object sender, MouseEventArgs e)
        {
			this.Focus();
            int mouseX, mouseY;
            GetCoordinatesAtGameResolution(e, out mouseX, out mouseY);

            if (_gui is TextWindowGUI)
            {
                SetSelectedControlToControlAtPosition(mouseX, mouseY);
            }
            else if (AboutToAddControl())
            {
				if (_gui.Controls.Count >= GUI.MAX_CONTROLS_PER_GUI)
				{
					Factory.GUIController.ShowMessage("You already have the maximum number of controls on this GUI, and cannot add any more.", MessageBoxIcon.Warning);
					return;
				}

                _addingControl = true;
                _addingControlX = e.X;
                _addingControlY = e.Y;
            }
            else if ((_inResizingArea) && (_selectedControl != null))
            {
                _resizingControl = true;
                _mouseXOffset = _selectedControl.Width - (mouseX - _selectedControl.Left);
                _mouseYOffset = _selectedControl.Height - (mouseY - _selectedControl.Top);
            }
            else
            {
                SetSelectedControlToControlAtPosition(mouseX, mouseY);
                if (_selectedControl != null)
                {
                    _movingControl = true;
                    _mouseXOffset = mouseX - _selectedControl.Left;
                    _mouseYOffset = mouseY - _selectedControl.Top;
                }
            }
        }

        private void MoveControlWithMouse(int mouseX, int mouseY)
        {
            NormalGUI normalGui = (NormalGUI)_gui;
            _selectedControl.Left = mouseX - _mouseXOffset;
            _selectedControl.Top = mouseY - _mouseYOffset;
            if (_selectedControl.Left >= normalGui.Width)
            {
                _selectedControl.Left = normalGui.Width - 1;
            }
            if (_selectedControl.Top >= normalGui.Height)
            {
                _selectedControl.Top = normalGui.Height - 1;
            }
            bgPanel.Invalidate();
        }

        private void bgPanel_MouseMove(object sender, MouseEventArgs e)
        {
            _inResizingArea = false;
            int mouseX, mouseY;
            GetCoordinatesAtGameResolution(e, out mouseX, out mouseY);
            _currentMouseX = e.X;
            _currentMouseY = e.Y;

            if (_movingControl)
            {
                MoveControlWithMouse(mouseX, mouseY);
            }
            else if (_addingControl)
            {
                bgPanel.Invalidate();
            }
            else if ((_resizingControl) && (_selectedControl != null))
            {
                try
                {
                    _selectedControl.Width = (mouseX - _selectedControl.Left) + _mouseXOffset;
                    _selectedControl.Height = (mouseY - _selectedControl.Top) + _mouseYOffset;
                }
                catch (ArgumentException) {}

                bgPanel.Invalidate();
            }
            else if (_selectedControl != null)
            {
                int bottomRightX = _selectedControl.Left + _selectedControl.Width;
                int bottomRightY = _selectedControl.Top + _selectedControl.Height;
                if ((mouseX >= bottomRightX - 2) &&
                    (mouseX <= bottomRightX + 2) &&
                    (mouseY >= bottomRightY - 2) &&
                    (mouseY <= bottomRightY + 2))
                {
                    _inResizingArea = true;
                }
            }

            UpdateCursorImage();
        }

        private void SetSelectedControlToControlAtPosition(int mouseX, int mouseY)
        {
            GUIControl controlFound = null;
            int zorderFound = -1;
            foreach (GUIControl control in _gui.Controls)
            {
                if ((mouseX >= control.Left) && (mouseX < control.Left + control.Width) &&
                    (mouseY >= control.Top) && (mouseY < control.Top + control.Height) &&
                    (control.ZOrder > zorderFound))
                {
                    controlFound = control;
                    zorderFound = control.ZOrder;
                }
            }

            _selectedControl = controlFound;
            if (_selectedControl == null)
            {
                DeSelectControl();
            }
        }

        private void SendToBackClick(object sender, EventArgs e)
        {
            _gui.SendControlToBack(_selectedControl);
            bgPanel.Invalidate();
        }

        private void BringToFrontClick(object sender, EventArgs e)
        {
            _gui.BringControlToFront(_selectedControl);
            bgPanel.Invalidate();
        }

        private void DeSelectControl()
        {
            _selectedControl = null;
            _movingControl = false;
            _resizingControl = false;
        }

        private void DeleteControlClick(object sender, EventArgs e)
        {
            if (_selectedControl != null)
            {
                if (Factory.GUIController.ShowQuestion("Are you sure you want to delete this control?") == DialogResult.Yes)
                {
                    _gui.DeleteControl(_selectedControl);
                    DeSelectControl();
                    Factory.GUIController.SetPropertyGridObject(_gui);
                    RaiseOnControlsChanged();
                }
            }
        }

        private void ShowContextMenu(int x, int y, GUIControl control)
        {
            if (_gui is NormalGUI)
            {
                ContextMenuStrip menu = new ContextMenuStrip();
                menu.Items.Add(new ToolStripMenuItem("Bring to Front", null, new EventHandler(BringToFrontClick), "BringToFront"));
                menu.Items.Add(new ToolStripMenuItem("Send to Back", null, new EventHandler(SendToBackClick), "SendToBack"));
                menu.Items.Add(new ToolStripSeparator());
                menu.Items.Add(new ToolStripMenuItem("Delete", null, new EventHandler(DeleteControlClick), Keys.Delete));

                menu.Show(bgPanel, x, y);
            }
        }

        private void bgPanel_MouseUp(object sender, MouseEventArgs e)
        {
            if (_addingControl)
            {
                _addingControl = false;
                CreateNewControl();
            }
            else if (_resizingControl)
            {
                _resizingControl = false;
            }
            else
            {
                _movingControl = false;
                if (_selectedControl != null)
                {
                    Factory.GUIController.SetPropertyGridObject(_selectedControl);
                }
                else
                {
                    Factory.GUIController.SetPropertyGridObject(_gui);
                }
                bgPanel.Invalidate();

                if ((e.Button == MouseButtons.Right) && (_selectedControl != null))
                {
                    ShowContextMenu(e.X, e.Y, _selectedControl);
                }
            }
        }

        private void bgPanel_MouseLeave(object sender, EventArgs e)
        {
            if (_addingControl)
            {
                _addingControl = false;
                bgPanel.Invalidate();
            }
        }

        private void bgPanel_MouseEnter(object sender, EventArgs e)
        {
            UpdateCursorImage();
        }

        private void UpdateCursorImage()
        {
            if (AboutToAddControl())
            {
                bgPanel.Cursor = Cursors.Cross;
            }
            else if ((_inResizingArea) || (_resizingControl))
            {
                bgPanel.Cursor = Cursors.SizeNWSE;
            }
            else 
            {
                bgPanel.Cursor = Cursors.Default;
            }
        }

        private void ConvertCoordinatesToGameUnits(ref int left, ref int top, ref int width, ref int height)
        {
            int guiScaleFactor = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
            left /= guiScaleFactor;
            top /= guiScaleFactor;
            width /= guiScaleFactor;
            height /= guiScaleFactor;
        }

        private void CreateNewControl()
        {
            int left, top, width, height;
            GetSelectionRectangle(out left, out top, out width, out height);
            ConvertCoordinatesToGameUnits(ref left, ref top, ref width, ref height);

            if ((width < 2) || (height < 2))
            {
                return;
            }

            GUIControl newControl = null;

            switch (_controlAddMode)
            {
                case GUIAddType.Button:
                    newControl = new GUIButton(left, top, width, height);
                    break;
                case GUIAddType.Label:
                    newControl = new GUILabel(left, top, width, height);
                    break;
                case GUIAddType.TextBox:
                    newControl = new GUITextBox(left, top, width, height);
                    break;
                case GUIAddType.ListBox:
                    newControl = new GUIListBox(left, top, width, height);
                    break;
                case GUIAddType.Slider:
                    newControl = new GUISlider(left, top, width, height);
                    break;
                case GUIAddType.InvWindow:
                    newControl = new GUIInventory(left, top, width, height);
                    break;
                default:
                    throw new AGSEditorException("Unknown control type added: " + _controlAddMode.ToString());
            }

            newControl.Name = Factory.AGSEditor.GetFirstAvailableScriptName(newControl.ControlType);
            newControl.ZOrder = _gui.Controls.Count;
            newControl.ID = _gui.Controls.Count;
            _gui.Controls.Add(newControl);
            _selectedControl = newControl;

            RaiseOnControlsChanged();

            Factory.GUIController.SetPropertyGridObject(newControl);

            bgPanel.Invalidate();
            UpdateCursorImage();
            // Revert back to Select cursor
            OnCommandClick(Components.GuiComponent.MODE_SELECT_CONTROLS);
        }

        private void RaiseOnControlsChanged()
        {
            if (OnControlsChanged != null)
            {
                OnControlsChanged(_gui);
            }
        }

        private void RaiseOnGuiNameChanged()
        {
            if (OnGuiNameChanged != null)
            {
                OnGuiNameChanged();
            }
        }

        protected override void OnDispose()
        {
            Factory.GUIController.OnPropertyObjectChanged -= new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
        }

        protected override void OnKeyPressed(Keys keyData)
        {
            if ((keyData == Keys.Delete) && (this.Focused))
            {
                DeleteControlClick(null, null);
            }
        }

        private void bgPanel_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            object objectToCheck = _gui;
            if (_selectedControl != null)
            {
                objectToCheck = _selectedControl;
            }

            foreach (PropertyInfo property in objectToCheck.GetType().GetProperties())
            {
                if (property.GetCustomAttributes(typeof(AGSEventPropertyAttribute), true).Length > 0)
                {
                    string eventHandler = (string)property.GetValue(objectToCheck, null);
					if (eventHandler.Length > 0)
					{
						Factory.GUIController.ZoomToFile(Script.GLOBAL_SCRIPT_FILE_NAME, eventHandler);
					}
					else 
					{
						CreateScriptFunctionForGUIItem(eventHandler, objectToCheck, property);
					}
                    break;
                }
            }
        }

		private void CreateScriptFunctionForGUIItem(string eventHandler, object objectToCheck, PropertyInfo property)
		{
			string itemName;
			int maxLength;
			if (objectToCheck is GUI)
			{
				itemName = ((GUI)objectToCheck).Name;
				maxLength = NormalGUI.MAX_EVENT_HANDLER_LENGTH;
			}
			else
			{
				itemName = ((GUIControl)objectToCheck).Name;
				maxLength = GUIControl.MAX_EVENT_HANDLER_LENGTH;
			}

			if (string.IsNullOrEmpty(itemName))
			{
				Factory.GUIController.ShowMessage("You need to give this a name before you can create a script for it. Set the Name property in the properties grid to the right.", MessageBoxIcon.Warning);
				return;
			}

			object[] paramsAttribute = property.GetCustomAttributes(typeof(ScriptFunctionParametersAttribute), true);
			if (paramsAttribute.Length > 0)
			{
				property.SetValue(objectToCheck, ScriptFunctionUIEditor.CreateOrOpenScriptFunction(eventHandler, itemName, property.Name, (ScriptFunctionParametersAttribute)paramsAttribute[0], true, maxLength), null);
			}
		}

    }
}
