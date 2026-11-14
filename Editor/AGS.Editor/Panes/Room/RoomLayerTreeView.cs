using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using AGS.Controls;

namespace AGS.Editor.Panes.Room
{
    public class RoomLayerNodeEventArgs : EventArgs
    {
        public RoomLayerNodeEventArgs(RoomLayerNode node)
        {
            Node = node;
        }

        public RoomLayerNode Node { get; private set; }
    }

    /// <summary>
    /// Shows a <see cref="RoomLayerTree"/>, with the visible and locked toggles at the right
    /// edge of every row. A click on a toggle changes it without selecting the row.
    /// </summary>
    public class RoomLayerTreeView : TreeView
    {
        private const int TVS_NOHSCROLL = 0x8000;
        private const int WM_ERASEBKGND = 0x0014;
        private const int WM_LBUTTONDOWN = 0x0201;
        private const int WM_LBUTTONDBLCLK = 0x0203;
        private const int TOGGLE_WIDTH = 18;
        private const int TOGGLES_GAP = 4;

        private const string TIP_NOTHING_DRAWN = "Nothing drawn: this area has properties set but no pixels in the mask";
        private const string TIP_UNUSED = "Unused: nothing drawn and no properties set";
        private const string TIP_UNUSED_GROUP = "Areas with nothing drawn and no properties set";

        private readonly HashSet<string> _expanded = new HashSet<string>();
        private readonly Image _eyeOpen = global::AGS.Editor.Properties.Resources.eye;
        private readonly Image _eyeClosed = global::AGS.Editor.Properties.Resources.eye_closed;
        private readonly Image _lockClosed = global::AGS.Editor.Properties.Resources.lockResource;
        private readonly Image _lockOpen = global::AGS.Editor.Properties.Resources.lock_open;
        private RoomLayerTree _tree;
        private Font _italicFont;
        private int _paintedWidth;
        // Set while the nodes are changed from code: no NodeSelected then
        private bool _updating;

        public RoomLayerTreeView()
        {
            DrawMode = TreeViewDrawMode.OwnerDrawText;
            HideSelection = false;
            ShowNodeToolTips = true;
        }

        /// <summary>
        /// Raised when the user selects a node.
        /// </summary>
        public event EventHandler<RoomLayerNodeEventArgs> NodeSelected;
        /// <summary>
        /// Raised when the user changed a node's visible or locked toggle.
        /// </summary>
        public event EventHandler<RoomLayerNodeEventArgs> NodeToggled;
        /// <summary>
        /// Raised when the user presses Enter.
        /// </summary>
        public event EventHandler LeaveRequested;

        public RoomLayerTree Tree
        {
            get { return _tree; }
            set
            {
                _tree = value;
                Reload();
            }
        }

        public RoomLayerNode SelectedLayerNode
        {
            get { return SelectedNode?.Tag as RoomLayerNode; }
        }

        /// <summary>
        /// Shows the nodes again after <see cref="RoomLayerTree.Rebuild"/>, keeping the expanded
        /// nodes, the selection and the scroll position. Returns the selected node.
        /// </summary>
        public RoomLayerNode Reload()
        {
            string selectedKey = SelectedLayerNode?.Key;
            string topKey = (TopNode?.Tag as RoomLayerNode)?.Key;
            _updating = true;
            BeginUpdate();
            try
            {
                Nodes.Clear();
                if (_tree == null)
                    return null;
                // The room is a leaf, its layers are not indented under it
                Nodes.Add(CreateNode(_tree.Root, false));
                foreach (RoomLayerNode layer in _tree.Root.Children)
                {
                    TreeNode layerNode = CreateNode(layer, true);
                    Nodes.Add(layerNode);
                    Expand(layerNode);
                }
                SelectedNode = FindNode(_tree.FindNearest(selectedKey).Key);
                TreeNode top = FindNode(topKey);
                if (top != null)
                    TopNode = top;
            }
            finally
            {
                EndUpdate();
                _updating = false;
            }
            return SelectedLayerNode;
        }

        public void InvalidateToggles()
        {
            int left = TogglesLeft;
            Invalidate(new Rectangle(left, 0, ClientSize.Width - left, ClientSize.Height));
        }

        /// <summary>
        /// Selects the node with this key without raising NodeSelected.
        /// </summary>
        public void SelectKey(string key)
        {
            TreeNode node = FindNode(key);
            if (node == null)
                return;
            _updating = true;
            try
            {
                SelectedNode = node;
            }
            finally
            {
                _updating = false;
            }
        }

