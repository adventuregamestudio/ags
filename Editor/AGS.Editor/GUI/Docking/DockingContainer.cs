﻿using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class DockingContainer : DockContent, IDockingContainer
    {
        private EditorContentPanel _panel;
        private bool _isShowing;
        private bool _movedFromDocument;

        public DockingContainer(EditorContentPanel panel)
        {
            _panel = panel;
            Controls.Add(panel);
            panel.Dock = DockStyle.Fill;
        }

        public EditorContentPanel Panel { get { return _panel; } }

        public bool IsShowing { get { return _isShowing; } set { _isShowing = value; } }

        public new void Refresh()
        {
            PerformUglyDockHack();
            base.Refresh();
        }

        public void InitScriptIfNeeded<TState>(Action<TState> action, TState state)
        {
            if (DockState != DockingState.Document && !_movedFromDocument)
            {
                _movedFromDocument = true;
                action(state);
            }
            else if (DockState == DockingState.Document && _movedFromDocument)
            {
                _movedFromDocument = false;
                action(state);
            }
        }

        public new IDockingPane FloatPane
        {
            get
            {
                if (base.FloatPane == null) return null;
                return new DockingPane(base.FloatPane);
            }
        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            base.OnClosing(e);
            e.Cancel = true;
            bool cancelClose = false;
            _panel.PanelClosing(true, ref cancelClose);
            //e.Cancel = cancelClose;            
            if (!cancelClose) Hide();
        }

        #region IDockingContainer Members

        public new DockingState DockState
        {
            get { return (DockingState)base.DockState; }
        }

        public void Show(IDockingPanel panel, DockData dockData)
        {
            IsShowing = true;
            DockPanel dockPanel = ((DockingPanel)panel).DockPanel;
            if (dockData.Location != Rectangle.Empty &&
                    dockData.DockState == DockingState.Float)
            {
                base.Show(dockPanel, dockData.Location);
            }
            else
            {
                PerformUglyDockHack();
                base.Show(dockPanel, (DockState)dockData.DockState);
            }
            IsShowing = false;
        }

        #endregion

        private void PerformUglyDockHack()
        {
            //Ugly Hack for a scenario when moving from floating to document dock, and the panel
            //dock is changed.
            _panel.SuspendLayout();
            _panel.ResumeLayout();
        }
    }
}
