#region Using Statements

#region .NET Namespace

using System;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

#endregion

#endregion

namespace AddressBarExt.Controls
{
    /*
     * There is some extra error handling in most of the methods that (in theory) is redundant. 
     * For performance reasons, you could remove alot of this information.
     * 
     * Additionally, the event handling code is still a bit messy. Particularly NodeButtonClicked.
     * 
     * Finally, a big performance bottleneck is present in the AddToolStripItemUpdate method.
     * When it first generates the drop down menus, it hits a bottleneck in creating the collecting of items.
     * 
     * Not sure how to resolve this, as I'm still unsure of how this control meets with the rest of Shinobi :)
     * 
     * -James
     */

    /// <summary>
    /// Custom Control that acts as a Vista Style Address bar.
    /// 
    /// Author : James Strain
    /// Email : leon_STARS@hotmail.com
    /// Tested Platforms : Windows Vista Ultimate x64 / WinXP  Pro 32-bit
    /// 
    /// Additional Work Needed :
    /// 
    /// Optimization for UI Performance
    /// Re-factoring of code based on any optimization
    /// Possible removal of 'redundant' error coding to speed up method calls
    /// Change caching system to reduce un-used DropDownMenus after period of time (currently retains them forever)
    /// Re-organize the "update" algorithm to not re-generate all of the nodes every upadte. Should only insert new nodes and push old ones into the overflow.
    /// 
    /// </summary>
    public partial class AddressBarExt : UserControl
    {
        #region Delegates

        /// <summary>
        /// Delegate for handling when a new node is selected
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="nca">Event Arguments</param>
        public delegate void SelectionChanged(object sender, NodeChangedArgs nca);

        /// <summary>
        /// Delegate for handling a node double click event.
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="nca">Event arguments</param>
        public delegate void NodeDoubleClicked(object sender, NodeChangedArgs nca);

        #endregion

        #region Event Declaration

        /// <summary>
        /// Stores the callback function for when the selected node changes
        /// </summary>
        public event SelectionChanged SelectionChange = null;

        /// <summary>
        /// Stores the callback function for when the selected node changes
        /// </summary>
        public event NodeDoubleClicked NodeDoubleClick = null;

        #endregion

        #region Class Variables

        /// <summary>
        /// Stores the root node.
        /// </summary>
        IAddressNode rootNode = null;

        /// <summary>
        /// Node contains the currently selected node (e.g. previous node)
        /// </summary>
        IAddressNode currentNode = null;

        /// <summary>
        /// Stores the default font for this control
        /// </summary>
        Font baseFont = null;

        /// <summary>
        /// Holds the font style that is used when an item is selected
        /// </summary>
        FontStyle selectedStyle = (FontStyle.Bold | FontStyle.Underline);

        /// <summary>
        /// Drop down menu that contains the overflow menu
        /// </summary>
        ToolStripDropDownMenu tsddOverflow = null;

        #endregion

        #region Properties

        /// <summary>
        /// Gets/Sets the font style for when a node is selected
        /// </summary>
        public FontStyle SelectedStyle
        {
            get { return this.selectedStyle; }
            set { this.selectedStyle = value; }
        }

        /// <summary>
        /// Gets/Sets the currently selected node. Validates upon set and updates the bar
        /// </summary>
        public IAddressNode CurrentNode
        {
            get { return this.currentNode; }
            set 
            {
                if (this.currentNode == value) return;

                this.currentNode = value;
                UpdateBar();

                //fire the change event
                if (SelectionChange != null)
                {
                    NodeChangedArgs nca = new NodeChangedArgs(currentNode.UniqueID);
                    SelectionChange(this, nca);
                }
            }
        }

        /// <summary>
        /// Gets/Sets the root node. Upon setting the root node, it resets the hierarchy.
        /// </summary>
        public IAddressNode RootNode
        {
            get { return this.rootNode; }
            set { InitializeRoot(value); }
        }

        #endregion

        #region Constructor/s

