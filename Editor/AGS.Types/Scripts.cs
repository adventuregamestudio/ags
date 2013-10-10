using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Scripts : IEnumerable
    {        
        private IList<ScriptAndHeader> _scripts;
        //private Script headerWithoutScript;

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

        public void Add(ScriptAndHeader newScript)
        {
            _scripts.Add(newScript);
        }

        /*public void Add(Script newScript)
        {
            if (headerWithoutScript != null || !newScript.IsHeader)
            {
                ScriptAndHeader scriptAndHeader = new ScriptAndHeader(headerWithoutScript, newScript);
                _scripts.Add(scriptAndHeader);
                headerWithoutScript = null;
            }
            else
            {
                headerWithoutScript = newScript;
            }            
        }*/

        public void AddAtTop(ScriptAndHeader newScript)
        {
            _scripts.Insert(0, newScript);
        }

        /*public void AddAtTop(Script newScript)
        {
            if (headerWithoutScript != null || !newScript.IsHeader)
            {
                ScriptAndHeader scriptAndHeader = new ScriptAndHeader(headerWithoutScript, newScript);
                _scripts.Insert(0, scriptAndHeader);
                headerWithoutScript = null;
            }
            else
            {
                headerWithoutScript = newScript;
            }            
        }*/

        public void Remove(ScriptAndHeader script)
        {
            _scripts.Remove(script);
        }

        /*public void Remove(Script script)
        {
            for (int i = _scripts.Count; i >= 0; i--)
            {
                ScriptAndHeader scriptAndHeader = _scripts[i];
                if (script.Equals(scriptAndHeader.Header) ||
                    script.Equals(scriptAndHeader.Script))
                {
                    _scripts.RemoveAt(i);
                    break;
                }
            }            
        }*/

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

        /*public Script FindMatchingScriptOrHeader(Script scriptOrHeader)
		{
            foreach (ScriptAndHeader scriptAndHeader in _scripts)
            {
                if (scriptOrHeader.Equals(scriptAndHeader.Header)) return scriptAndHeader.Script;
                if (scriptOrHeader.Equals(scriptAndHeader.Script)) return scriptAndHeader.Header;
            }
            return null;
		}*/

        public void MoveScriptAndHeaderUp(ScriptAndHeader script)
        {
            if (script == null) return;
            int index = _scripts.IndexOf(script);
            if (index == -1) return;
            _scripts.RemoveAt(index);
            _scripts.Insert(index - 1, script);
        }

        /*public void MoveScriptAndHeaderUp(Script script)
        {
            int index;
            ScriptAndHeader scriptAndHeader = FindScriptAndHeader(script, out index);
            if (scriptAndHeader == null) return;

            _scripts.RemoveAt(index);
            _scripts.Insert(index - 1, scriptAndHeader);                         
        }*/

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

        /*public void MoveScriptAndHeaderDown(Script script)
        {
            int index;
            ScriptAndHeader scriptAndHeader = FindScriptAndHeader(script, out index);
            if (scriptAndHeader == null) return;

            if (index < _scripts.Count - 1)
            {
                _scripts.RemoveAt(index);
                _scripts.Insert(index + 1, scriptAndHeader);                
            }
        }*/

        /*private ScriptAndHeader FindScriptAndHeader(Script script, out int index)
        {
            int currentIndex;
            ScriptAndHeader scriptAndHeader = null;
            for (currentIndex = 0; currentIndex < _scripts.Count; currentIndex++)
            {
                scriptAndHeader = _scripts[currentIndex];
                if (script.Equals(scriptAndHeader.Header) || script.Equals(scriptAndHeader.Script))
                {
                    index = currentIndex;
                    return scriptAndHeader;
                }
            }
            index = -1;
            return null;        
        }*/


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

        /*public Script this[int index]
        {
            get
            {
                int currentIndex = -1;
                foreach (ScriptAndHeader scriptAndHeader in _scripts)
                {
                    if (scriptAndHeader.Header != null)
                    {
                        currentIndex++;
                        if (currentIndex == index) return scriptAndHeader.Header;
                    }
                    currentIndex++;
                    if (currentIndex == index) return scriptAndHeader.Script;
                }
                return null;
            }
        }*/

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

        /*public Script GetScriptByFilename(string filename)
        {
            foreach (ScriptAndHeader scriptAndHeader in _scripts)
            {
                if (scriptAndHeader.Header != null && scriptAndHeader.Header.FileName == filename)
                {
                    return scriptAndHeader.Header;
                }
                if (scriptAndHeader.Script.FileName == filename)
                {
                    return scriptAndHeader.Script;
                }
            }
            return null;
        }*/

        public IEnumerator GetEnumerator()
        {
            foreach (ScriptAndHeader script in _scripts)
            {
                yield return script;
            }
        }

        /*public IEnumerator GetEnumerator()
        {
            foreach (ScriptAndHeader scriptAndHeader in _scripts)
            {
                if (scriptAndHeader.Header != null) yield return scriptAndHeader.Header;
                yield return scriptAndHeader.Script;
            }
        }*/

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

        /*public Scripts(XmlNode node)
        {
            _scripts = new ScriptFolders();
            foreach (XmlNode child in node.SelectSingleNode("Scripts").ChildNodes)
            {
                Add(new Script(child));
            }
        }*/

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
