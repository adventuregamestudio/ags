using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ViewLoopEditor : UserControl
    {
        private const int FRAME_DISPLAY_SIZE_96DPI = 50;
        private int FRAME_DISPLAY_SIZE;
        private const string MENU_ITEM_COPY_FRAME = "CopyFrame";
        private const string MENU_ITEM_CUT_FRAME = "CutFrame";
        private const string MENU_ITEM_PASTE_FRAME_BEFORE = "PasteFrameBefore";
        private const string MENU_ITEM_PASTE_FRAME_AFTER = "PasteFrameAfter";
        private const string MENU_ITEM_DELETE_FRAME = "DeleteFrame";
		private const string MENU_ITEM_FLIP_FRAME = "FlipFrame";
        private const string MENU_ITEM_REVERSE_FRAMES = "ReverseFrames";
        private const string MENU_ITEM_INSERT_BEFORE = "InsertBefore";
        private const string MENU_ITEM_INSERT_AFTER = "InsertAfter";
        private const string MENU_ITEM_CUT_LOOP = "CutLoop";
        private const string MENU_ITEM_COPY_LOOP = "CopyLoop";
        private const string MENU_ITEM_PASTE_OVER_LOOP = "PasteLoop";
        private const string MENU_ITEM_PASTE_OVER_LOOP_FLIPPED = "PasteLoopFlipped";
        private const string MENU_ITEM_PASTE_OVER_LOOP_REVERSED = "PasteLoopReversed";
        private const string MENU_ITEM_FLIP_ALL = "FlipAll";
        private const string MENU_ITEM_REVERSE_ALL = "ReverseAll";
        private const string MENU_ITEM_QUICK_IMPORT = "QuickImport";
        private const string MENU_ITEM_QUICK_IMPORT_REPLACE = "QuickImportReplace";
        private Icon _audioIcon = Resources.ResourceManager.GetIcon("audio_indicator.ico");
        private Icon _delayIcon = Resources.ResourceManager.GetIcon("delay_indicator.ico");
        private const int ICON_WIDTH = 16;

        public delegate void SelectedFrameChangedHandler(ViewLoop loop, int newSelectedFrame, MultiSelectAction action);
        public event SelectedFrameChangedHandler SelectedFrameChanged;

        public delegate void NewFrameAddedHandler(ViewLoop loop, int newFrameIndex);
        public event NewFrameAddedHandler NewFrameAdded;

        public event EventHandler<ViewLoopContextMenuArgs> OnContextMenu;

        private ViewEditClipboard _clipboard;

        private float _zoomLevel = 1.0f;
        private ViewLoop _loop;
        private bool _isLastLoop;
        private int _loopDisplayY;
        private List<int> _selectedFrames = new List<int>();
        private bool _handleRangeSelection = true;
        private int _lastSingleSelection = 0;
        private int _framelessWidth;
        private GUIController _guiController;

        public ViewLoopEditor(ViewLoop loopToEdit, GUIController guiController, ViewEditClipboard clipboard)
        {
            InitializeComponent();
            _guiController = guiController;
            _clipboard = clipboard;
            _loop = loopToEdit;
            lblLoopTitle.Text = "Loop " + _loop.ID + " (" + _loop.DirectionDescription + ")";
            chkRunNextLoop.DataBindings.Add("Checked", _loop, "RunNextLoop", false, DataSourceUpdateMode.OnPropertyChanged);
            _isLastLoop = false;
            UpdateSize();
        }

        void UpdateSize()
        {
            FRAME_DISPLAY_SIZE = _guiController.AdjustSizeFrom96DpiToSystemDpi((int)(FRAME_DISPLAY_SIZE_96DPI * _zoomLevel));
            this.Height = chkRunNextLoop.Height + lblLoopTitle.Height + FRAME_DISPLAY_SIZE + 28;
            _loopDisplayY = chkRunNextLoop.Top + chkRunNextLoop.Height + 2;

            btnNewFrame.Width = FRAME_DISPLAY_SIZE;
            btnNewFrame.Height = FRAME_DISPLAY_SIZE;
            btnNewFrame.Top = _loopDisplayY;

            _framelessWidth = Math.Min(chkRunNextLoop.Width, this.Width + this.Left);
            UpdateControlWidth();
            Invalidate();
        }

        public ViewLoop Loop
        {
            get { return _loop; }
        }

        public List<int> SelectedFrames
        {
            get { return _selectedFrames; }
        }

        /// <summary>
        /// Get/set whether this ViewLoopEditor will handle frame range selection.
        /// If not, then it will only send range selection events, but does not modify
        /// its own SelectedFrames list.
        /// </summary>
        public bool HandleRangeSelection
        {
            get { return _handleRangeSelection; }
            set { _handleRangeSelection = value; }
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

        public bool TrySelectFrame(int frameIndex, bool nearest = false)
        {
            if (nearest)
            {
                frameIndex = Math.Max(0, Math.Min(frameIndex, _loop.Frames.Count - 1));
            }
            if (frameIndex >= 0 && frameIndex < _loop.Frames.Count)
            {
                ChangeSelectedFrame(frameIndex);
                return true;
            }
            return false;
        }

		public void FlipSelectedFrames()
		{
            if (_selectedFrames.Count == 0)
                return;

            foreach (int sel in _selectedFrames)
			{
				ViewFrame frame = _loop.Frames[sel];
				frame.Flipped = !frame.Flipped;
			}
            this.Invalidate();
        }

        public void ReverseSelectedFrames()
        {
            _selectedFrames.Sort();
            ViewFrame swap = new ViewFrame();
            for (int fw = 0, bw = _selectedFrames.Count - 1; fw < _selectedFrames.Count / 2; ++fw, --bw)
            {
                ViewFrame f1 = _loop.Frames[_selectedFrames[fw]];
                ViewFrame f2 = _loop.Frames[_selectedFrames[bw]];
                f1.CopyTo(swap);
                f2.CopyTo(f1);
                swap.CopyTo(f2);
            }
            this.Invalidate();
        }

        private void ReverseAllFrames()
        {
            ViewFrame swap = new ViewFrame();
            for (int fw = 0, bw = _loop.Frames.Count - 1; fw < _loop.Frames.Count / 2; ++fw, --bw)
            {
                ViewFrame f1 = _loop.Frames[fw];
                ViewFrame f2 = _loop.Frames[bw];
                f1.CopyTo(swap);
                f2.CopyTo(f1);
                swap.CopyTo(f2);
            }
            this.Invalidate();
        }

        public void DeleteSelectedFrames()
        {
            if (_selectedFrames.Count == 0)
                return;

            _selectedFrames.Sort();
            int lastSelected = _selectedFrames.Last();
            ViewFrame nextFrame = (lastSelected < _loop.Frames.Count() - 1) ? _loop.Frames[lastSelected + 1] : null;
            int removedCount = 0;
            foreach (int sel in _selectedFrames)
            {
                int remove_index = sel - (removedCount++);
                _loop.Frames.RemoveAt(remove_index);
                for (int frame = remove_index; frame < _loop.Frames.Count; ++frame)
                {
                    _loop.Frames[frame].ID--;
                }
            }

            UpdateControlWidth();
            btnNewFrame.Visible = true;
            // Try to select any frame after deleted range
            if (_loop.Frames.Count > 0)
                ChangeSelectedFrame(nextFrame != null ? nextFrame.ID : _loop.Frames.Count - 1, MultiSelectAction.Set);
            else
                ChangeSelectedFrame(-1, MultiSelectAction.ClearAll);
        }

        private void UpdateControlWidth()
        {
            this.Width = Math.Max((_loop.Frames.Count + 1) * FRAME_DISPLAY_SIZE + 10, _framelessWidth);
            btnNewFrame.Left = _loop.Frames.Count * FRAME_DISPLAY_SIZE;
        }

        private void ViewLoopEditor_Paint(object sender, PaintEventArgs e)
        {
            IntPtr hdc = e.Graphics.GetHdc();
            Factory.NativeProxy.DrawViewLoop(hdc, _loop, 0, _loopDisplayY, FRAME_DISPLAY_SIZE, _selectedFrames);
            e.Graphics.ReleaseHdc();

			for (int i = 0; i < _loop.Frames.Count; i++)
            {
                bool has_delay_info = _loop.Frames[i].Delay != 0;
                bool has_sound_info = _loop.Frames[i].Sound != 0;
                bool has_any_info = has_delay_info || has_sound_info;
                if (!has_any_info) continue;

                string delayString;
                if (_loop.Frames[i].Delay <= 99)
                {
                    delayString = _loop.Frames[i].Delay.ToString();
                }
                else
                {
                    delayString = ">99";
                }
                int info_width = 0; 

                if (has_delay_info) info_width += ICON_WIDTH + (int)e.Graphics.MeasureString(delayString, this.Font).Width;
                if (has_sound_info) info_width += ICON_WIDTH;

                Point infoPos = new Point(i * FRAME_DISPLAY_SIZE + FRAME_DISPLAY_SIZE / 2 - (info_width / 2), btnNewFrame.Bottom + 2);

                if (has_delay_info)
                {
                    e.Graphics.DrawString(delayString, this.Font, Brushes.Black, infoPos);
                    infoPos.X += (int)e.Graphics.MeasureString(delayString, this.Font).Width - 1;
                    e.Graphics.DrawIcon(_delayIcon, infoPos.X, infoPos.Y);
                    infoPos.X += ICON_WIDTH + 3;
                }
                if (has_sound_info)
                {
                    e.Graphics.DrawIcon(_audioIcon, infoPos.X, infoPos.Y);
                }
            }
        }

		private void InsertNewFrame(int afterIndex)
        {
            InsertNewFrames(afterIndex, new ViewFrame[] { new ViewFrame() }, areBlankFrames: true);
        }

        private void InsertNewFrames(int afterIndex, ViewFrame[] newFrames, bool areBlankFrames)
        {
            if (afterIndex < 0) afterIndex = -1;
            if (afterIndex >= _loop.Frames.Count) afterIndex = _loop.Frames.Count - 1;

            // Shift later frames IDs
            foreach (ViewFrame frame in _loop.Frames)
            {
                if (frame.ID > afterIndex)
                {
                    frame.ID += newFrames.Length;
                }
            }

            // Insert new frames, assigning new IDs
            for (int i = 0; i < newFrames.Length; ++i)
            {
                ViewFrame frame = newFrames[i];
                frame.ID = afterIndex + 1 + i;
                _loop.Frames.Insert(afterIndex + 1 + i, frame);
            }

            UpdateControlWidth();

            // If these are blank frames, then run callback, which may assign new sprites onto them
            if (areBlankFrames && NewFrameAdded != null)
            {
                for (int i = afterIndex + 1; i < afterIndex + 1 + newFrames.Length; ++i)
                {
                    NewFrameAdded(_loop, i);
                }
            }

            SetSelectedFrames(afterIndex + 1, afterIndex + newFrames.Length);
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

        private void ChangeSelectedFrame(int newSelection, MultiSelectAction action = MultiSelectAction.Set)
		{
            switch (action)
            {
                case MultiSelectAction.Add:
                    _selectedFrames.Add(newSelection);
                    _lastSingleSelection = newSelection;
                    break;
                case MultiSelectAction.AddRange:
                    _selectedFrames.Clear();
                    if (_handleRangeSelection)
                    {
                        int min = Math.Min(_lastSingleSelection, newSelection);
                        int max = Math.Max(_lastSingleSelection, newSelection);
                        for (int i = min; i <= max; ++i)
                            _selectedFrames.Add(i);
                    }
                    break;
                case MultiSelectAction.ClearAll:
                    _selectedFrames.Clear();
                    _lastSingleSelection = 0;
                    break;
                case MultiSelectAction.Remove:
                    _selectedFrames.Remove(newSelection);
                    _lastSingleSelection = newSelection;
                    break;
                case MultiSelectAction.Set:
                default:
                    _selectedFrames.Clear();
                    _selectedFrames.Add(newSelection);
                    _lastSingleSelection = newSelection;
                    break;
            }

            this.Invalidate();
            OnSelectedFrameChanged(newSelection, action);
        }

        private void SetSelectedFrames(int first, int last)
        {
            first = MathExtra.Clamp(first, 0, _loop.Frames.Count - 1);
            last = MathExtra.Clamp(first, 0, _loop.Frames.Count - 1);
            first = Math.Min(first, last);
            last = Math.Max(first, last);

            _selectedFrames.Clear();
            for (int i = first; i <= last; ++i)
                _selectedFrames.Add(i);
            _lastSingleSelection = 0;

            this.Invalidate();
            OnSelectedFrameChanged(last, (last - first > 1) ? MultiSelectAction.AddRange : MultiSelectAction.Set);
        }

        private void ViewLoopEditor_MouseUp(object sender, MouseEventArgs e)
        {
            int clickedOnFrame = GetFrameAtLocation(e.X, e.Y);
            if (e.Button == MouseButtons.Left && clickedOnFrame >= 0)
            {
                MultiSelectAction action;
                if ((Control.ModifierKeys & Keys.Shift) != 0)
                    action = MultiSelectAction.AddRange;
                else if ((Control.ModifierKeys & Keys.Control) != 0)
                    action = MultiSelectAction.Add;
                else
                    action = MultiSelectAction.Set;

                ChangeSelectedFrame(clickedOnFrame, action);
            }

            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e.Location, clickedOnFrame);
            }
        }

        private void OnSelectedFrameChanged(int selectedFrame, MultiSelectAction action)
        {
            SelectedFrameChanged?.Invoke(_loop, selectedFrame, action);
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_DELETE_FRAME)
            {
                DeleteSelectedFrames();
            }
			else if (item.Name == MENU_ITEM_FLIP_FRAME)
			{
                FlipSelectedFrames();
			}
			else if (item.Name == MENU_ITEM_INSERT_AFTER)
            {
                int selectedFrame = _selectedFrames.Count > 0 ? _selectedFrames[_selectedFrames.Count - 1] : -1;
                InsertNewFrame(selectedFrame);
            }
            else if (item.Name == MENU_ITEM_INSERT_BEFORE)
            {
                int selectedFrame = _selectedFrames.Count > 0 ? _selectedFrames[0] : 0;
                InsertNewFrame(selectedFrame - 1);
            }
        }

        private void ShowContextMenu(Point menuPosition, int selectedFrame)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            ViewLoopContextMenuArgs ctxArgs = new ViewLoopContextMenuArgs(menu, _loop, selectedFrame >= 0 ? _loop.Frames[selectedFrame] : null);
            OnContextMenu?.Invoke(this, ctxArgs);
            if (ctxArgs.ItemsOverriden)
            {
                if (menu.Items.Count > 0)
                    menu.Show(this, menuPosition);
                return;
            }

            if (selectedFrame >= 0)
            {
                menu.Items.Add(ToolStripExtensions.CreateMenuItem("Copy frame(s)", null, OnCopyFrames, MENU_ITEM_COPY_FRAME, Keys.Control | Keys.C));
                menu.Items.Add(ToolStripExtensions.CreateMenuItem("Cut frame(s)", null, OnCutFrames, MENU_ITEM_CUT_FRAME, Keys.Control | Keys.X));
            }
            if ((selectedFrame >= 0) && (_clipboard.CopiedFrames != null))
            {
                menu.Items.Add(new ToolStripMenuItem("Paste frame(s) before", null, OnPasteFramesBefore, MENU_ITEM_PASTE_FRAME_BEFORE));
                menu.Items.Add(ToolStripExtensions.CreateMenuItem("Paste frame(s) after", null, OnPasteFramesAfter, MENU_ITEM_PASTE_FRAME_AFTER, Keys.Control | Keys.V));
            }
            if (selectedFrame >= 0)
            {
                // NOTE: simple keys like 'F' or 'R' does not work as a menu item shortkey for some reason, so we handle it in OnKeyPressed
                menu.Items.Add(new ToolStripMenuItem("&Flip selected frame(s)", null, onClick, MENU_ITEM_FLIP_FRAME));
                if (_selectedFrames.Count > 1)
                {
                    menu.Items.Add(new ToolStripMenuItem("&Reverse selected frame(s)", null, OnReverseFrames, MENU_ITEM_REVERSE_FRAMES));
                }
                menu.Items.Add(ToolStripExtensions.CreateMenuItem("Delete selected frame(s)", null, onClick, MENU_ITEM_DELETE_FRAME, Keys.Delete));
                if (_selectedFrames.Count == 1)
                {
                    menu.Items.Add(new ToolStripSeparator());
                    menu.Items.Add(new ToolStripMenuItem("Insert frame before this", null, onClick, MENU_ITEM_INSERT_BEFORE));
                    menu.Items.Add(new ToolStripMenuItem("Insert frame after this", null, onClick, MENU_ITEM_INSERT_AFTER));
                }
                menu.Items.Add(new ToolStripSeparator());
            }
            menu.Items.Add(new ToolStripMenuItem("Cut loop", null, onCutLoopClicked, MENU_ITEM_CUT_LOOP));
            menu.Items.Add(new ToolStripMenuItem("Copy loop", null, onCopyLoopClicked, MENU_ITEM_COPY_LOOP));
            if (_clipboard.CopiedLoop != null || _clipboard.CopiedFrames != null)
            {
                menu.Items.Add(new ToolStripMenuItem("Paste over this loop", null, onPasteLoopClicked, MENU_ITEM_PASTE_OVER_LOOP));
                menu.Items.Add(new ToolStripMenuItem("Paste over this loop flipped", null, onPasteFlippedClicked, MENU_ITEM_PASTE_OVER_LOOP_FLIPPED));
                menu.Items.Add(new ToolStripMenuItem("Paste over this loop reversed", null, OnPasteReversedClicked, MENU_ITEM_PASTE_OVER_LOOP_REVERSED));
            }
            menu.Items.Add(new ToolStripMenuItem("Flip all frames in loop", null, onFlipAllClicked, MENU_ITEM_FLIP_ALL));
            menu.Items.Add(new ToolStripMenuItem("Reverse all frames in loop", null, OnReverseAllClicked, MENU_ITEM_REVERSE_ALL));
            menu.Items.Add(new ToolStripMenuItem("Add all sprites from folder...", null, onQuickImportFromFolderClicked, MENU_ITEM_QUICK_IMPORT));
            menu.Items.Add(new ToolStripMenuItem("Replace with all sprites from folder...", null, onQuickImportReplaceFromFolderClicked, MENU_ITEM_QUICK_IMPORT_REPLACE));

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
					initialSprite = _clipboard.LastSelectedSprite;
				}

                Sprite chosen = SpriteChooser.ShowSpriteChooser(initialSprite);
                if (chosen != null)
                {
                    _loop.Frames[clickedFrame].Image = chosen.Number;
                    _clipboard.LastSelectedSprite = chosen.Number;
                }
            }
        }

        private void OnCopyFrames(object sender, EventArgs e)
        {
            List<ViewFrame> frames = new List<ViewFrame>();
            foreach (var i in _selectedFrames)
                frames.Add(_loop.Frames[i].Clone());
            _clipboard.CopiedFrames = frames.ToArray();
        }

        private void OnCutFrames(object sender, EventArgs e)
        {
            OnCopyFrames(sender, e);
            DeleteSelectedFrames();
        }

        private void OnPasteFramesBefore(object sender, EventArgs e)
        {
            int selectedFrame = _selectedFrames.Count > 0 ? _selectedFrames[0] : 0;
            InsertNewFrames(selectedFrame - 1, _clipboard.CopiedFrames.Select(f => f.Clone()).ToArray(), areBlankFrames: false);
        }

        private void OnPasteFramesAfter(object sender, EventArgs e)
        {
            int selectedFrame = _selectedFrames.Count > 0 ? _selectedFrames[_selectedFrames.Count - 1] : -1;
            InsertNewFrames(selectedFrame, _clipboard.CopiedFrames.Select(f => f.Clone()).ToArray(), areBlankFrames: false);
        }

        private void OnReverseFrames(object sender, EventArgs e)
        {
            ReverseSelectedFrames();
        }

        private void copyLoop()
        {
            _clipboard.CopiedLoop = _loop.Clone();
        }

        private void onCopyLoopClicked(object sender, EventArgs e)
        {
            copyLoop();
        }

        private void onCutLoopClicked(object sender, EventArgs e)
        {
            copyLoop();
            if (_loop.Frames.Count > 0)
            {
                _selectedFrames.Clear();
                _loop.Frames.Clear();

                btnNewFrame.Visible = true;
                UpdateControlWidth();
                this.Invalidate();
                OnSelectedFrameChanged(-1, MultiSelectAction.ClearAll);
            }
        }

        private void pasteLoop(bool flipped)
        {
            if (_clipboard.CopiedLoop != null)
            {
                _clipboard.CopiedLoop.Clone(_loop, flipped);
            }
            else
            {
                _loop.Frames.Clear();
                _loop.Frames.AddRange(_clipboard.CopiedFrames.Select(f => f.Clone(flipped)));
            }
            UpdateControlWidth();
            this.Invalidate();
        }

        private void onPasteLoopClicked(object sender, EventArgs e)
        {
            pasteLoop(false);
        }

        private void onPasteFlippedClicked(object sender, EventArgs e)
        {
            pasteLoop(true);
        }

        private void OnPasteReversedClicked(object sender, EventArgs e)
        {
            pasteLoop(false);
            ReverseAllFrames();
        }

        private void onFlipAllClicked(object sender, EventArgs e)
        {            
            _loop.Frames.ForEach(c => c.Flipped = !c.Flipped);
            this.Invalidate();
        }

        private void OnReverseAllClicked(object sender, EventArgs e)
        {
            ReverseAllFrames();
        }

        private void QuickImportFromFolder(bool clear_loop_frames)
        {
            Sprite chosen = SpriteChooser.ShowSpriteChooser(_clipboard.LastSelectedSprite, "Select the first sprite to be imported from the folder");
            if (chosen != null)
            {
                SpriteFolder parent = Factory.AGSEditor.CurrentGame.RootSpriteFolder.FindFolderThatContainsSprite(chosen.Number);
                if (parent != null)
                {
                    if (clear_loop_frames) _loop.Frames.Clear();
                    for (int i = 0; i < parent.Sprites.Count; i++)
                    {
                        if (parent.Sprites[i].Number >= chosen.Number)
                        {
                            _loop.Frames.Add(new ViewFrame
                            {
                                ID = _loop.Frames.Count,
                                Image = parent.Sprites[i].Number,
                            });
                        }
                    }

                    UpdateControlWidth();
                    this.Invalidate();
                }
            }
        }

        private void onQuickImportFromFolderClicked(object sender, EventArgs e)
        {
            QuickImportFromFolder(false);
        }

        private void onQuickImportReplaceFromFolderClicked(object sender, EventArgs e)
        {
            QuickImportFromFolder(true);
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ButtonHelper(btnNewFrame, "view-editor/btn-new-frame");
        }

        public float ZoomLevel
        {
            get
            {
                return _zoomLevel;
            }
            set
            {
                _zoomLevel = value;
                UpdateSize();
            }
        }

        private void ViewLoopEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