        /// <summary>
        /// Base contructor for the AddressBarExt Control.
        /// </summary>
        public AddressBarExt()
        {
            //Windows Forms code
            InitializeComponent();

            //get the basic font
            this.baseFont = this.Font;
        }

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes this AddressBarExt with a given root node
        /// </summary>
        /// <param name="rootNode"></param>
        public void InitializeRoot(IAddressNode rootNode)
        {
            //remove all items
            ts_bar.Items.Clear();
            this.rootNode = null;

            if (rootNode != null)
            {
                //create the root node
                this.rootNode = rootNode;//.Clone();

                //force update the node
                this.rootNode.UpdateNode();

                //set the current node to be the root
                this.currentNode = this.rootNode;

                //update the address bar
                UpdateBar();
            }
        }

        #endregion

        #region Event Handlers

        /// <summary>
        /// Method to handle when a child has been selected from a node
        /// </summary>
        /// <param name="sender">Sender of this Event</param>
        /// <param name="e">Event Arguments</param>
        private void NodeButtonClicked(Object sender, EventArgs e)
        {
            ToolStripItem tsb = sender as ToolStripItem;
            if (tsb != null)
            {
                //check we have the right tag type
                if (tsb.Tag != null && tsb.Tag.GetType() == rootNode.GetType())
                {
                    //set the current node
                    currentNode = (IAddressNode)tsb.Tag;

                    //update the address bar
                    UpdateBar();

                    //if the selection changed event is handled
                    if (SelectionChange != null)
                    {
                        //throw the event
                        NodeChangedArgs nca = new NodeChangedArgs(currentNode.UniqueID);
                        SelectionChange(this, nca);
                    }
                }

                return;
            }
        }

        /// <summary>
        /// Method to handle when a node is double clicked
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Event arguments</param>
        private void NodeDoubleClickHandler(Object sender, EventArgs e)
        {
            //check we are handlign the double click event
            if (NodeDoubleClick != null && sender.GetType() == typeof(ToolStripButton))
            {
                //get the node from the tag
                currentNode = (IAddressNode)((ToolStripButton)sender).Tag;

                //create the node changed event arguments
                NodeChangedArgs nca = new NodeChangedArgs(currentNode.UniqueID);

                //fire the event
                NodeDoubleClick(this, nca);
            }
        }

        /// <summary>
        /// Method to handle when the bar is double clicked
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Event arguments</param>
        private void BarDoubleClickHandler(object sender, EventArgs e)
        {
            this.OnDoubleClick(e);
        }

        /// <summary>
        /// Handles when the overflow menu should be entirely destroyed
        /// </summary>
        /// <param name="sender">Sender of this Event</param>
        /// <param name="e">Event arguments</param>
        private void OverflowDestroyed(Object sender, EventArgs e)
        {
            //check we have the right type
            if (sender.GetType() == typeof(ToolStripDropDownButton))
            {
                //get the button as the right type
                ToolStripDropDownButton tsddb = (ToolStripDropDownButton)sender;

                //if the button is no longer visible
                if (!tsddb.Visible)
                    //clear all items from the overflow
                    this.tsddOverflow.Items.Clear();
            }
        }

        /// <summary>
        /// Method handler using the middle mouse wheel to scroll the drop down menus
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Event arguments</param>
        private void ScrollDropDownMenu(Object sender, MouseEventArgs e)
        {
            //if we have the right type
            if (sender.GetType() == typeof(ToolStripDropDownMenu))
            {
                //This doesn't work :(

                Point prev = ((ToolStripDropDownMenu)sender).AutoScrollOffset;
                prev.Y += (e.Delta);
                ((ToolStripDropDownMenu)sender).AutoScrollOffset = prev;

            }
        }

