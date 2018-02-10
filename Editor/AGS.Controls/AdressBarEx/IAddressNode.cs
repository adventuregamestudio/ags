#region Using Statements

#region .NET Namespace

using System;
using System.Drawing;
using System.Windows.Forms;

#endregion

#endregion

namespace AddressBarExt
{
    /// <summary>
    /// Interface for a traversable node used in the AddressBarExt control.
    /// 
    /// Author : James Strain
    /// Email : leon_STARS@hotmail.com
    /// Tested Platforms : Windows Vista Ultimate x64 / WinXP  Pro 32-bit
    /// 
    /// Additional Work Needed :
    /// 
    /// None that I am aware of...
    /// 
    /// </summary>
    public interface IAddressNode
    {
        /// <summary>
        /// Gets/Sets the parent of this node
        /// </summary>
        IAddressNode Parent
        {
            get; set;           
        }

        /// <summary>
        /// Gets/Sets the Display name of this node
        /// </summary>
        String DisplayName
        {
            get;            
        }

        /// <summary>
        /// Gets the Icon that represents this node type.
        /// </summary>
        Icon Icon
        {
            get;
        }

        /// <summary>
        /// Gets the Unique ID for this node
        /// </summary>
        Object UniqueID
        {
            get;
        }

        /// <summary>
        /// Gets/Sets any user defined extra data for this node
        /// </summary>
        Object Tag
        {
            get;
            set;
        }

        /// <summary>
        /// Gets an array of Child Nodes
        /// </summary>
        IAddressNode[] Children
        {
            get;
        }

        /// <summary>
        /// Method that updates this node to gather all relevant detail.
        /// </summary>
        void UpdateNode();

        /// <summary>
        /// Returns a given child, based on a unique ID
        /// </summary>
        /// <param name="uniqueID">Unique ID to identify the child</param>
        /// <param name="recursive">Indicates if the search should recurse through childrens children..</param>
        /// <returns>Returns the child node</returns>
        IAddressNode GetChild(object uniqueID, bool recursive);

        /// <summary>
        /// Creates the node UI that will be shown in the drop down
        /// </summary>
        /// <returns>Returns the node UI that will be shown in the drop down</returns>
        ToolStripItem CreateNodeUI(EventHandler nodeClicked);        
    }
}
