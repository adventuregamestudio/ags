using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public class DockingPanel : IDockingPanel
    {
        private DockPanel _dockPanel;

        public DockingPanel(DockPanel dockPanel)
        {
            _dockPanel = dockPanel;
        }

        public DockPanel DockPanel { get { return _dockPanel; } }

        public IDockingPane ActivePane { get { return new DockingPane(_dockPanel.ActivePane); } }

        #region IDockingPanel Members

        public event EventHandler ActiveContentChanged
        {
            add { _dockPanel.ActiveContentChanged += value; }
            remove { _dockPanel.ActiveContentChanged -= value; }
        }

        public bool IsDisposed
        {
            get { return _dockPanel.IsDisposed; }
        }

        public IDockingContent ActiveContent
        {
            get 
            { 
                DockingContainer container = _dockPanel.ActiveContent as DockingContainer;
                if (container == null) return null;
                return container.Panel;
            }
        }

        #endregion
    }
}
