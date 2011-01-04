using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public interface IViewFolder
	{
		string Name { get; }
		/// <summary>
		/// Sub folders containing their own views.
		/// </summary>
        IList<ViewFolder> SubFolders { get; }
		/// <summary>
		/// The views contained in this folder.
		/// </summary>
		IList<View> Views { get; }
		/// <summary>
		/// Finds the View object for the specified view number.
		/// Returns null if the view is not found.
		/// </summary>
		/// <param name="viewNumber">View number to look for</param>
		/// <param name="recursive">Whether to also search sub-folders</param>
		View FindViewByID(int viewNumber, bool recursive);
	}
}