        /// <summary>
        /// Method that puts focus onto a given ToolStripDropDownMenu
        /// </summary>
        /// <param name="sender">Sender of this event</param>
        /// <param name="e">Event Arguments</param>
        private void GiveToolStripDropDownMenuFocus(Object sender, EventArgs e)
        {
            if (sender.GetType() == typeof(ToolStripDropDownMenu))
            {
                //focus on the item
                ((ToolStripDropDownMenu)sender).Focus();
            }
        }

        #endregion

        #region Update

        /// <summary>
        /// Creates a new tool strip menu item based on a given node
        /// </summary>
        /// <param name="node">The node to base the item on</param>
        /// <returns>Built ToolStripItem. Returns Null if method failed</returns>
        private void AddToolStripItemUpdate(ref IAddressNode node, int position)
        {
            //variables needed for our toolstrip
            ToolStripButton tsButton = null;
            ToolStripDropDownButton tsddButton = null;
            ToolStripDropDownMenu tsDropDown = null;

            //update the node
            node.UpdateNode();

            if (node.Icon != null)
                tsButton = new ToolStripButton(node.DisplayName, node.Icon.ToBitmap(), NodeButtonClicked);
            else
                tsButton = new ToolStripButton(node.DisplayName, null, NodeButtonClicked);

            //attach the node as the tag
            tsButton.Tag = node;

            //align the image
            tsButton.ImageAlign = ContentAlignment.TopCenter;

            //enable double clicks
            tsButton.DoubleClickEnabled = true;

            //add the double click handler
            tsButton.DoubleClick += new EventHandler(NodeDoubleClickHandler);

            if (position < 0)
                ts_bar.Items.Add(tsButton);
            else
                ts_bar.Items.Insert(0, tsButton);

            try
            {

                //if we have any children
                if (node.Children.Length > 0)
                {
                    //create the drop down button
                    tsddButton = new ToolStripDropDownButton("");

                    //check if we have any tag data (we cache already built drop down items in the node TAG data.
                    if (node.Tag == null)
                    {
                        IAddressNode curNode = null;

                        //create the drop down menu
                        tsDropDown = new ToolStripDropDownMenu();

                        //Some variables to let the drawing happen smoothly
                        tsDropDown.LayoutStyle = ToolStripLayoutStyle.VerticalStackWithOverflow;
                        tsDropDown.MaximumSize = new Size(1000, 400);
                        tsDropDown.ShowImageMargin = false;
                        tsDropDown.ShowCheckMargin = false;                        
                                                
                        /*
                         * This is the primary bottleneck for this app, creating all the necessary drop-down menu items.
                         * 
                         * To optimize performance of this control, this is the main area that needs tuning.
                         */

                        //for all child nodes
                        for (int i = 0; i < node.Children.Length; i++)
                        {
                            curNode = (IAddressNode)node.Children.GetValue(i);

                            ToolStripItem tsb = curNode.CreateNodeUI(NodeButtonClicked);                            
                            //assign the child node as the tag
                            tsb.Tag = curNode;

                            //if the node we are working on is the current node
                            if (curNode == currentNode)
                                //set the font to indicate it is selected
                                tsb.Font = new Font(tsb.Font, this.selectedStyle);

                            //enable overflow on the buttons
                            ToolStripMenuItem menuItem = tsb as ToolStripMenuItem;
                            if (menuItem != null) menuItem.Overflow = ToolStripItemOverflow.AsNeeded;

                            //add the item to the drop down list                            
                            tsDropDown.Items.Add(tsb); //THIS IS THE BIGGEST BOTTLENECK. LOTS OF TIME SPENT IN/CALLING THIS METHOD!
                        }

                        //assign the tag
                        node.Tag = tsDropDown;

                        //Some variables to let the drawing happen smoothly
                        tsDropDown.LayoutStyle = ToolStripLayoutStyle.Table;
                        tsDropDown.MaximumSize = new Size(1000, 400);

                        //assign the parent
                        tsDropDown.Tag = tsddButton;

                        //add the method handler for the mouse wheel scrolling
                        tsDropDown.MouseWheel += new MouseEventHandler(ScrollDropDownMenu);

                        //handle the mouse entering/leaving the control
                        tsDropDown.MouseEnter += new EventHandler(GiveToolStripDropDownMenuFocus);
                    }
                    else
                    {
                        if(node.Tag.GetType() == typeof(ToolStripDropDownMenu))
                        {
                            tsDropDown = (ToolStripDropDownMenu)node.Tag;

                            foreach (ToolStripItem tsmi in tsDropDown.Items)
                            {
                                if (tsmi.Font.Style != baseFont.Style)
                                    tsmi.Font = baseFont;
                            }
                        }
                    }

                    //assign the drop down list
                    tsddButton.DropDown = tsDropDown;

                    //set it to ignore text rendering
                    tsddButton.DisplayStyle = ToolStripItemDisplayStyle.None;

                    //align the image
                    tsddButton.ImageAlign = ContentAlignment.TopCenter;

                    //giving right margin to avoid drop down hiding itself when accidentally slightly moving the cursor to the button on the right
                    tsddButton.Margin = new Padding(0, 0, 5, 0);

                    //add it to the bar
                    if (position < 0)
                        ts_bar.Items.Add(tsddButton);
                    else
                        ts_bar.Items.Insert(1, tsddButton);
                }
            }
            catch (System.NullReferenceException nrex)
            {
                System.Console.Error.WriteLine(nrex.Message);
            }
        }

