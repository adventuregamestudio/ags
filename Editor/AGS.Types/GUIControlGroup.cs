using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    [Serializable]
    public class GUIControlGroup : List<GUIControl>
    {
        public void AddToGroup(GUIControl gc)
        {
            if (!Contains(gc))
            {
                Add(gc);
                gc.MemberOf = this;
            }
        }

        public void RemoveFromGroup(GUIControl gc)
        {
            Remove(gc);
            gc.MemberOf = null;
        }

        public void Update()
        {
            for (int i = Count - 1; i >= 0; i--)
            {
                if (this[i].MemberOf != this) RemoveAt(i);
            }
        }

        public void ClearGroup()
        {
            foreach (GUIControl gc in this)
            {
                gc.MemberOf = null;
            }
            Clear();
        }
    }
}
