using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class ViewFolder : BaseFolderCollection<View, ViewFolder>, IViewFolder
    {
        public ViewFolder(string name) : base(name) { }

        public ViewFolder() : this("Default") { }

        public ViewFolder(XmlNode node) : base(node) { }

        public IList<View> Views
        {
            get { return _items; }
        }

        /// <summary>
		/// Finds the View object for the specified view number.
		/// Returns null if the view is not found.
		/// </summary>
		/// <param name="viewNumber">View number to look for</param>
		/// <param name="recursive">Whether to also search sub-folders</param>
		public View FindViewByID(int viewNumber, bool recursive)
		{
            return FindItem(IsItem, viewNumber, recursive);			
		}

        public override ViewFolder CreateChildFolder(string name)
        {
            return new ViewFolder(name);
        }
        
        protected override ViewFolder CreateFolder(XmlNode node)
        {
            return new ViewFolder(node);
        }

        protected override View CreateItem(XmlNode node)
        {
            return new View(node);
        }

        private bool IsItem(View view, int viewNumber)
        {
            return view.ID == viewNumber;
        }		
    }
}
