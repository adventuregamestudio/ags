using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Controls
{
    /// <summary>
    /// ListViewExtended extends ListView and adds some additional common functionality:
    /// - ContextMenuTrigger event, triggered by RMB and Apps key alike.
    /// </summary>
    public class ListViewExtended : ListView
    {
        public delegate void ContextMenuTriggerEventHandler(object sender, ListViewContextMenuEventArgs e);
        public event ContextMenuTriggerEventHandler ContextMenuTrigger;

        protected override void OnKeyUp(KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Apps)
            {
                if (SelectedItems.Count > 0)
                {
                    var item = SelectedItems[SelectedItems.Count - 1];
                    var rect = GetItemRect(item.Index, ItemBoundsPortion.Label);
                    OnContextMenuTrigger(new ListViewContextMenuEventArgs(new Point(rect.Left + rect.Width / 2, rect.Bottom), item));
                }
                else
                {
                    var hitPt = PointToClient(MousePosition);
                    if (ClientRectangle.Contains(hitPt))
                    {
                        OnContextMenuTrigger(new ListViewContextMenuEventArgs(hitPt, null));
                    }
                }
                e.Handled = true;
            }

            base.OnKeyUp(e);
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                OnContextMenuTrigger(new ListViewContextMenuEventArgs(e.Location, null));
            }

            base.OnMouseUp(e);
        }

        protected virtual void OnContextMenuTrigger(ListViewContextMenuEventArgs e)
        {
            ContextMenuTrigger?.Invoke(this, e);
        }
    }
}
