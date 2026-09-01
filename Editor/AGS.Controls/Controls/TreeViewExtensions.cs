using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace AGS.Controls
{
    /// <summary>
    /// Extender methods for the TreeView control.
    /// 
    /// Node state recovery is taken from
    /// https://stackoverflow.com/a/20081867
    /// </summary>
    public static class TreeViewExtensions
    {
        private const int TVM_SETEXTENDEDSTYLE = 0x1100 + 44;
        private const int TVM_GETEXTENDEDSTYLE = 0x1100 + 45;
        private const int TVS_EX_DOUBLEBUFFER = 0x0004;

        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wp, IntPtr lp);

        /// <summary>
        /// Enables double buffering so that the control does not flicker.
        /// 
        /// See <see href="https://stackoverflow.com/a/10364283/20494">this StackOverflow answer</see> for more details.
        /// </summary>
        /// <param name="control"></param>
        public static void EnableDoubleBuffering(this TreeView control)
        {
            SendMessage(control.Handle, TVM_SETEXTENDEDSTYLE, (IntPtr)TVS_EX_DOUBLEBUFFER, (IntPtr)TVS_EX_DOUBLEBUFFER);
        }

        public static List<string> GetExpansionState(this TreeNodeCollection nodes)
        {
            return nodes.Descendants()
                        .Where(n => n.IsExpanded)
                        .Select(n => n.FullPath)
                        .ToList();
        }

        public static List<string> GetExpansionState(this TreeNodeCollection nodes, Func<TreeNode, string> selectBy)
        {
            return nodes.Descendants()
                        .Where(n => n.IsExpanded)
                        .Select(selectBy)
                        .ToList();
        }

        public static void SetExpansionState(this TreeNodeCollection nodes, List<string> savedExpansionState)
        {
            foreach (var node in nodes.Descendants()
                                      .Where(n => savedExpansionState.Contains(n.FullPath)))
            {
                node.Expand();
            }
        }

        public static void SetExpansionState(this TreeNodeCollection nodes, List<string> savedExpansionState,
            Func<TreeNode, bool> selectBy)
        {
            foreach (var node in nodes.Descendants().Where(selectBy))
            {
                node.Expand();
            }
        }

        public static IEnumerable<TreeNode> Descendants(this TreeNodeCollection c)
        {
            foreach (var node in c.OfType<TreeNode>())
            {
                yield return node;

                foreach (var child in node.Nodes.Descendants())
                {
                    yield return child;
                }
            }
        }

        /// <summary>
        /// Tells whether the given node is a descendant of otherNode.
        /// Returns positive also if both references refer to the same node.
        /// </summary>
        public static bool IsDescendantOf(this TreeNode node, TreeNode otherNode)
        {
            while (node != null)
            {
                if (node == otherNode)
                    return true;
                node = node.Parent;
            }
            return false;
        }

        /// <summary>
        /// Finds a TreeNode inside TreeNodeCollection using a unique case-sensitive key.
        /// </summary>
        public static TreeNode FindUnique(this TreeNodeCollection nodes, string name, bool searchAllChildren)
        {
            return nodes.Find(name, searchAllChildren).FirstOrDefault(n => n.Name == name);
        }
    }
}
