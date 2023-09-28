using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ViewEditor : EditorContentPanel
    {
        private const string MENU_ITEM_DELETE_FRAMES = "DeleteFrames";
        private const string MENU_ITEM_FLIP_FRAMES = "FlipFrames";

        private AGS.Types.View _editingView;
        private List<ViewLoopEditor> _loopPanes = new List<ViewLoopEditor>();
        private AGS.Types.View.ViewUpdatedHandler _viewUpdateHandler;
		private delegate void CorrectAutoScrollDelegate(Point p);
        private GUIController _guiController;
        private bool _processingSelection = false;
        // Multiframe selection cache, may contain frames from multiple loops too!
        private List<ViewFrame> _selectedFrames = new List<ViewFrame>();
        private int _lastSelectedLoop = 0;
        private int _lastSelectedFrame = 0;

        public ViewEditor(AGS.Types.View viewToEdit)
        {
            _guiController = Factory.GUIController;
            _guiController.OnPropertyObjectChanged += new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            _viewUpdateHandler = new AGS.Types.View.ViewUpdatedHandler(View_ViewUpdated);
            viewToEdit.ViewUpdated += _viewUpdateHandler;

            InitializeComponent();

            _editingView = viewToEdit;
            InitializeControls();
			viewPreview.DynamicUpdates = true;
			chkShowPreview.Checked = Factory.AGSEditor.Settings.ShowViewPreviewByDefault;
			UpdateWhetherPreviewIsShown();
        }

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            viewPreview.Invalidate(true);
            editorPanel.Invalidate(true);
        }

        protected override string OnGetHelpKeyword()
        {
            return "View Editor";
        }

        void UpdateLoopVisuals()
        {
            int i = 0;
            foreach (var loopPane in _loopPanes)
            {
                loopPane.ZoomLevel = sldZoomLevel.ZoomScale;
                loopPane.Top = 10 + i * loopPane.Height + editorPanel.AutoScrollPosition.Y;

                if (loopPane.IsLastLoop)
                {
                    btnNewLoop.Top = 10 + loopPane.Top + loopPane.Height;
                    btnDeleteLastLoop.Top = btnNewLoop.Top;
                }
                i++;
            }
        }

        private void sldZoomLevel_ValueChanged(object sender, EventArgs e)
        {
            UpdateLoopVisuals();
        }

        private void InitializeControls()
        {
            int lastPaneY = 10;
            foreach (ViewLoop loop in _editingView.Loops)
            {
                ViewLoopEditor loopPane = AddNewLoopPane(loop);
                lastPaneY = loopPane.Top + loopPane.Height;
            }
            btnNewLoop.Left = 10;
            btnDeleteLastLoop.Left = btnNewLoop.Left + btnNewLoop.Width + 10;
            btnNewLoop.Top = 10 + lastPaneY;
            btnDeleteLastLoop.Top = btnNewLoop.Top;

            if (_editingView.Loops.Count == 0)
            {
                btnDeleteLastLoop.Visible = false;
            }

        }

        public AGS.Types.View ViewToEdit
        {
            get { return _editingView; }
        }

        private void View_ViewUpdated(AGS.Types.View view)
        {
            foreach (ViewLoopEditor pane in _loopPanes)
            {
				editorPanel.Controls.Remove(pane);
                pane.Dispose();
            }
            _loopPanes.Clear();
            InitializeControls();
			viewPreview.ViewUpdated();
        }

        private ViewLoopEditor AddNewLoopPane(ViewLoop loop)
        {
            ViewLoopEditor loopPane = new ViewLoopEditor(loop, _guiController);
            loopPane.Left = 10 + editorPanel.AutoScrollPosition.X;
            loopPane.ZoomLevel = sldZoomLevel.ZoomScale;
            loopPane.Top = 10 + _loopPanes.Count * loopPane.Height + editorPanel.AutoScrollPosition.Y;
            loopPane.HandleRangeSelection = false; // we will handle multi-loop selection ourselves
            loopPane.SelectedFrameChanged += new ViewLoopEditor.SelectedFrameChangedHandler(loopPane_SelectedFrameChanged);
			loopPane.NewFrameAdded += new ViewLoopEditor.NewFrameAddedHandler(loopPane_NewFrameAdded);
            loopPane.OnContextMenu += loopPane_OnContextMenu;
            if (loop.ID == _editingView.Loops.Count - 1)
            {
                loopPane.IsLastLoop = true;
            }
			loopPane.Enter += new EventHandler(loopPane_GotFocus);
			//loopPane.GotFocus += new EventHandler(loopPane_GotFocus);
			//loopPane.Leave += new EventHandler(loopPane_GotFocus);
            editorPanel.Controls.Add(loopPane);
            editorPanel.AutoScrollPosition = new Point(0, editorPanel.VerticalScroll.Maximum);
            _loopPanes.Add(loopPane);
            return loopPane;
        }

		/// <summary>
		/// Workaround for the panel's AutoScroll behaviour, which centres
		/// the loop pane control within the viewable area of the panel
		/// when it gets focus. This causes an annoying jump in the window
		/// position. BeginInvoke queues up the CorrectAutoScroll function
		/// to run after the panel's autoscroll code has run, and we put
		/// it back to where it was before.
		/// </summary>
		private void loopPane_GotFocus(object sender, EventArgs e)
		{
			Point p = editorPanel.AutoScrollPosition;
			BeginInvoke(new CorrectAutoScrollDelegate(CorrectAutoScroll), p);
		}

		private void CorrectAutoScroll(Point p)
		{
			// We have to make the X/Y positive when setting the position.
			// How bizarre.
			editorPanel.AutoScrollPosition = new Point(Math.Abs(p.X), Math.Abs(p.Y));
		}

		private void loopPane_NewFrameAdded(ViewLoop loop, int newSelectedFrame)
		{
			if (newSelectedFrame > 0)
			{
				// Attempt to initialize the new frame to the sprite that
				// comes after the previous frame in the sprite manager
				int previousFrameImage = loop.Frames[newSelectedFrame - 1].Image;
				SpriteFolder parent = Factory.AGSEditor.CurrentGame.RootSpriteFolder.FindFolderThatContainsSprite(previousFrameImage);
				if ((parent != null) && (previousFrameImage > 0))
				{
					for (int i = 0; i < parent.Sprites.Count; i++)
					{
						if ((parent.Sprites[i].Number == previousFrameImage) &&
							(i < parent.Sprites.Count - 1))
						{
							loop.Frames[newSelectedFrame].Image = parent.Sprites[i + 1].Number;
							break;
						}
					}
				}
			}
		}

        private void loopPane_SelectedFrameChanged(ViewLoop loop, int newSelectedFrame, MultiSelectAction action)
        {
            if (_processingSelection)
                return; // avoid double entering, or something updates selection in a batch

            _processingSelection = true;

            // If it's not a single frame Add or Remove, then reset all previous selection
            if (action != MultiSelectAction.Add && action != MultiSelectAction.Remove)
            {
                _selectedFrames.Clear();
                // Deselect all the other loops
                foreach (ViewLoopEditor pane in _loopPanes)
                {
                    if (pane.Loop != loop)
                    {
                        pane.SelectedFrames.Clear();
                        pane.Invalidate();
                    }
                }
            }

            switch (action)
            {
                case MultiSelectAction.Add:
                case MultiSelectAction.Set:
                    _selectedFrames.Add(loop.Frames[newSelectedFrame]);
                    break;
                case MultiSelectAction.Remove:
                    _selectedFrames.Remove(loop.Frames[newSelectedFrame]);
                    break;
                case MultiSelectAction.AddRange:
                    if (_lastSelectedLoop == loop.ID)
                    {
                        // Simplest case: a range within a single loop
                        ViewLoopEditor loopPane = _loopPanes[loop.ID];
                        int min = Math.Min(_lastSelectedFrame, newSelectedFrame);
                        int max = Math.Max(_lastSelectedFrame, newSelectedFrame);
                        for (int i = min; i <= max; ++i)
                        {
                            loopPane.SelectedFrames.Add(i);
                            _selectedFrames.Add(loop.Frames[i]);
                        }
                    }
                    else
                    {
                        // Selection across multiple loops
                        // Select parts of the first and last loops in range
                        int minLoop = Math.Min(_lastSelectedLoop, loop.ID);
                        int maxLoop = Math.Max(_lastSelectedLoop, loop.ID);
                        int minFrame = Math.Min(_lastSelectedFrame, newSelectedFrame);
                        int maxFrame = Math.Max(_lastSelectedFrame, newSelectedFrame);
                        ViewLoop firstLoop = _editingView.Loops[minLoop];
                        ViewLoop lastLoop = _editingView.Loops[maxLoop];
                        ViewLoopEditor firstLoopPane = _loopPanes[minLoop];
                        ViewLoopEditor lastLoopPane = _loopPanes[maxLoop];

                        for (int i = minFrame; i < firstLoop.Frames.Count; ++i)
                        {
                            firstLoopPane.SelectedFrames.Add(i);
                            _selectedFrames.Add(firstLoop.Frames[i]);
                        }
                        for (int i = 0; i <= maxFrame; ++i)
                        {
                            lastLoopPane.SelectedFrames.Add(i);
                            _selectedFrames.Add(lastLoop.Frames[i]);
                        }
                        firstLoopPane.Invalidate();
                        lastLoopPane.Invalidate();

                        // Now select all the loops in between
                        for (int loopIndex = minLoop + 1; loopIndex < maxLoop; ++loopIndex)
                        {
                            ViewLoop otherLoop = _editingView.Loops[loopIndex];
                            ViewLoopEditor otherPane = _loopPanes[loopIndex];
                            for (int i = 0; i < otherLoop.Frames.Count; ++i)
                            {
                                _loopPanes[loopIndex].SelectedFrames.Add(i);
                                _selectedFrames.Add(otherLoop.Frames[i]);
                            }
                            otherPane.Invalidate();
                        }
                    }
                    break;
            }

            _lastSelectedLoop = loop != null ? loop.ID : 0;
            _lastSelectedFrame = Math.Max(0, newSelectedFrame);

            // Now refill the Property Grid
            if (_selectedFrames.Count == 1)
            {
                _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(loop));
                _guiController.SetPropertyGridObject(loop.Frames[newSelectedFrame]);
            }
            else if (_selectedFrames.Count > 1)
            {
                // NOTE: we could keep record of number of loops selected, and still
                // fill loop frames in the objectlist, if all selection is within 1 loop
                _guiController.SetPropertyGridObjectList(null);
                var frames = _selectedFrames.Distinct().ToArray();
                _guiController.SetPropertyGridObjects(frames);
            }
            else
            {
                _guiController.SetPropertyGridObjectList(null);
                _guiController.SetPropertyGridObject(_editingView);
            }

            // the view's Flipped setting might have changed, ensure
            // the preview is updated
            viewPreview.ViewUpdated();
            _processingSelection = false;
        }

        private void loopPane_OnContextMenu(object sender, ViewLoopContextMenuArgs e)
        {
            // In case of a multi-loop selection, override with our own commands
            e.ItemsOverriden = _loopPanes.Count(pane => pane.SelectedFrames.Count > 0) > 1;
            if (e.ItemsOverriden)
            {
                var menu = e.Menu;
                EventHandler onClick = new EventHandler(ContextMenuEventHandler);
                menu.Items.Add(new ToolStripMenuItem("&Flip selected frame(s)", null, onClick, MENU_ITEM_FLIP_FRAMES));
                ToolStripMenuItem deleteOption = new ToolStripMenuItem("Delete selected frame(s)", null, onClick, MENU_ITEM_DELETE_FRAMES);
                deleteOption.ShortcutKeys = Keys.Delete;
                menu.Items.Add(deleteOption);
            }
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_DELETE_FRAMES)
            {
                DeleteSelectedFrames();
            }
            else if (item.Name == MENU_ITEM_FLIP_FRAMES)
            {
                FlipSelectedFrames();
            }
        }

        private void DeleteSelectedFrames()
        {
            _processingSelection = true;
            foreach (ViewLoopEditor loopPane in _loopPanes)
            {
                loopPane.DeleteSelectedFrames();
            }
            _processingSelection = false;
            loopPane_SelectedFrameChanged(null, 0, MultiSelectAction.ClearAll);
        }

        private void FlipSelectedFrames()
        {
            foreach (ViewLoopEditor loopPane in _loopPanes)
            {
                loopPane.FlipSelectedFrames();
            }
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (GUIController.Instance.ActivePane != ContentDocument)
                return; // not this pane

            if (_processingSelection)
                return;

            if (newPropertyObject is ViewFrame)
            {
                foreach (ViewLoopEditor pane in _loopPanes)
                {
                    foreach (ViewFrame frame in pane.Loop.Frames)
                    {
                        if (newPropertyObject == frame)
                        {
                            // FIXME: find a way to assign a property, with invalidation
                            pane.SelectedFrames.Clear();
                            pane.SelectedFrames.Add(frame.ID);
                            pane.Invalidate();
                            break;
                        }
                    }
                }
            }
        }

        private Dictionary<string, object> ConstructPropertyObjectList(ViewLoop loop)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            foreach (ViewFrame frame in loop.Frames)
            {
                list.Add("Loop " + loop.ID + " frame " + frame.ID + " (View frame)", frame);
            }
            return list;
        }

        protected override void OnDispose()
        {
            _editingView.ViewUpdated -= _viewUpdateHandler;
            _guiController.OnPropertyObjectChanged -= new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
        }

        private void btnNewLoop_Click(object sender, EventArgs e)
        {
            if (_loopPanes.Count > 0)
            {
                _loopPanes[_loopPanes.Count - 1].IsLastLoop = false;
            }
            ViewLoop newLoop = _editingView.AddNewLoop();
            ViewLoopEditor newPane = AddNewLoopPane(newLoop);
            btnNewLoop.Top = 10 + newPane.Top + newPane.Height;
            btnDeleteLastLoop.Top = btnNewLoop.Top;
            btnDeleteLastLoop.Visible = true;
			viewPreview.ViewUpdated();
        }

        private void btnDeleteLastLoop_Click(object sender, EventArgs e)
        {
            btnNewLoop.Top -= _loopPanes[0].Height;
            btnDeleteLastLoop.Top = btnNewLoop.Top;
            btnNewLoop.Visible = true;

            _editingView.Loops.RemoveAt(_editingView.Loops.Count - 1);
            _loopPanes[_loopPanes.Count - 1].Dispose();
            _loopPanes.RemoveAt(_loopPanes.Count - 1);

            loopPane_SelectedFrameChanged(null, -1, MultiSelectAction.ClearAll);

            if (_loopPanes.Count == 0)
            {
                btnDeleteLastLoop.Visible = false;
            }
            else
            {
                _loopPanes[_loopPanes.Count - 1].IsLastLoop = true;
            }

			viewPreview.ViewUpdated();
        }

        protected override void OnKeyPressed(Keys keyData)
        {
            if (keyData == Keys.Delete)
            {
                DeleteSelectedFrames();
            }
			else if (keyData == Keys.F)
			{
                FlipSelectedFrames();
			}
        }

		private void UpdateWhetherPreviewIsShown()
		{
			if (chkShowPreview.Checked)
			{
                // Adjust control size to match user's DPI settings
                viewPreview.Width = viewPreview.PreferredSize.Width;
                viewPreview.Height = viewPreview.PreferredSize.Height;
				viewPreview.ViewToPreview = _editingView;
                splitContainer1.Panel1Collapsed = false;
            }
			else
			{
                viewPreview.ReleaseResources();
                splitContainer1.Panel1Collapsed = true;
            }

			editorPanel.Width = this.ClientSize.Width - editorPanel.Left;
			editorPanel.AutoScrollPosition = new Point(0, 0);
		}

		private void chkShowPreview_CheckedChanged(object sender, EventArgs e)
		{
			UpdateWhetherPreviewIsShown();
		}

		private void ViewEditor_Resize(object sender, EventArgs e)
		{
			if (this.ClientSize.Width > editorPanel.Left)
			{
				editorPanel.Width = this.ClientSize.Width - editorPanel.Left;
			}
			if (this.ClientSize.Height > editorPanel.Top)
			{
				editorPanel.Height = this.ClientSize.Height - editorPanel.Top;
			}
		}

		private void ViewEditor_Load(object sender, EventArgs e)
		{
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "view-editor");
            t.ButtonHelper(btnDeleteLastLoop, "view-editor/btn-delete-option");
            t.ButtonHelper(btnNewLoop, "view-editor/btn-new-option");
        }
    }
}
