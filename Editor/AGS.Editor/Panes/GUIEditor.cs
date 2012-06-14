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
        private bool _drawingSelectionBox = false;
        private int _selectionBoxX, _selectionBoxY;
        private Rectangle _selectionRect;
        private int _addingControlX, _addingControlY;
        private int _currentMouseX, _currentMouseY;
        private int _mouseXOffset, _mouseYOffset;
        
        private int _snappedx = -1;
        private int _snappedy = -1;
        private Pen _drawRectanglePen = new Pen(Color.Yellow, 2);
        private Pen _drawSelectedPen = new Pen(Color.Red, 2);
        private Pen _drawSnapPen = new Pen(Color.Yellow, 2);
        private bool fromCombo = true; // Hack to show how the the property object changed, need to change the delegate.
        private GUIControl _selectedControl = null;
        private GUIAddType _controlAddMode = GUIAddType.None;
        private List<MenuCommand> _toolbarIcons;
        private List<GUIControl> _selected;
        private List<GUIControlGroup> _groups;


        public GUIEditor(GUI guiToEdit, List<MenuCommand> toolbarIcons) : this() 
        {
            _gui = guiToEdit;
            _selected = new List<GUIControl>();
            _groups = new List<GUIControlGroup>();

            _toolbarIcons = toolbarIcons;
            
            PreviewKeyDown += new PreviewKeyDownEventHandler(GUIEditor_PreviewKeyDown);
            
            _drawSnapPen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
        }


        void GUIEditor_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
        {
            switch (e.KeyCode)
            {
                case Keys.Down:
                case Keys.Up:
                case Keys.Left:
                case Keys.Right:
                    e.IsInputKey = true;
                    break;
            }

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
                if (fromCombo)
                {
                    _selected.Clear();
                    _selected.Add((GUIControl)newPropertyObject);
                }
                fromCombo = true;
            }
            else if (newPropertyObject is GUI)
            {
                DeSelectControl();
                _selected.Clear();
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
                //Factory.NativeProxy.DrawGUI(hdc, 0, 0, _gui, Factory.AGSEditor.CurrentGame.GUIScaleFactor, (_selectedControl == null) ? -1 : _selectedControl.ID);
                Factory.NativeProxy.DrawGUI(hdc, 0, 0, _gui, Factory.AGSEditor.CurrentGame.GUIScaleFactor, -1);
                e.Graphics.ReleaseHdc(hdc);
                
                if (_addingControl)
                {
                    DrawSelectionRectangle(e.Graphics);
                }

                if (_drawingSelectionBox)
                {
                    Rectangle _rectToDraw = new Rectangle(_selectionRect.X * Factory.AGSEditor.CurrentGame.GUIScaleFactor,
                                                           _selectionRect.Y * Factory.AGSEditor.CurrentGame.GUIScaleFactor,
                                                           _selectionRect.Width * Factory.AGSEditor.CurrentGame.GUIScaleFactor,
                                                           _selectionRect.Height * Factory.AGSEditor.CurrentGame.GUIScaleFactor);
                    
                    e.Graphics.DrawRectangle(_drawRectanglePen, _rectToDraw);

                }
                //draw selection handles
                if (_selected.Count > 0)
                {
                    foreach (GUIControl _gc in _selected)
                    {
                        Rectangle _topleft = new Rectangle(_gc.Left * Factory.AGSEditor.CurrentGame.GUIScaleFactor, _gc.Top * Factory.AGSEditor.CurrentGame.GUIScaleFactor, 2, 2);
                        Rectangle _topright = new Rectangle((_gc.Left + _gc.Width - 1) * Factory.AGSEditor.CurrentGame.GUIScaleFactor, _gc.Top * Factory.AGSEditor.CurrentGame.GUIScaleFactor, 2, 2);
                        Rectangle _bottomleft = new Rectangle(_gc.Left * Factory.AGSEditor.CurrentGame.GUIScaleFactor, (_gc.Top + _gc.Height - 1) * Factory.AGSEditor.CurrentGame.GUIScaleFactor, 2, 2);
                        Rectangle _bottomright = new Rectangle((_gc.Left + _gc.Width - 1) * Factory.AGSEditor.CurrentGame.GUIScaleFactor, (_gc.Top + _gc.Height - 1) * Factory.AGSEditor.CurrentGame.GUIScaleFactor, 2, 2);
                        Pen _pen;
                        if (_gc == _selectedControl) _pen = _drawSelectedPen;
                        else _pen = _drawRectanglePen;
                        e.Graphics.DrawRectangle(_pen, _topleft);
                        e.Graphics.DrawRectangle(_pen, _topright);
                        e.Graphics.DrawRectangle(_pen, _bottomleft);
                        e.Graphics.DrawRectangle(_pen, _bottomright);

                        //draw cross if locked
                        if (_gc.Locked)
                        {
                            Point center = new Point(_gc.Left + (_gc.Width / 2), _gc.Top + (_gc.Height / 2));
                            center.X *= Factory.AGSEditor.CurrentGame.GUIScaleFactor;
                            center.Y *= Factory.AGSEditor.CurrentGame.GUIScaleFactor;

                            e.Graphics.DrawLine(_pen, center.X - 3, center.Y - 3, center.X + 3, center.Y + 3);
                            e.Graphics.DrawLine(_pen, center.X - 3, center.Y + 3, center.X + 3, center.Y - 3);
                            
                        }

                    }
                }

                if (_snappedx != -1)
                {
                    NormalGUI g = (NormalGUI)_gui;
                    e.Graphics.DrawLine(_drawSnapPen, _snappedx * Factory.AGSEditor.CurrentGame.GUIScaleFactor, 0, _snappedx * Factory.AGSEditor.CurrentGame.GUIScaleFactor, g.Height * Factory.AGSEditor.CurrentGame.GUIScaleFactor);
                    string snapxstring = String.Format("{0}px", _snappedx);

                    e.Graphics.DrawString(snapxstring,
                        DefaultFont,
                        _drawSnapPen.Brush,
                        _snappedx * Factory.AGSEditor.CurrentGame.GUIScaleFactor - e.Graphics.MeasureString(snapxstring, DefaultFont).Width,
                        _selectedControl.Top * Factory.AGSEditor.CurrentGame.GUIScaleFactor
                        );
                }
                if (_snappedy != -1)
                {
                    NormalGUI g = (NormalGUI)_gui;
                    string snapystring = String.Format("{0}px", _snappedy);
                    e.Graphics.DrawLine(_drawSnapPen, 0, _snappedy * Factory.AGSEditor.CurrentGame.GUIScaleFactor, g.Width * Factory.AGSEditor.CurrentGame.GUIScaleFactor, _snappedy * Factory.AGSEditor.CurrentGame.GUIScaleFactor);

                    e.Graphics.DrawString(snapystring,
                   DefaultFont,
                   _drawSnapPen.Brush,
                   _selectedControl.Left * Factory.AGSEditor.CurrentGame.GUIScaleFactor,
                   _selectedControl.Top * Factory.AGSEditor.CurrentGame.GUIScaleFactor - e.Graphics.MeasureString(snapystring, DefaultFont).Height
                   );
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
            else if ((_inResizingArea) && (_selectedControl != null) && (!_selectedControl.Locked))
            {
                _resizingControl = true;
                _mouseXOffset = _selectedControl.Width - (mouseX - _selectedControl.Left);
                _mouseYOffset = _selectedControl.Height - (mouseY - _selectedControl.Top);
            }
            else
            {
                fromCombo = false;
                SetSelectedControlToControlAtPosition(mouseX, mouseY);
                if ((_selectedControl != null) && (!_selectedControl.Locked))
                {
                    _movingControl = true;
                    _mouseXOffset = mouseX - _selectedControl.Left;
                    _mouseYOffset = mouseY - _selectedControl.Top;
                }
                else if (e.Button == MouseButtons.Left && _selectedControl == null)//nothing selected
                {
                    _drawingSelectionBox = true;
                    _selectionRect = new Rectangle(mouseX, mouseY, 1, 1);
                    _selectionBoxX = mouseX;
                    _selectionBoxY = mouseY;
            }
        }

        }

        private void MoveControlWithMouse(int mouseX, int mouseY)
        {
            NormalGUI normalGui = (NormalGUI)_gui;
            //int _changex = (mouseX - _mouseXOffset) - _selectedControl.Left;
            //int _changey = (mouseY - _mouseYOffset) - _selectedControl.Top;
            int[] _diffx = new int[_selected.Count];
            int[] _diffy = new int[_selected.Count];
            for (int i = 0; i < _selected.Count; i++)
            {
                _diffx[i] = _selected[i].Left - _selectedControl.Left;
                _diffy[i] = _selected[i].Top - _selectedControl.Top;
            }

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

            _snappedx = -1;
            _snappedy = -1;
            if (!Factory.NativeProxy.IsControlPressed())
            {
                foreach (GUIControl _gc in normalGui.Controls)
                {
                    if (_gc != _selectedControl && !_selected.Contains(_gc))
                    {
                        if (Math.Abs(_selectedControl.Left - _gc.Left) < 5)
                        {
                            _selectedControl.Left = _gc.Left;
                            _snappedx = _gc.Left;
                        }
                        if (Math.Abs(_selectedControl.Top - _gc.Top) < 5)
                        {
                            _selectedControl.Top = _gc.Top;
                            _snappedy = _gc.Top;
                        }
                    }

                }
            }

            for (int i = 0; i < _selected.Count; i++)
            {
                if (!_selected[i].Locked)
                {
                    _selected[i].Left = _diffx[i] + _selectedControl.Left;
                    _selected[i].Top = _diffy[i] + _selectedControl.Top;
                }
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
            else if (_drawingSelectionBox)
            {
                if (mouseX >= _selectionBoxX)
                {
                    _selectionRect.X = _selectionBoxX;
                    _selectionRect.Width = mouseX - _selectionBoxX;
                }
                else if (mouseX < _selectionBoxX)
                {
                    _selectionRect.X = mouseX;
                    _selectionRect.Width = _selectionBoxX - mouseX;
                }

                if (mouseY >= _selectionBoxY)
                {
                    _selectionRect.Y = _selectionBoxY;
                    _selectionRect.Height = mouseY - _selectionRect.Y;
                }
                else if (mouseY < _selectionBoxY)
                {
                    _selectionRect.Y = mouseY;
                    _selectionRect.Height = _selectionBoxY - mouseY;
                }
                
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

            // check for ctrl


            if (controlFound != null)
            {
                if (Factory.NativeProxy.IsControlPressed())
                {
                    if (_selected.Contains(controlFound) && _selected.Count > 1)
                    {
                        _selected.Remove(controlFound);
                        if (_selected.Count > 0)
                        {
                            _selectedControl = _selected[_selected.Count - 1];
                        }
                        else _selectedControl = null;

                    }
                    else if (!_selected.Contains(controlFound))
                    {
                        if (controlFound.MemberOf != null)
                        {
                            foreach (GUIControl gc in controlFound.MemberOf)
                            {
                                _selected.Add(gc);
                            }
                        }
                        else _selected.Add(controlFound);
                    }
                }
                else
                {

                    if (!_selected.Contains(controlFound))
                    {
                        _selected.Clear();
                        if (controlFound.MemberOf != null)
                        {
                            foreach (GUIControl gc in controlFound.MemberOf)
                            {
                                _selected.Add(gc);
                            }
                        }
                        else _selected.Add(controlFound);

                    }
                    _selectedControl = controlFound;
                }
            }
            else if (!Factory.NativeProxy.IsControlPressed())
            {
                _selected.Clear();
                _selectedControl = null;
            }

            if (_selectedControl == null)
            {
                DeSelectControl();
            }
        }

        private void UnlockControl(object sender, EventArgs e)
        {
            foreach (GUIControl _gc in _selected)
            {
                _gc.Locked = false;
            }
            RaiseOnControlsChanged();
            bgPanel.Invalidate();
        }

        private bool AnyGroupedInSelected()
        {
            foreach (GUIControl gc in _selected)
            {
                if (gc.MemberOf != null) return true;
            }
            return false;
        }

        private bool AllSelectedInSameGroup()
        {
            GUIControlGroup temp = _selected[0].MemberOf;
            foreach (GUIControl gc in _selected)
            {
                if (gc.MemberOf != temp) return false;
                temp = gc.MemberOf;
            }
            return true;
        }

        private void Ungroup(object sender, EventArgs e)
        {
            if (_selectedControl.MemberOf != null)
            {
                _selectedControl.MemberOf.ClearGroup();
                _groups.Remove(_selectedControl.MemberOf);
            }
        }

        private void Group(object sender, EventArgs e)
        {
            GUIControlGroup gg = new GUIControlGroup();
            foreach (GUIControl gc in _selected)
            {
                gg.AddToGroup(gc);
            }
            _groups.Add(gg);
            UpdateGroups();
        }

        private void UpdateGroups()
        {
            for (int i = _groups.Count - 1; i >= 0; i--)
            {
                _groups[i].Update();
                if (_groups[i].Count == 0) _groups.RemoveAt(i);
            }
            
        }

      
        private void LockControl(object sender, EventArgs e)
        {
            foreach (GUIControl _gc in _selected)
            {
                _gc.Locked = true;
            }
            RaiseOnControlsChanged();
            bgPanel.Invalidate();
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

        private void DistributeVertiClick(object sender, EventArgs e)
        {
            if (_selected.Count < 3) return;
            //RemoveLockedFromSelected();
            //if (_selected.Count == 0) return;
            _selected.Sort(GUIControl.CompareByTop);

            int spacing = (_selected[_selected.Count - 1].Top - _selected[0].Top) / (_selected.Count - 1);
            for (int i = 0; i < _selected.Count; i++)
            {
                if (!_selected[i].Locked) _selected[i].Top = _selected[0].Top + (spacing * i);


            }
            RaiseOnControlsChanged();
            bgPanel.Invalidate();
        }

        private void DistributeHorizClick(object sender, EventArgs e)
        {
            if (_selected.Count < 3) return;
            //RemoveLockedFromSelected();
            //if (_selected.Count == 0) return;
            _selected.Sort(GUIControl.CompareByLeft);

            int spacing = (_selected[_selected.Count - 1].Left - _selected[0].Left) / (_selected.Count - 1);
            for (int i = 0; i < _selected.Count; i++)
            {
                if (!_selected[i].Locked) _selected[i].Left = _selected[0].Left + (spacing * i);


            }
            RaiseOnControlsChanged();
            bgPanel.Invalidate();

        }

        private void RemoveLockedFromSelected()
        {
            foreach (GUIControl _gc in _selected)
            {
                if (_gc.Locked) _selected.Remove(_gc);

            }
            RaiseOnControlsChanged();
            bgPanel.Invalidate();
        }

        private void AlignTopClick(object sender, EventArgs e)
        {
            /*
            RemoveLockedFromSelected();
            if (_selected.Count == 0) return;
            */

            foreach (GUIControl _gc in _selected)
            {
                if (!_gc.Locked) _gc.Top = _selectedControl.Top;
            }

            RaiseOnControlsChanged();
            bgPanel.Invalidate();

        }

        private void AlignLeftClick(object sender, EventArgs e)
        {
            /*
            RemoveLockedFromSelected();
            if (_selected.Count == 0) return;
            */
            foreach (GUIControl _gc in _selected)
            {
                if (!_gc.Locked) _gc.Left = _selectedControl.Left;
            }

            RaiseOnControlsChanged();
            bgPanel.Invalidate();

        }

        private void CopyControlClick(object sender, EventArgs e)
        {
            if (_selectedControl != null)
            {

                GUIControl _copyBuffer;
                _copyBuffer = (GUIControl)_selectedControl.Clone();
                _copyBuffer.SaveToClipboard();
            }
        }



        private void PasteControlClick(object sender, EventArgs e)
        {
            if (_gui.Controls.Count >= GUI.MAX_CONTROLS_PER_GUI)
            {
                Factory.GUIController.ShowMessage("You already have the maximum number of controls on this GUI, and cannot add any more.", MessageBoxIcon.Warning);
                return;
            }
            GUIControl newControl = GUIControl.GetFromClipBoard();
            if (newControl != null)
            {

                newControl.Name = Factory.AGSEditor.GetFirstAvailableScriptName(newControl.ControlType);
                newControl.ZOrder = _gui.Controls.Count;
                newControl.ID = _gui.Controls.Count;
                newControl.MemberOf = null;

                newControl.Left = _currentMouseX / Factory.AGSEditor.CurrentGame.GUIScaleFactor;
                newControl.Top = _currentMouseY / Factory.AGSEditor.CurrentGame.GUIScaleFactor;
                _gui.Controls.Add(newControl);
                _selected.Clear();
                _selectedControl = newControl;
                _selected.Add(newControl);


                RaiseOnControlsChanged();

                Factory.GUIController.SetPropertyGridObject(newControl);

                bgPanel.Invalidate();
                UpdateCursorImage();
                // Revert back to Select cursor
                OnCommandClick(Components.GuiComponent.MODE_SELECT_CONTROLS);
            }
        }

        private void DeleteControlClick(object sender, EventArgs e)
        {
            if (_selectedControl != null && !_selectedControl.Locked)
            {
                _selected.Remove(_selectedControl);
                _gui.DeleteControl(_selectedControl);
                if (_selectedControl.MemberOf != null)
                {
                    _selectedControl.MemberOf.RemoveFromGroup(_selectedControl);
                }
                if (_selected.Count > 0)
                {
                    _selectedControl = _selected[_selected.Count - 1];
                }
                else _selectedControl = null;
                RaiseOnControlsChanged();
                if (_selectedControl != null)
                {
                    Factory.GUIController.SetPropertyGridObject(_selectedControl);
                }
                else
                {
                    Factory.GUIController.SetPropertyGridObject(_gui);
                }
                bgPanel.Invalidate();
            }
        }

        private void ShowContextMenu(int x, int y, GUIControl control)
        {
            if (_gui is NormalGUI)
            {

                ContextMenuStrip menu = new ContextMenuStrip();
                if (control != null)
                {
                menu.Items.Add(new ToolStripMenuItem("Bring to Front", null, new EventHandler(BringToFrontClick), "BringToFront"));
                menu.Items.Add(new ToolStripMenuItem("Send to Back", null, new EventHandler(SendToBackClick), "SendToBack"));



                    if (_selected.Count > 1)
                    {
                        if (!AllSelectedInSameGroup() || control.MemberOf == null)
                        {
                            menu.Items.Add(new ToolStripMenuItem("Group", null, new EventHandler(Group), "LockControl"));
                        }
                        if (control.MemberOf != null) menu.Items.Add(new ToolStripMenuItem("Ungroup", null, new EventHandler(Ungroup), "LockControl"));
                        menu.Items.Add(new ToolStripMenuItem("Lock All", null, new EventHandler(LockControl), "LockControl"));
                        menu.Items.Add(new ToolStripMenuItem("Unlock All", null, new EventHandler(UnlockControl), "LockControl"));
                        menu.Items.Add(new ToolStripMenuItem("Distribute Vertically", null, new EventHandler(DistributeVertiClick), "DistVert"));
                        menu.Items.Add(new ToolStripMenuItem("Distribute Horizontally", null, new EventHandler(DistributeHorizClick), "DistVert"));
                        menu.Items.Add(new ToolStripMenuItem("Align Left", null, new EventHandler(AlignLeftClick), "DistVert"));
                        menu.Items.Add(new ToolStripMenuItem("Align Top", null, new EventHandler(AlignTopClick), "DistVert"));
                    }
                    else
                    {
                        if (control.Locked)
                        {
                            menu.Items.Add(new ToolStripMenuItem("Unlock", null, new EventHandler(UnlockControl), "LockControl"));
                        }
                        else
                        {
                            menu.Items.Add(new ToolStripMenuItem("Lock", null, new EventHandler(LockControl), "LockControl"));
                        }
                       

                    }
                menu.Items.Add(new ToolStripSeparator());


                    menu.Items.Add(new ToolStripMenuItem("Copy", null, new EventHandler(CopyControlClick), Keys.Control & Keys.C));
                menu.Items.Add(new ToolStripMenuItem("Delete", null, new EventHandler(DeleteControlClick), Keys.Delete));
                }

                if (GUIControl.AvailableOnClipboard()) menu.Items.Add(new ToolStripMenuItem("Paste", null, new EventHandler(PasteControlClick), Keys.Control & Keys.V));

                if (menu.Items.Count > 0) menu.Show(bgPanel, x, y);
            }
        }

        private void refreshProperties()
        {
            if (_selectedControl != null)
            {

                Factory.GUIController.SetPropertyGridObject(_selectedControl);
            }
            else
            {
                Factory.GUIController.SetPropertyGridObject(_gui);
            }
            bgPanel.Invalidate();
        }

        private void bgPanel_MouseUp(object sender, MouseEventArgs e)
        {
            _snappedx = -1;
            _snappedy = -1;

            if ((_drawingSelectionBox) && (e.X == _selectionBoxX) && (e.Y == _selectionBoxY))
            {
                _drawingSelectionBox = false;
            }

            if (_addingControl)
            {
                _addingControl = false;
                CreateNewControl();
            }
            else if (_resizingControl)
            {
                _resizingControl = false;
                refreshProperties();
            }
            else if (_drawingSelectionBox)
            {
               _drawingSelectionBox = false;
               foreach (GUIControl _gc in _gui.Controls)
               {
                   if (_selectionRect.Contains(_gc.GetRectangle()) && !_selected.Contains(_gc)) _selected.Add(_gc);
               }
               if (_selected.Count > 0) _selectedControl = _selected[_selected.Count - 1];
               bgPanel.Invalidate();
            }
            else
            {
                _movingControl = false;
                refreshProperties();

                if ((e.Button == MouseButtons.Right))
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
            _selected.Clear();
            _selected.Add(newControl);

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


            if (_selectedControl != null && (this.Focused))
            {
                switch (keyData)
            {
                    case Keys.Delete:
                DeleteControlClick(null, null);
                        break;

                    case Keys.Left:
                        foreach (GUIControl _gc in _selected)
                        {
                            _gc.Left--;
                        }
                        break;

                    case Keys.Right:
                        foreach (GUIControl _gc in _selected)
                        {
                            _gc.Left++;
            }
                        break;


                    case Keys.Up:
                        foreach (GUIControl _gc in _selected)
                        {
                            _gc.Top--;
                        }
                        break;


                    case Keys.Down:
                        foreach (GUIControl _gc in _selected)
                        {
                            _gc.Top++;
                        }
                        break;


                }
                bgPanel.Invalidate();
                RaiseOnControlsChanged();
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
