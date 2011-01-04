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
			foreach (View view in _items)
			{
				if (view.ID == viewNumber)
				{
					return view;
				}
			}

			if (recursive)
			{
				foreach (ViewFolder subFolder in this.SubFolders)
				{
					View found = subFolder.FindViewByID(viewNumber, recursive);
					if (found != null)
					{
						return found;
					}
				}
			}
			return null;
		}


        protected override ViewFolder CreateFolder(XmlNode node)
        {
            return new ViewFolder(node);
        }

        protected override View CreateItem(XmlNode node)
        {
            return new View(node);
        }
    }
}
