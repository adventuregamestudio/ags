using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;

namespace AGS.Types
{
    public class GameObjectEvent
    {
        public string _name;

        public GameObjectEvent(string name)
        {
            _name = name;
        }

        public string Name { get { return _name; } }
    }

    /// <summary>
    /// GameObjectEvents gathers a map of events from the game class,
    /// using reflection for event properties and accessing Interactions
    /// instance if an object type provides one.
    ///
    /// TODO: consider gather PropertyDescriptorCollection here as well,
    /// then this class may have all the event-gathering code and used
    /// in PropertyTabEvents too.
    ///
    /// TODO: consider redesigning this into a non-static class that
    /// parses and records properties of a Type, and then can read
    /// script function values from an Object of that Type. This may
    /// make things more optimal and faster.
    /// </summary>
    public static class GameObjectEvents
    {
        public static Dictionary<string, GameObjectEvent> GetEvents(object obj)
        {
            return GetEvents(obj.GetType());
        }

        public static Dictionary<string, GameObjectEvent> GetEvents(Type objType)
        {
            Dictionary<string, GameObjectEvent> events = new Dictionary<string, GameObjectEvent>();
            PropertyDescriptorCollection eventProps =
                TypeDescriptor.GetProperties(objType, new Attribute[] { new AGSEventPropertyAttribute() });
            foreach (PropertyDescriptor p in eventProps)
                events.Add(p.Name, new GameObjectEvent(p.Name));
            GetInteractionProperties(objType, events);
            return events;
        }

        public static Dictionary<string, string> GetScriptFunctions(object obj)
        {
            Dictionary<string, string> scriptFunctions = new Dictionary<string, string>();
            PropertyDescriptorCollection eventProps =
                TypeDescriptor.GetProperties(obj.GetType(), new Attribute[] { new AGSEventPropertyAttribute() });
            foreach (PropertyDescriptor p in eventProps)
            {
                scriptFunctions.Add(p.Name, (string)p.GetValue(obj));
            }
            GetInteractionFunctions(obj, scriptFunctions);
            return scriptFunctions;
        }

        private static void GetInteractionProperties(Type objType, Dictionary<string, GameObjectEvent> events)
        {
            PropertyInfo interactionsProperty = objType.GetProperty("Interactions");
            if (interactionsProperty != null)
            {
                // "Interactions" property must NOT have a Obsolete attribute,
                // otherwise it's considered to be disabled.
                if (interactionsProperty.GetCustomAttribute(typeof(ObsoleteAttribute)) != null)
                    return;

                var schema = InteractionSchema.Instance;
                var interEvents = schema.Events;
                foreach (var evt in interEvents)
                {
                    events.Add(evt.EventName, new GameObjectEvent(evt.EventName));
                }
            }
        }

        private static void GetInteractionFunctions(object obj, Dictionary<string, string> scriptFunctions)
        {
            PropertyInfo interactionsProperty = obj.GetType().GetProperty("Interactions");
            if (interactionsProperty != null)
            {
                // "Interactions" property must NOT have a Obsolete attribute,
                // otherwise it's considered to be disabled.
                if (interactionsProperty.GetCustomAttribute(typeof(ObsoleteAttribute)) != null)
                    return;

                Interactions interactions = (Interactions)interactionsProperty.GetValue(obj, null);
                foreach (var fn in interactions.ScriptFunctionNames)
                    scriptFunctions.Add(fn.Key, fn.Value);
            }
        }
    }
}
