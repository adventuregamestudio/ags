using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class Factory
    {
        internal static GUIController GUIController
        {
            get { return GUIController.Instance; }
        }

        internal static AGSEditor AGSEditor
        {
            get { return AGSEditor.Instance; }
        }

		internal static ComponentController ComponentController
		{
			get { return ComponentController.Instance; }
		}

		internal static EditorEvents Events
		{
			get { return EditorEvents.Instance; }
		}

        internal static ToolBarManager ToolBarManager
        {
            get { return GUIController.ToolBarManager; }
        }

        internal static MainMenuManager MenuManager
        {
            get { return GUIController.MenuManager; }
        }

        internal static NativeProxy NativeProxy
        {
            get { return NativeProxy.Instance; }
        }
    }
}
