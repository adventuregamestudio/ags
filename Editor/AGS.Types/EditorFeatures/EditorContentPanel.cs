using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Types
{
	public class EditorContentPanel : UserControl, IDisposable, IDockingContent
	{
        IDockingContainer _dockingContainer;
        ContentDocument _contentDocument;
        bool _firstTimeShown;

        public EditorContentPanel()
			: base()
		{                        
		}

        public IDockingContainer DockingContainer
        {
            get { return _dockingContainer; }
            set { _dockingContainer = value; }
        }

        public ContentDocument ContentDocument
        {
            get { return _contentDocument; }
            set { _contentDocument = value; }
        }

        public void CommandClick(string command)
		{
			OnCommandClick(command);
		}

		protected virtual void OnCommandClick(string command)
		{
		}

        /// <summary>
        /// This method override is used to perform an unfortunate hack:
        /// we need the derived classes to be able to tell when the panel
        /// is shown in its full size. OnLoad does not guarantee this, because
        /// its happening prior to panel resized on a docking site. Therefore
        /// we do this instead...
        /// </summary>
        protected override void OnSizeChanged(EventArgs e)
        {
            base.OnSizeChanged(e);

            if (Created && !_firstTimeShown)
            {
                OnPanelFirstTimeShown();
                _firstTimeShown = true;
            }
        }

        /// <summary>
        /// The panel is about to be shown for the first time.
        /// This method is called after the panel is created, and resized
        /// to fill its docking container.
        /// </summary>
        protected virtual void OnPanelFirstTimeShown()
        {
        }

        /// <summary>
        /// Notifies the panel that it is about to close. If canCancel is true, then
        /// the user is attempting to close and it can be aborted. If it's
        /// false, then the editor is exiting and you can't abort the close.
        /// </summary>
		public void PanelClosing(bool canCancel, ref bool cancelClose)
		{
			OnPanelClosing(canCancel, ref cancelClose);
		}

		/// <summary>
		/// This pane is about to be closed. If canCancel is true, then
		/// the user is attempting to close and it can be aborted. If it's
		/// false, then the editor is exiting and you can't abort the close.
		/// </summary>
		protected virtual void OnPanelClosing(bool canCancel, ref bool cancelClose)
		{
		}

		public bool KeyPressed(Keys keyData)
		{
			if (HandleKeyPress(keyData))
			{
				return true;
			}
			OnKeyPressed(keyData);
			return false;
		}

        public bool KeyReleased(Keys keyData)
        {
            if (HandleKeyRelease(keyData))
            {
                return true;
            }
            OnKeyReleased(keyData);
            return false;
        }

		/// <summary>
		/// Attempts to handle a key press, and returns true if it has done.
		/// </summary>
		protected virtual bool HandleKeyPress(Keys keyData)
		{
			return false;
		}

		/// <summary>
		/// Process a key press, but cannot mark it as handled.
		/// </summary>
		/// <param name="keyData"></param>
		protected virtual void OnKeyPressed(Keys keyData)
		{
		}

        /// <summary>
		/// Attempts to handle a key release, and returns true if it has done.
		/// </summary>
		protected virtual bool HandleKeyRelease(Keys keyData)
        {
            return false;
        }

        /// <summary>
		/// Process a key release, but cannot mark it as handled.
		/// </summary>
		/// <param name="keyData"></param>
        protected virtual void OnKeyReleased(Keys keyData)
        {
        }

        /// <summary>
        /// This panel has just become the active one
        /// </summary>
        public void WindowActivated()
		{
			OnWindowActivated();
		}

		protected virtual void OnWindowActivated()
		{
		}

        public void WindowDeactivated()
        {
            OnWindowDeactivated();
        }

        protected virtual void OnWindowDeactivated()
        {
        }

		public void PropertyChanged(string propertyName, object oldValue)
		{
			OnPropertyChanged(propertyName, oldValue);
		}

		protected virtual void OnPropertyChanged(string propertyName, object oldValue)
		{
		}

		public string HelpKeyword
		{
			get { return OnGetHelpKeyword(); }
		}

		/// <summary>
		/// Gets the help keyword that will be used to bring up the help
		/// file when the user presses F1 in this pane.
		/// </summary>
		protected virtual string OnGetHelpKeyword()
		{
			return string.Empty;
		}

		public new void Dispose()
		{
			OnDispose();
			base.Dispose();
		}

        protected virtual void OnDispose()
		{
		}
	}
}
