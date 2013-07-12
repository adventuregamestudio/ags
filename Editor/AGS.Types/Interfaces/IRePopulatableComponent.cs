using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IRePopulatableComponent
    {
        void RePopulateTreeView();
        void RePopulateTreeView(string selectedNode);
    }
}
