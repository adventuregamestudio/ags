using System;
using System.Collections.Generic;

namespace AGS.Types
{
    /// <summary>
    /// InteractionEvent is a definition of a event triggered by a
    /// particular player interaction. Event has a Name (id) and a DisplayName (label).
    /// </summary>
    public class InteractionEvent
    {
        private int _index;
        private string _eventName;
        private string _displayName;
        private string _functionSuffix;

        public InteractionEvent(int index, string evtName, string displayName, string functionSuffix)
        {
            _index = index;
            _eventName = evtName;
            _displayName = displayName;
            _functionSuffix = functionSuffix;
        }

        /// <summary>
        /// Get a numeric interaction index, matching the Cursor's ID
        /// </summary>
        public int Index { get { return _index; } }
        public string EventName { get { return _eventName; } }
        public string DisplayName { get { return _displayName; } }
        public string FunctionSuffix { get { return _functionSuffix; } }
    }

    public class InteractionSchemaChangedEventArgs
    {
        Dictionary<string, string> _eventNameRemap;

        public InteractionSchemaChangedEventArgs()
        {
            _eventNameRemap = new Dictionary<string, string>();
        }

        public InteractionSchemaChangedEventArgs(Dictionary<string, string> eventNameRemap)
        {
            _eventNameRemap = eventNameRemap;
        }

        public Dictionary<string, string> EventNameRemap { get { return _eventNameRemap; } }
    }

    /// <summary>
    /// InteractionSchema contains definitions of the Interaction Events in game.
    /// Interaction Events are sort of a direct player interactions with game objects.
    /// Each one of them may correspond to a object event.
    /// Normally these are gathered from the game Cursors.
    /// </summary>
    public class InteractionSchema
    {
        private InteractionEvent[] _events = new InteractionEvent[0];

        public InteractionEvent[] Events
        {
            get
            {
                return _events;
            }
            set
            {
                _events = value;
                // TODO: make a proper matching, based on change operation (add/remove/rename)
                Dictionary<string, string> remap = new Dictionary<string, string>();
                foreach (var evt in _events)
                    remap.Add(evt.EventName, evt.EventName);
                Changed?.Invoke(this, new InteractionSchemaChangedEventArgs(remap));
            }
        }

        public delegate void SchemaChanged(object sender, InteractionSchemaChangedEventArgs args);
        public event SchemaChanged Changed;

        // TODO: had to make a static instance of InteractionSchema for now,
        // because all game objects must assign it to their Interactions instance
        // on creation.
        // Will need to investigate and find a good way to resolve this;
        // after that InteractionSchema can become a member of Game, for example.
        private static InteractionSchema _instance;
        public static InteractionSchema Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new InteractionSchema();
                return _instance;
            }
        }
    }
}
