using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Types
{
    //Need to be the same as the DockState enum
    //of the DockPanel Suite.
    //The reason that we have this enum, is for
    //seperating the API from the library, so that
    //plugins won't face a licensing problem.
    public enum DockingState
    {
        Unknown = 0,
        Float = 1,
        DockTopAutoHide = 2,
        DockLeftAutoHide = 3,
        DockBottomAutoHide = 4,
        DockRightAutoHide = 5,
        Document = 6,
        DockTop = 7,
        DockLeft = 8,
        DockBottom = 9,
        DockRight = 10,
        Hidden = 11,
    }

    public interface IDockingContainer
    {
        event EventHandler DockStateChanged;

        DockingState DockState { get; }
        Icon Icon { get; set; }
        IDockingPane FloatPane { get; }
        ContextMenuStrip TabPageContextMenuStrip { get; set; }
        string Text { get; set; }
        bool IsDisposed { get; }

        void Show(IDockingPanel panel, DockData dockData);
        void Hide();
        void Refresh();
        bool Focus();
    }
}
