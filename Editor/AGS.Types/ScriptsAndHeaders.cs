using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class ScriptsAndHeaders : IEnumerable<ScriptAndHeader>
    {
        private IList<ScriptAndHeader> _scripts;

        public ScriptsAndHeaders()
        {
            _scripts = new ScriptFolders();
        }

        public ScriptsAndHeaders(ScriptFolders scriptFolders)
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

        public void AddAt(ScriptAndHeader newScript, int index)
        {
            if (index == -1)
            {
                Add(newScript);
            }
            else
            {
                _scripts.Insert(index, newScript);
            }
        }

        public void Remove(ScriptAndHeader script)
        {
            _scripts.Remove(script);
        }

        public int Count
        {
            get { return _scripts.Count;  }
        }

        public ScriptAndHeader this[int index]
        {
            get
            {
                return _scripts[index];
            }
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

        public void MoveScriptAndHeaderUp(ScriptAndHeader script)
        {
            if (script == null) return;
            int index = _scripts.IndexOf(script);
            if (index == -1) return;
            if (index > 0)
            {
                _scripts.RemoveAt(index);
                _scripts.Insert(index - 1, script);
            }
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

        public Script GetScriptByFilename(string filename)
        {
            int index;
            ScriptAndHeader scriptAndHeader = GetScriptAndHeaderByFilename(filename, out index);
            if (scriptAndHeader == null) return null;
            if (filename.EndsWith(".ash")) return scriptAndHeader.Header;
            return scriptAndHeader.Script;
        }

        public ScriptAndHeader GetScriptAndHeaderByFilename(string filename, out int index)
        {
            index = 0;
            if (filename.EndsWith(".ash") || filename.EndsWith(".asc"))
            {
                filename = filename.Substring(0, filename.Length - 4);
            }
            foreach (ScriptAndHeader scriptAndHeader in _scripts)
            {
                if (((scriptAndHeader.Header != null) && (scriptAndHeader.Header.FileName == (filename + ".ash"))) ||
                    ((scriptAndHeader.Script != null) && (scriptAndHeader.Script.FileName == (filename + ".asc"))))
                {
                    return scriptAndHeader;
                }
                index++;
            }
            return null;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public IEnumerator<ScriptAndHeader> GetEnumerator()
        {
            foreach (ScriptAndHeader scriptAndHeader in _scripts)
            {
                yield return scriptAndHeader;
            }
        }

        public ScriptsAndHeaders(XmlNode node)
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
                if (script.Script != null) script.Script.ToXml(writer);
            }

            writer.WriteEndElement();
        }
    }
}