        /// <summary>
        /// Updates the address bar
        /// </summary>
        private void UpdateBar()
        {
            //check we have a valid root
            if (rootNode != null)
            {
                //update the current node, if it doesn't exist
                if (currentNode == null)
                    currentNode = rootNode;

                //now we know we have a root node, we need to build a relationship all the way from the current node, to the root.
                IAddressNode tempNode = currentNode;
                int steps = 1;
 
                //we step up through each parent until we hit a root node
                while (true)
                {
                    //check we aren't root
                    if(tempNode.Parent != null)
                    {
                        tempNode = tempNode.Parent;
                        steps++;
                    }else
                        break;
                }

                //check we have a valid traversal
                if (tempNode != rootNode)
                {
                    //traversal was invalid, so we set the current node to be the root node, and we traverse again
                    currentNode = rootNode;

                    //recursive call
                    UpdateBar();
                }
                else
                {
                    //set the current node
                    tempNode = currentNode;

                    //remove all the items
                    ts_bar.Items.Clear();

                    //we had a valid node, so we can start adding elements to the view
                    for (int i = 0; i < steps; i++)
                    {
                        //add it to the beginning of the bar ;)
                        AddToolStripItemUpdate(ref tempNode, 0);

                        if (tempNode.Parent == null)
                            rootNode = tempNode;

                        tempNode = tempNode.Parent;
                    }

                    //check if we have too many nodes
                    if (TooManyNodes())
                    {
                        //create the drop down menu for the overflow
                        CreateOverflowDropDown();
                        AddOrUpdateOverflow();
                    }
                }
            }
        }

