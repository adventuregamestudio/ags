using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class MenuCommands : IComparable<MenuCommands>
    {
        private string _menuName;
        private string _insertAfterMenu;
        private IList<MenuCommand> _commands;
        private int _priority = -1;

        /// <summary>
        /// Creates a new MenuCommands collection
        /// </summary>
        /// <param name="menuName">The Menu ID to add the commands to</param>
        /// <param name="insertAfterMenu">For pane-specific menus, where to insert the extra menu</param>
        public MenuCommands(string menuName, string insertAfterMenu)
        {
            _menuName = menuName;
            _insertAfterMenu = insertAfterMenu;
            _commands = new List<MenuCommand>();
        }

        /// <summary>
        /// Creates a new MenuCommands collection
        /// </summary>
        /// <param name="menuName">The Menu ID to add the commands to</param>
        /// <param name="priority">The relative position of this set of commands on the menu</param>
        public MenuCommands(string menuName, int priority)
        {
            _menuName = menuName;
            _insertAfterMenu = null;
            _priority = priority;
            _commands = new List<MenuCommand>();
        }

        /// <summary>
        /// Creates a new MenuCommands collection to wrap an existing list of commands
        /// </summary>
        /// <param name="commands"></param>
        public MenuCommands(IList<MenuCommand> commands)
        {
            _menuName = null;
            _insertAfterMenu = null;
            _commands = commands;
        }

        /// <summary>
        /// Creates a new MenuCommands collection
        /// </summary>
        /// <param name="menuName">The Menu ID to add the commands to</param>
        public MenuCommands(string menuName) : this(menuName, null)
        {
        }

        public string MenuName
        {
            get { return _menuName; }
        }

        public string InsertAfterMenu
        {
            get { return _insertAfterMenu; }
        }

        public IList<MenuCommand> Commands
        {
            get { return _commands; }
        }


        #region IComparable<MenuCommands> Members

        public int CompareTo(MenuCommands other)
        {
            return this._priority - other._priority;
        }

        #endregion
    }
}
