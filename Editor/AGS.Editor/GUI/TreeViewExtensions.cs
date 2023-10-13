using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// Extender methods for the TreeView control.
    /// 
    /// Node state recovery is taken from
    /// https://stackoverflow.com/a/20081867
    /// </summary>
    public static class TreeViewExtensions
    {
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
    }
}
