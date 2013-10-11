using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Scripts : IEnumerable, IEnumerable<ScriptAndHeader>
    {
        private IList<ScriptAndHeader> _scripts;
        private Script headerWithoutScript;

        public Scripts()
        {
            _scripts = new ScriptFolders();
        }

        public Scripts(ScriptFolders scriptFolders)
        {
            _scripts = scriptFolders;
        }

        public void Clear()
        {
            _scripts.Clear();
        }

        [Obsolete("Adding individual scripts is deprecated, and may not be supported by future versions of AGS. Add a ScriptAndHeader at once instead.")]
        public void Add(Script newScript)
        {
            if (newScript == null) return;
            ScriptAndHeader scriptAndHeader = null;
            if (newScript.IsHeader)
            {
                if (headerWithoutScript != null)
                {
                    scriptAndHeader = new ScriptAndHeader(headerWithoutScript, null);
                }
                headerWithoutScript = newScript;
            }
            else
            {
                scriptAndHeader = new ScriptAndHeader(headerWithoutScript, newScript);
                headerWithoutScript = null;
            }
            if (scriptAndHeader != null) _scripts.Add(scriptAndHeader);
        }

        public void Add(ScriptAndHeader newScript)
        {
            _scripts.Add(newScript);
        }

        [Obsolete("Adding individual scripts is deprecated, and may not be supported by future versions of AGS. Add a ScriptAndHeader at once instead.")]
        public void AddAtTop(Script newScript)
        {
            if (newScript == null) return;
            ScriptAndHeader scriptAndHeader = null;
            if (newScript.IsHeader)
            {
                if (headerWithoutScript != null)
                {
                    scriptAndHeader = new ScriptAndHeader(headerWithoutScript, null);
                }
                headerWithoutScript = newScript;
            }
            else
            {
                scriptAndHeader = new ScriptAndHeader(headerWithoutScript, newScript);
                headerWithoutScript = null;
            }
            if (scriptAndHeader != null) _scripts.Insert(0, scriptAndHeader);
        }

        public void AddAtTop(ScriptAndHeader newScript)
        {
            _scripts.Insert(0, newScript);
        }

        public void Remove(ScriptAndHeader script)
        {
            _scripts.Remove(script);
        }

        public void Remove(Script script)
        {
            if (script == null) return;
            for (int i = _scripts.Count; i >= 0; --i)
            {
                ScriptAndHeader scriptAndHeader = _scripts[i];
                if (script.Equals(scriptAndHeader.Header) ||
                    script.Equals(scriptAndHeader.Script))
                {
                    _scripts.RemoveAt(i);
                    break;
                }
            }
        }

        public Script FindMatchingScriptOrHeader(Script scriptOrHeader)
        {
            if (scriptOrHeader == null) return null;
            foreach (ScriptAndHeader script in _scripts)
            {
                if (scriptOrHeader.Equals(script.Header)) return script.Script;
                if (scriptOrHeader.Equals(script.Script)) return script.Header;
            }
            return null;
        }

        public ScriptAndHeader FindScriptAndHeader(Script scriptOrHeader)
        {
            int dummy;
            return FindScriptAndHeader(scriptOrHeader, out dummy);
        }

        public ScriptAndHeader FindScriptAndHeader(Script scriptOrHeader, out int index)
        {
            index = -1;
            if (scriptOrHeader == null) return null;
            for (int i = 0; i < _scripts.Count; ++i)
            {
                if (scriptOrHeader.Equals(_scripts[i].Header) || scriptOrHeader.Equals(_scripts[i].Script))
                {
                    index = i;
                    return _scripts[i];
                }
            }
            return null;
        }

        public void MoveScriptAndHeaderUp(Script script)
        {
            MoveScriptAndHeaderUp(FindScriptAndHeader(script));
        }

        public void MoveScriptAndHeaderUp(ScriptAndHeader script)
        {
            if (script == null) return;
            int index = _scripts.IndexOf(script);
            if (index == -1) return;
            _scripts.RemoveAt(index);
            _scripts.Insert(index - 1, script);
        }

        public void MoveScriptAndHeaderDown(Script script)
        {
            MoveScriptAndHeaderDown(FindScriptAndHeader(script));
        }

        public void MoveScriptAndHeaderDown(ScriptAndHeader script)
        {
            if (script == null) return;
            int index = _scripts.IndexOf(script);
            if (index == -1) return;
            if (index < (_scripts.Count - 1))
            {
                _scripts.RemoveAt(index);
                _scripts.Insert(index + 1, script);
            }
        }

        public int Count
        {
            get { return _scripts.Count; }
        }

        public ScriptAndHeader this[int index]
        {
            get
            {
                return _scripts[index];
            }
        }

        public Script GetScriptByFilename(string filename)
        {
            ScriptAndHeader scriptAndHeader = GetScriptAndHeaderByFile(filename);
            if (scriptAndHeader == null) return null;
            if (filename.EndsWith(".ash")) return scriptAndHeader.Header;
            return scriptAndHeader.Script;
        }

        public ScriptAndHeader GetScriptAndHeaderByFile(string filename)
        {
            if (filename.EndsWith(".ash") || filename.EndsWith(".asc"))
            {
                filename = filename.Substring(0, filename.Length - 4);
            }
            foreach (ScriptAndHeader script in _scripts)
            {
                if (((script.Header != null) && (script.Header.FileName == (filename + ".ash"))) ||
                    ((script != null) && (script.Script.FileName == (filename + ".asc"))))
                {
                    return script;
                }
            }
            return null;
        }

        [Obsolete("The behavior of this enumerator is deprecated and may not be supported by future versions of AGS. Use the ScriptAndHeader enumerator instead.")]
        IEnumerator IEnumerable.GetEnumerator()
        {
            foreach (ScriptAndHeader script in _scripts)
            {
                if (script.Header != null) yield return script.Header;
                if (script.Script != null) yield return script.Script;
            }
        }

        public IEnumerator<ScriptAndHeader> GetEnumerator()
        {
            foreach (ScriptAndHeader script in _scripts)
            {
                yield return script;
            }
        }

        public void RefreshProjectTree(IAGSEditor editor, string selectedNodeID)
        {
            foreach (IEditorComponent component in editor.Components)
            {
                if ((component.ComponentID == "Scripts") && (component is IRePopulatableComponent))
                {
                    if (selectedNodeID == null)
                    {
                        (component as IRePopulatableComponent).RePopulateTreeView();
                    }
                    else
                    {
                        (component as IRePopulatableComponent).RePopulateTreeView(selectedNodeID);
                    }
                    return;
                }
            }
        }

        public void RefreshProjectTree(IAGSEditor editor)
        {
            RefreshProjectTree(editor, null);
        }

        public Scripts(XmlNode node)
        {
            _scripts = new ScriptFolders();
            XmlNodeList children = node.SelectSingleNode("Scripts").ChildNodes;
            Script header = null;
            for (int i = 0; i < children.Count; ++i)
            {
                Script script = new Script(children[i]);
                if (!script.IsHeader)
                {
                    // found a script, with or without header
                    Add(new ScriptAndHeader(header, script));
                    header = null;
                }
                else // found a header
                {
                    if (header != null) Add(new ScriptAndHeader(header, null)); // found header with no script
                    header = script; // found header, checking for script
                }
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Scripts");

            foreach (ScriptAndHeader script in _scripts)
            {
                if (script.Header != null) script.Header.ToXml(writer);
                script.Script.ToXml(writer);
            }

            writer.WriteEndElement();
        }
    }
}
