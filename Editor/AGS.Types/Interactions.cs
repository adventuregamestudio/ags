using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Interactions is a table of script function names mapped to the
    /// object events. The list of events is defined by InteractionSchema.
    /// While InteractionSchema is attached to the object *type*,
    /// Interactions object is attached to individual object instances.
    /// </summary>
    public class Interactions
    {
        public const string XML_VERSION = "4.0.0.21";

        private InteractionSchema _schema;
        private string _scriptModule = string.Empty;
        // Map event name to a script function
        private Dictionary<string, string> _scriptFunctionNames;
        // Event index to function name lookup, for compatibility with old serialization formats
        // FIXME: figure out if it's possible to get rid of these in the class;
        // have them passed as arguments to a function, for example, or stored elsewhere.
        // Because they are only needed when loading old projects, and not useful after.
        private Dictionary<int, string> _indexedFunctionNames;

        public Interactions(InteractionSchema schema)
        {
            _schema = schema;
            _scriptFunctionNames = new Dictionary<string, string>();
            _indexedFunctionNames = new Dictionary<int, string>();
            if (_schema != null)
                _schema.Changed += Schema_Changed;
        }

        public InteractionSchema Schema
        {
            get { return _schema; }
            set
            {
                if (_schema == value)
                    return;
                if (_schema != null)
                    _schema.Changed -= Schema_Changed;
                _schema = value;
                if (_schema != null)
                    _schema.Changed += Schema_Changed;

                if (_indexedFunctionNames.Count > 0 && _schema != null && _schema.Events.Length > 0)
                {
                    ResolveIndexedFunctions();
                }
            }
        }

        /// <summary>
        /// An optional ScriptModule reference, which may be loaded
        /// from an older version of Interactions instance.
        /// </summary>
        public string ScriptModule
        {
            get { return _scriptModule; }
            set { _scriptModule = value; }
        }

        public Dictionary<string, string> ScriptFunctionNames
        {
            get { return _scriptFunctionNames; }
        }

        public IReadOnlyDictionary<int, string> IndexedFunctionNames
        {
            get { return _indexedFunctionNames; }
        }

        public void FromXml(XmlNode node)
        {
            _scriptFunctionNames = new Dictionary<string, string>();
            _indexedFunctionNames = new Dictionary<int, string>();

            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "Interactions"))
            {
                if (child.Name != "Event")
                {
                    if (child.Name == "ScriptModule")
                        _scriptModule = child.InnerText;
                    continue;
                }

                int index = SerializeUtils.GetAttributeIntOrDefault(child, "Index", -1);
                string name = SerializeUtils.GetAttributeStringOrDefault(child, "Name", string.Empty);
                string value = child.InnerText;

                if (!string.IsNullOrEmpty(name))
                {
                    if (!string.IsNullOrEmpty(value))
                        _scriptFunctionNames[name] = value;
                }
                else if (index >= 0)
                {
                    if (!string.IsNullOrEmpty(value))
                        _indexedFunctionNames[index] = value;
                }
            }

            if (_indexedFunctionNames.Count > 0 && _schema != null && _schema.Events.Length > 0)
            {
                ResolveIndexedFunctions();
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Interactions");
            writer.WriteAttributeString("Version", XML_VERSION);
            foreach (var evt in _schema.Events)
            {
                string value;
                if (!_scriptFunctionNames.TryGetValue(evt.EventName, out value))
                    continue;

                writer.WriteStartElement("Event");
                writer.WriteAttributeString("Index", evt.Index.ToString());
                writer.WriteAttributeString("Name", evt.EventName.ToString());
                writer.WriteString(value);
                writer.WriteEndElement();
            }
            writer.WriteEndElement();
        }

        /// <summary>
        /// Remaps from the legacy hardcoded list of script functions to the
        /// standard interaction/cursors IDs.
        /// This remap is necessary, because the script function list in game objects
        /// did not precisely correspond to the cursor list. And different objects
        /// had functions on different indexes.
        /// interIndexForCursorID contains legacy interaction index, corresponding to each
        /// standard cursor ID (it tells which function to assign to this cursor ID).
        /// </summary>
        public void RemapLegacyFunctionIndexes(int[] interIndexForCursorID)
        {
            if (_indexedFunctionNames.Count == 0)
                return;

            var newIndexedMap = new Dictionary<int, string>();
            for (int cursorID = 0; cursorID < interIndexForCursorID.Length; ++cursorID)
            {
                string fnName = _indexedFunctionNames.TryGetValueOrDefault(interIndexForCursorID[cursorID], null);
                if (fnName != null)
                    newIndexedMap.Add(cursorID, fnName);
            }
            _indexedFunctionNames = newIndexedMap;
        }

        private void Schema_Changed(object sender, InteractionSchemaChangedEventArgs args)
        {
            SyncWithSchema(args.EventNameRemap);
        }

        private void SyncWithSchema(Dictionary<string, string> eventNameRemap)
        {
            var oldFunctions = _scriptFunctionNames;
            _scriptFunctionNames = new Dictionary<string, string>();
            if (eventNameRemap != null)
            {
                foreach (var remap in eventNameRemap)
                {
                    if (oldFunctions.ContainsKey(remap.Key))
                    {
                        _scriptFunctionNames.Add(remap.Value, oldFunctions[remap.Key]);
                    }
                }
            }

            if (_indexedFunctionNames.Count > 0 && _schema != null && _schema.Events.Length > 0)
            {
                ResolveIndexedFunctions();
            }
        }

        private void ResolveIndexedFunctions()
        {
            if (_schema == null || _schema.Events.Length == 0)
                return; // schema is not ready yet

            foreach (var ifn in _indexedFunctionNames)
            {
                var interEvent = _schema.Events.FirstOrDefault(evt => evt.Index == ifn.Key);
                if (interEvent != null)
                    _scriptFunctionNames.Add(interEvent.EventName, ifn.Value);
            }

            // Drop the indexed functions, we no longer need them, and they are likely not valid anymore
            _indexedFunctionNames.Clear();
        }
    }
}
