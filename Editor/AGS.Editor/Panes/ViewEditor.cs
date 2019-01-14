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
    public partial class ViewEditor : EditorContentPanel
    {
        private AGS.Types.View _editingView;
        private List<ViewLoopEditor> _loopPanes = new List<ViewLoopEditor>();
        private AGS.Types.View.ViewUpdatedHandler _viewUpdateHandler;
		private delegate void CorrectAutoScrollDelegate(Point p);
        private GUIController _guiController;

        public ViewEditor(AGS.Types.View viewToEdit)
        {
            _guiController = Factory.GUIController;
            _guiController.OnPropertyObjectChanged += new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            _viewUpdateHandler = new AGS.Types.View.ViewUpdatedHandler(View_ViewUpdated);
            viewToEdit.ViewUpdated += _viewUpdateHandler;

            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            _editingView = viewToEdit;
            InitializeControls();
			viewPreview.DynamicUpdates = true;
			chkShowPreview.Checked = Factory.AGSEditor.Settings.ShowViewPreviewByDefault;
			UpdateWhetherPreviewIsShown();
        }


        protected override string OnGetHelpKeyword()
        {
            return "Views";
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
            loopPane.Top = 10 + _loopPanes.Count * loopPane.Height + editorPanel.AutoScrollPosition.Y;
            loopPane.SelectedFrameChanged += new ViewLoopEditor.SelectedFrameChangedHandler(loopPane_SelectedFrameChanged);
			loopPane.NewFrameAdded += new ViewLoopEditor.NewFrameAddedHandler(loopPane_NewFrameAdded);
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

        private void loopPane_SelectedFrameChanged(ViewLoop loop, int newSelectedFrame)
        {
            if (newSelectedFrame >= 0)
            {
                _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(loop));
                _guiController.SetPropertyGridObject(loop.Frames[newSelectedFrame]);
            }
            else
            {
                _guiController.SetPropertyGridObjectList(null);
                _guiController.SetPropertyGridObject(_editingView);
            }

            // deselect all other loops
            foreach (ViewLoopEditor pane in _loopPanes)
            {
                if (pane.Loop != loop)
                {
                    pane.SelectedFrame = -1;
                }
            }

            // the view's Flipped setting might have changed, ensure
            // the preview is updated
            viewPreview.ViewUpdated();
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is ViewFrame)
            {
                foreach (ViewLoopEditor pane in _loopPanes)
                {
                    foreach (ViewFrame frame in pane.Loop.Frames)
                    {
                        if (newPropertyObject == frame)
                        {
                            pane.SelectedFrame = frame.ID;
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

            loopPane_SelectedFrameChanged(null, -1);

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
                foreach (ViewLoopEditor pane in _loopPanes)
                {
                    if (pane.SelectedFrame >= 0)
                    {
                        pane.DeleteSelectedFrame();
                        break;
                    }
                }
            }
			else if (keyData == Keys.F)
			{
				foreach (ViewLoopEditor pane in _loopPanes)
				{
					if (pane.SelectedFrame >= 0)
					{
						pane.FlipSelectedFrame();
						break;
					}
				}
			}
        }

		private void UpdateWhetherPreviewIsShown()
		{
			viewPreview.Visible = chkShowPreview.Checked;
			
			if (viewPreview.Visible)
			{
                // Adjust control size to match user's DPI settings
                viewPreview.Width = viewPreview.PreferredSize.Width;
                viewPreview.Height = viewPreview.PreferredSize.Height;

				editorPanel.Left = viewPreview.Right;
				viewPreview.ViewToPreview = _editingView;
			}
			else
			{
				editorPanel.Left = 0;
				viewPreview.ReleaseResources();
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
		}

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("view-editor/background");
            ForeColor = t.GetColor("view-editor/foreground");
            btnDeleteLastLoop.BackColor = t.GetColor("view-editor/btn-delete-option/background");
            btnDeleteLastLoop.ForeColor = t.GetColor("view-editor/btn-delete-option/foreground");
            btnDeleteLastLoop.FlatStyle = (FlatStyle)t.GetInt("view-editor/btn-delete-option/flat/style");
            btnDeleteLastLoop.FlatAppearance.BorderSize = t.GetInt("view-editor/btn-delete-option/flat/border/size");
            btnDeleteLastLoop.FlatAppearance.BorderColor = t.GetColor("view-editor/btn-delete-option/flat/border/color");
            btnNewLoop.BackColor = t.GetColor("view-editor/btn-new-option/background");
            btnNewLoop.ForeColor = t.GetColor("view-editor/btn-new-option/foreground");
            btnNewLoop.FlatStyle = (FlatStyle)t.GetInt("view-editor/btn-new-option/flat/style");
            btnNewLoop.FlatAppearance.BorderSize = t.GetInt("view-editor/btn-new-option/flat/border/size");
            btnNewLoop.FlatAppearance.BorderColor = t.GetColor("view-editor/btn-new-option/flat/border/color");
        }
    }
}