        /// <summary>
        /// Method to create the overflow element for handling
        /// </summary>
        private void CreateOverflowDropDown()
        {
            if (ts_bar != null)
            {
                //ensure we have an overflow
                if (this.tsddOverflow == null)
                {
                    tsddOverflow = new ToolStripDropDownMenu();
                    tsddOverflow.MaximumSize = new Size(1000, 400);
                }
                else
                {
                    tsddOverflow.Items.Clear();
                }

                ToolStripDropDownButton currentDrop = null;
                ToolStripItem tsi = null;
                ToolStripItem currentItem = null;

                // whilst we have too many nodes to render
                while (TooManyNodes())
                {
                    // create a menu item for our  drop down
                    tsi = new ToolStripMenuItem(ts_bar.Items[0].Text, ts_bar.Items[0].Image, NodeButtonClicked);
                    tsi.Tag = ts_bar.Items[0].Tag;

                    //if we have a valid button
                    if (ts_bar.Items[0].GetType() == typeof(ToolStripButton))
                    {
                        //add the item to the overflow
                        tsddOverflow.Items.Add(tsi);
                    }

                    // remove our item from the normal bar
                    ts_bar.Items.RemoveAt(0);

                    if (ts_bar.Items.Count > 0)
                    {
                        // remove the items drop-down as-well :)
                        ts_bar.Items.RemoveAt(0);
                    }
                }
                //for each item in the bar
                //for(int i= ts_bar.Items.Count -1; i>-1; i--) 
                //{
                //    tsi = new ToolStripMenuItem(ts_bar.Items[i].Text,ts_bar.Items[i].Image,NodeButtonClicked);
                //    tsi.Tag = ts_bar.Items[i].Tag;

                //    //if we have a valid button
                //    if (ts_bar.Items[i].GetType() == typeof(ToolStripButton))
                //        //if we have a valid tag
                //        if (tsi.Tag.GetType() == currentNode.GetType())
                //            //for all nodes but the current node
                //            if (((IAddressNode)tsi.Tag) != currentNode)
                //            {
                //                //if our overflow doesn't already contain this item
                //                if (!tsddOverflow.Items.Contains(tsi))
                //                { 
                //                    //add the item to the overflow
                //                    tsddOverflow.Items.Add(tsi);
                //                }
                //            }
                //            else
                //            {
                //                currentItem = ts_bar.Items[i];

                //                if (currentNode.Children.Length > 0)
                //                {
                //                    currentDrop = ts_bar.Items[i + 1] as ToolStripDropDownButton;
                //                    tsi.Tag = ts_bar.Tag;
                //                }
                //            }
                //}

                //clear all items from the bar
                //ts_bar.Items.Clear();

                //add the current item
                //ts_bar.Items.Add(currentItem);

                //add the current drop down button, if applicable
                //if(currentDrop != null)
                    //ts_bar.Items.Add(currentDrop);
            }
        }

        /// <summary>
        /// Method adds the overflow menu to the drop down list
        /// </summary>
        private void AddOrUpdateOverflow()
        {
            //if we have a valid overflow
            if (tsddOverflow != null)
            {
                if (ts_bar.Items[0].GetType() == typeof(ToolStripDropDownButton))
                {
                    // do nothing, since we updated our overflow elsewhere :)
                }
                else
                {
                    //create the overflow button
                    ToolStripDropDownButton tsd = new ToolStripDropDownButton("..");

                    //add the drop down
                    tsd.DropDown = tsddOverflow;

                    //add that to the bar
                    ts_bar.Items.Insert(0, tsd);

                    //add the event handler for destruction
                    tsd.OwnerChanged += new EventHandler(OverflowDestroyed);
                }
            }
        }

        /// <summary>
        /// Method that calculates if we have too many nodes in our address bar
        /// </summary>
        /// <returns>Boolean indicating </returns>
        private bool TooManyNodes()
        {
            if (ts_bar.Items.Count == 0) return false;
            //check if the last item has overflowed
            return ts_bar.Items[ts_bar.Items.Count - 1].IsOnOverflow;
        }

        #endregion
    }

    /// <summary>
    /// Custom Event Arguments for when a node has been changed
    /// </summary>
    public class NodeChangedArgs : EventArgs
    {
        #region Class Variables

        /// <summary>
        /// Stores the Unique ID of the newly opened node
        /// </summary>
        private object oUniqueId = null;

        #endregion

        #region Properties

        /// <summary>
        /// Gets the UniqueID from the newly opened node
        /// </summary>
        public object OUniqueID
        {
            get { return this.oUniqueId; }
        }

        #endregion

        #region Constructor

        /// <summary>
        /// Base constructor for when a node selection is changed
        /// </summary>
        /// <param name="uniqueId">Unique Identifier for this node. Controled by IAddressNode implementation used.</param>
        public NodeChangedArgs(object uniqueId)
        {
            //set the values for the args
            this.oUniqueId = uniqueId;
        }

        #endregion
    }
}