        protected override CreateParams CreateParams
        {
            get
            {
                // No horizontal scrolling: the toggles stay at the right edge
                CreateParams cp = base.CreateParams;
                cp.Style |= TVS_NOHSCROLL;
                return cp;
            }
        }

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            this.EnableDoubleBuffering();
            UpdateItemHeight();
        }

        protected override void OnFontChanged(EventArgs e)
        {
            base.OnFontChanged(e);
            _italicFont?.Dispose();
            _italicFont = null;
            UpdateItemHeight();
        }

        protected override void OnClientSizeChanged(EventArgs e)
        {
            base.OnClientSizeChanged(e);
            Invalidate();
        }

        protected override void OnAfterSelect(TreeViewEventArgs e)
        {
            base.OnAfterSelect(e);
            RoomLayerNode node = SelectedLayerNode;
            if (!_updating && node != null)
                NodeSelected?.Invoke(this, new RoomLayerNodeEventArgs(node));
        }

        protected override void OnAfterExpand(TreeViewEventArgs e)
        {
            base.OnAfterExpand(e);
            _expanded.Add(e.Node.Name);
        }

        protected override void OnAfterCollapse(TreeViewEventArgs e)
        {
            base.OnAfterCollapse(e);
            _expanded.Remove(e.Node.Name);
        }

        protected override bool IsInputKey(Keys keyData)
        {
            return keyData == Keys.Enter || base.IsInputKey(keyData);
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (e.KeyData == Keys.Enter)
            {
                e.Handled = true;
                e.SuppressKeyPress = true;
                LeaveRequested?.Invoke(this, EventArgs.Empty);
                return;
            }
            base.OnKeyDown(e);
        }

        protected override void WndProc(ref Message m)
        {
            if (m.Msg == WM_ERASEBKGND)
            {
                // The double-buffered tree does not erase here, and its expand animation
                // showed garbage in the rows it uncovered
                using (Graphics graphics = Graphics.FromHdc(m.WParam))
                using (SolidBrush brush = new SolidBrush(BackColor))
                    graphics.FillRectangle(brush, ClientRectangle);
                m.Result = (IntPtr)1;
                return;
            }
            if ((m.Msg == WM_LBUTTONDOWN || m.Msg == WM_LBUTTONDBLCLK) && _tree != null)
            {
                int lParam = unchecked((int)(long)m.LParam);
                Point point = new Point((short)(lParam & 0xFFFF), (short)((lParam >> 16) & 0xFFFF));
                bool onEye;
                RoomLayerNode node = FindToggleAt(point, out onEye);
                if (node != null)
                {
                    bool changed = onEye ? _tree.SetVisible(node, !_tree.IsVisible(node)) :
                        _tree.SetLocked(node, !_tree.IsLocked(node));
                    if (changed)
                    {
                        InvalidateToggles(); // showing a layer with a mask hides the others
                        NodeToggled?.Invoke(this, new RoomLayerNodeEventArgs(node));
                    }
                    return; // the click neither selects nor expands
                }
            }
            base.WndProc(ref m);
        }

