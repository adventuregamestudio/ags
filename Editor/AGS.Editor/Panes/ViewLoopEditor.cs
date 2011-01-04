using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ViewLoopEditor : UserControl
    {
        private const int FRAME_DISPLAY_SIZE_96DPI = 50;
        private int FRAME_DISPLAY_SIZE;
        private const string MENU_ITEM_DELETE_FRAME = "DeleteFrame";
		private const string MENU_ITEM_FLIP_FRAME = "FlipFrame";
        private const string MENU_ITEM_INSERT_BEFORE = "InsertBefore";
        private const string MENU_ITEM_INSERT_AFTER = "InsertAfter";

        public delegate void SelectedFrameChangedHandler(ViewLoop loop, int newSelectedFrame);
        public event SelectedFrameChangedHandler SelectedFrameChanged;

		public delegate void NewFrameAddedHandler(ViewLoop loop, int newFrameIndex);
		public event NewFrameAddedHandler NewFrameAdded;

		private static int _LastSelectedSprite = 0;

        private ViewLoop _loop;
        private bool _isLastLoop;
        private int _loopDisplayY;
        private int _selectedFrame;

        public ViewLoopEditor(ViewLoop loopToEdit, GUIController guiController)
        {
            InitializeComponent();
            _selectedFrame = -1;
            _loop = loopToEdit;
            lblLoopTitle.Text = "Loop " + _loop.ID + " (" + _loop.DirectionDescription + ")";
            chkRunNextLoop.DataBindings.Add("Checked", _loop, "RunNextLoop", false, DataSourceUpdateMode.OnPropertyChanged);
            _isLastLoop = false;
            _loopDisplayY = chkRunNextLoop.Top + chkRunNextLoop.Height + 2;

            FRAME_DISPLAY_SIZE = guiController.AdjustSizeFrom96DpiToSystemDpi(FRAME_DISPLAY_SIZE_96DPI);
            btnNewFrame.Width = FRAME_DISPLAY_SIZE;
            btnNewFrame.Height = FRAME_DISPLAY_SIZE;
            btnNewFrame.Top = _loopDisplayY;

            UpdateControlWidth();
        }

        public ViewLoop Loop
        {
            get { return _loop; }
        }

        public int SelectedFrame
        {
            get { return _selectedFrame; }
            set { _selectedFrame = value; this.Invalidate(); }
        }

        public bool IsLastLoop
        {
            get { return _isLastLoop; }
            set 
            { 
                _isLastLoop = value;
                if (_isLastLoop)
                {
                    chkRunNextLoop.Checked = false;
                    _loop.RunNextLoop = false;
                }
                chkRunNextLoop.Enabled = !_isLastLoop;
            }
        }

		public void FlipSelectedFrame()
		{
			if ((_selectedFrame >= 0) && (_selectedFrame < _loop.Frames.Count))
			{
				ViewFrame frame = _loop.Frames[_selectedFrame];
				frame.Flipped = !frame.Flipped;
				this.Invalidate();
				OnSelectedFrameChanged();
			}
		}

        public void DeleteSelectedFrame()
        {
            if ((_selectedFrame >= 0) && (_selectedFrame < _loop.Frames.Count))
            {
                _loop.Frames.RemoveAt(_selectedFrame);
                foreach (ViewFrame frame in _loop.Frames)
                {
                    if (frame.ID > _selectedFrame)
                    {
                        frame.ID--;
                    }
                }
                if (_selectedFrame >= _loop.Frames.Count)
                {
                    _selectedFrame = -1;
                }
                btnNewFrame.Visible = true;
                UpdateControlWidth();
                this.Invalidate();
                OnSelectedFrameChanged();
            }
        }

        private void UpdateControlWidth()
        {
            this.Width = Math.Max((_loop.Frames.Count + 1) * FRAME_DISPLAY_SIZE, chkRunNextLoop.Width + 10);
            btnNewFrame.Left = _loop.Frames.Count * FRAME_DISPLAY_SIZE;
        }

        private void ViewLoopEditor_Paint(object sender, PaintEventArgs e)
        {
            IntPtr hdc = e.Graphics.GetHdc();
            Factory.NativeProxy.DrawViewLoop(hdc, _loop, 0, _loopDisplayY, FRAME_DISPLAY_SIZE, _selectedFrame);
            e.Graphics.ReleaseHdc();

			for (int i = 0; i < _loop.Frames.Count; i++)
			{
				string delayString = "DLY:" + _loop.Frames[i].Delay;
				int textWidth = (int)e.Graphics.MeasureString(delayString, this.Font).Width;
				Point textPos = new Point(i * FRAME_DISPLAY_SIZE + FRAME_DISPLAY_SIZE / 2 - (textWidth / 2), btnNewFrame.Bottom + 2);
				e.Graphics.DrawString(delayString, this.Font, Brushes.Black, textPos);
			}
        }

		private void InsertNewFrame(int afterIndex)
        {
            if (afterIndex < 0) afterIndex = -1;
            if (afterIndex >= _loop.Frames.Count) afterIndex = _loop.Frames.Count - 1;

            foreach (ViewFrame frame in _loop.Frames)
            {
                if (frame.ID > afterIndex)
                {
                    frame.ID++;
                }
            }
            ViewFrame newFrame = new ViewFrame();
            newFrame.ID = afterIndex + 1;
            _loop.Frames.Insert(afterIndex + 1, newFrame);

            UpdateControlWidth();

			if (NewFrameAdded != null)
			{
				NewFrameAdded(_loop, newFrame.ID);
			}

			ChangeSelectedFrame(newFrame.ID);
        }

        private void btnNewFrame_Click(object sender, EventArgs e)
        {
            InsertNewFrame(_loop.Frames.Count);
        }

        private int GetFrameAtLocation(int x, int y)
        {
            if ((y >= _loopDisplayY) && (y < _loopDisplayY + FRAME_DISPLAY_SIZE) &&
                (x > 0) && (x < _loop.Frames.Count * FRAME_DISPLAY_SIZE))
            {
                return x / FRAME_DISPLAY_SIZE;
            }
            return -1;
        }

		private void ChangeSelectedFrame(int newSelection)
		{
			_selectedFrame = newSelection;
			this.Invalidate();

			OnSelectedFrameChanged();
		}

        private void ViewLoopEditor_MouseUp(object sender, MouseEventArgs e)
        {
            int clickedOnFrame = GetFrameAtLocation(e.X, e.Y);
            if (clickedOnFrame >= 0) 
            {
				ChangeSelectedFrame(clickedOnFrame);

                if (e.Button == MouseButtons.Right)
                {
                    ShowContextMenu(e.Location);
                }
            }

        }

        private void OnSelectedFrameChanged()
        {
            if (SelectedFrameChanged != null)
            {
                SelectedFrameChanged(_loop, _selectedFrame);
            }
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_DELETE_FRAME)
            {
                DeleteSelectedFrame();
            }
			else if (item.Name == MENU_ITEM_FLIP_FRAME)
			{
				FlipSelectedFrame();
			}
			else if (item.Name == MENU_ITEM_INSERT_AFTER)
            {
                InsertNewFrame(_selectedFrame);
            }
            else if (item.Name == MENU_ITEM_INSERT_BEFORE)
            {
                InsertNewFrame(_selectedFrame - 1);
            }
        }

        private void ShowContextMenu(Point menuPosition)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
			menu.Items.Add(new ToolStripMenuItem("&Flip", null, onClick, MENU_ITEM_FLIP_FRAME));
			menu.Items.Add(new ToolStripSeparator());
			ToolStripMenuItem deleteOption = new ToolStripMenuItem("Delete frame", null, onClick, MENU_ITEM_DELETE_FRAME);
            deleteOption.ShortcutKeys = Keys.Delete;
            menu.Items.Add(deleteOption);
			menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add(new ToolStripMenuItem("Insert frame before this", null, onClick, MENU_ITEM_INSERT_BEFORE));
            menu.Items.Add(new ToolStripMenuItem("Insert frame after this", null, onClick, MENU_ITEM_INSERT_AFTER));

            menu.Show(this, menuPosition);
        }

        private void ViewLoopEditor_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            int clickedFrame = GetFrameAtLocation(e.X, e.Y);
            if (clickedFrame >= 0)
            {
				int initialSprite = _loop.Frames[clickedFrame].Image;
				if ((initialSprite == 0) && (clickedFrame > 0))
				{
					initialSprite = _loop.Frames[clickedFrame - 1].Image;
				}
				if (initialSprite == 0)
				{
					initialSprite = _LastSelectedSprite;
				}

                Sprite chosen = SpriteChooser.ShowSpriteChooser(initialSprite);
                if (chosen != null)
                {
                    _loop.Frames[clickedFrame].Image = chosen.Number;
					_LastSelectedSprite = chosen.Number;
                }
            }
        }

    }
}
