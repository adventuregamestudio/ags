using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;

namespace AGS.Types
{
    public class GUIIndexTypeConverter : BaseListSelectTypeConverter<int, string>
    {
        protected static Dictionary<int, string> _allGUIs = new Dictionary<int, string>();
        protected static Dictionary<int, string> _normalGUIs = new Dictionary<int, string>();
        protected static Dictionary<int, string> _textWindowGUIs = new Dictionary<int, string>();
        private static IList<GUI> _GUIs = null;

        protected override Dictionary<int, string> GetValueList(ITypeDescriptorContext context)
        {
            return _allGUIs;
        }

        public static void SetGUIList(IList<GUI> guis)
        {
            // Keep a reference to the list so it can be updated whenever we need to
            _GUIs = guis;
            RefreshGUIList();
        }

        public static void RefreshGUIList()
        {
            if (_GUIs == null)
            {
                throw new InvalidOperationException("Static collection has not been set");
            }

            _allGUIs.Clear();
            _normalGUIs.Clear();
            _textWindowGUIs.Clear();
            _allGUIs.Add(0, "(None)");
            _normalGUIs.Add(0, "(None)");
            _textWindowGUIs.Add(0, "(None)");

            foreach (GUI gui in _GUIs.OrderBy(a => a.Name))
            {
                // Skip gui 0, because 0 is used as "no selection" in properties
                if (gui.ID == 0)
                    continue;

                _allGUIs.Add(gui.ID, gui.Name);
                if (gui is NormalGUI)
                    _normalGUIs.Add(gui.ID, gui.Name);
                else if (gui is TextWindowGUI)
                    _textWindowGUIs.Add(gui.ID, gui.Name);
            }
        }
    }

    public class GUINormalIndexTypeConverter : GUIIndexTypeConverter
    {
        protected override Dictionary<int, string> GetValueList(ITypeDescriptorContext context)
        {
            return _normalGUIs;
        }
    }

    public class GUITextWindowIndexTypeConverter : GUIIndexTypeConverter
    {
        protected override Dictionary<int, string> GetValueList(ITypeDescriptorContext context)
        {
            return _textWindowGUIs;
        }
    }
}