        protected override void OnDrawNode(DrawTreeNodeEventArgs e)
        {
            RoomLayerNode node = e.Node.Tag as RoomLayerNode;
            if (node == null || _tree == null || e.Bounds.IsEmpty)
            {
                e.DrawDefault = true;
                return;
            }

            // The vertical scroll bar changes the width without a resize, and the toggles move
            // with it: draw every row again, after this paint
            if (ClientSize.Width != _paintedWidth)
            {
                _paintedWidth = ClientSize.Width;
                BeginInvoke((MethodInvoker)(() => Invalidate()));
            }
            int togglesLeft = TogglesLeft;
            bool selected = (e.State & TreeNodeStates.Selected) != 0;
            bool active = selected && Focused;
            bool grey = node.Kind == RoomLayerNodeKind.UnusedGroup || node.ItemState == RoomLayerItemState.Unused;
            Font font = node.ItemState == RoomLayerItemState.NothingDrawn ? ItalicFont : Font;
            Color fore = active ? SystemColors.HighlightText : (grey ? SystemColors.GrayText : ForeColor);
            Color back = active ? SystemColors.Highlight : (selected ? Blend(BackColor, ForeColor, 0.25) : BackColor);

            int labelRight = Math.Max(e.Bounds.Left, togglesLeft - TOGGLES_GAP);
            Size textSize = TextRenderer.MeasureText(e.Graphics, e.Node.Text, font);
            Rectangle label = new Rectangle(e.Bounds.Left, e.Bounds.Top,
                Math.Min(Math.Max(e.Bounds.Width, textSize.Width), labelRight - e.Bounds.Left), e.Bounds.Height);
            using (SolidBrush brush = new SolidBrush(back))
                e.Graphics.FillRectangle(brush, label);
            TextRenderer.DrawText(e.Graphics, e.Node.Text, font, label, fore,
                TextFormatFlags.NoPrefix | TextFormatFlags.SingleLine | TextFormatFlags.VerticalCenter |
                TextFormatFlags.EndEllipsis);
            if (active && (e.State & TreeNodeStates.Focused) != 0)
                ControlPaint.DrawFocusRectangle(e.Graphics, label, fore, back);

            Rectangle toggles = new Rectangle(togglesLeft, e.Bounds.Top, ClientSize.Width - togglesLeft, e.Bounds.Height);
            using (SolidBrush brush = new SolidBrush(BackColor))
                e.Graphics.FillRectangle(brush, toggles);
            if (node.HasVisibleToggle)
                DrawToggle(e.Graphics, _tree.IsVisible(node) ? _eyeOpen : _eyeClosed, togglesLeft, e.Bounds);
            if (node.HasLockToggle)
                DrawToggle(e.Graphics, _tree.IsLocked(node) ? _lockClosed : _lockOpen, togglesLeft + TOGGLE_WIDTH, e.Bounds);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                _italicFont?.Dispose();
                _italicFont = null;
            }
            base.Dispose(disposing);
        }

        private int TogglesLeft
        {
            get { return ClientSize.Width - 2 * TOGGLE_WIDTH; }
        }

        private Font ItalicFont
        {
            get
            {
                if (_italicFont == null)
                    _italicFont = new Font(Font, FontStyle.Italic);
                return _italicFont;
            }
        }

        private void UpdateItemHeight()
        {
            ItemHeight = Math.Max(Font.Height + 4, Math.Max(_eyeOpen.Height, _lockOpen.Height) + 3);
        }

        private static void DrawToggle(Graphics graphics, Image image, int columnLeft, Rectangle row)
        {
            graphics.DrawImage(image, columnLeft + (TOGGLE_WIDTH - image.Width) / 2,
                row.Top + (row.Height - image.Height) / 2, image.Width, image.Height);
        }

        private static Color Blend(Color from, Color to, double amount)
        {
            return Color.FromArgb(
                (int)(from.R + (to.R - from.R) * amount),
                (int)(from.G + (to.G - from.G) * amount),
                (int)(from.B + (to.B - from.B) * amount));
        }

        private RoomLayerNode FindToggleAt(Point point, out bool onEye)
        {
            onEye = false;
            int togglesLeft = TogglesLeft;
            if (point.X < togglesLeft || point.X >= ClientSize.Width)
                return null;
            RoomLayerNode node = GetNodeAt(point)?.Tag as RoomLayerNode;
            if (node == null)
                return null;
            onEye = point.X < togglesLeft + TOGGLE_WIDTH;
            return (onEye ? node.HasVisibleToggle : node.HasLockToggle) ? node : null;
        }

        private TreeNode CreateNode(RoomLayerNode node, bool withChildren)
        {
            TreeNode treeNode = new TreeNode(node.Text) { Name = node.Key, Tag = node };
            if (node.Kind == RoomLayerNodeKind.UnusedGroup)
                treeNode.ToolTipText = TIP_UNUSED_GROUP;
            else if (node.ItemState == RoomLayerItemState.NothingDrawn)
                treeNode.ToolTipText = TIP_NOTHING_DRAWN;
            else if (node.ItemState == RoomLayerItemState.Unused)
                treeNode.ToolTipText = TIP_UNUSED;
            if (withChildren)
            {
                foreach (RoomLayerNode child in node.Children)
                    treeNode.Nodes.Add(CreateNode(child, true));
            }
            return treeNode;
        }

        private void Expand(TreeNode node)
        {
            // A node keeps its state under a collapsed parent
            if (_expanded.Contains(node.Name))
                node.Expand();
            foreach (TreeNode child in node.Nodes)
                Expand(child);
        }

        private TreeNode FindNode(string key)
        {
            if (key == null)
                return null;
            TreeNode[] found = Nodes.Find(key, true);
            return found.Length > 0 ? found[0] : null;
        }
    }
}
