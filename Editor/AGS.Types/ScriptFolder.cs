using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.IO;

namespace AGS.Types
{
    public class ScriptFolders : FolderListHybrid<ScriptAndHeader, ScriptFolder>
    {
        public ScriptFolders() : base(new ScriptFolder()) { }

        public ScriptFolders(string name) : base(new ScriptFolder(name)) { }

        public ScriptFolders(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(new ScriptFolder(node, parentNodeForBackwardsCompatability)) { }
    }

    public class ScriptFolder : BaseFolderCollection<ScriptAndHeader, ScriptFolder>
    {
        public const string MAIN_SCRIPT_FOLDER_NAME = "Main";

        public ScriptFolder(string name) : base(name) { }

        public ScriptFolder() : this("Default") { }

        public ScriptFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(node, parentNodeForBackwardsCompatability) { }

        private ScriptFolder(XmlNode node) : base(node) { }

        public IEnumerable<Script> AllScriptsFlat
        {
            get
            {
                foreach (ScriptAndHeader scripts in AllItemsFlat)
                {
                    yield return scripts.Header;
                    yield return scripts.Script;
                }
            }
        }

        public override ScriptFolder CreateChildFolder(string name)
        {
            return new ScriptFolder(name);
        }

        public ScriptAndHeader GetScriptAndHeaderByName(string filename, bool recursive)
        {
            return FindItem(IsItem, filename, recursive);
        }

        public Script GetScriptByFileName(string filename, bool recursive)
        {
            ScriptAndHeader scripts = GetScriptAndHeaderByName(filename, recursive);
            if (scripts == null) return null;
            if (scripts.Header.FileName == filename) return scripts.Header;
            if (scripts.Script.FileName == filename) return scripts.Script;
            return null;
        }

        /*public bool MoveScriptAndHeaderUp(Script script)
        {
            int currentIndex = _items.IndexOf(script);
            if (currentIndex == -1)
            {
                foreach (ScriptFolder folder in _subFolders)
                {
                    if (folder.MoveScriptAndHeaderUp(script)) return true;
                }
                return false;
            }
            if (!script.IsHeader)
            {
                currentIndex--;
            }
            if (currentIndex > 1)
            {
                Script header = _items[currentIndex];
                Script mainScript = _items[currentIndex + 1];
                _items.RemoveAt(currentIndex);
                _items.RemoveAt(currentIndex);
                _items.Insert(currentIndex - 2, mainScript);
                _items.Insert(currentIndex - 2, header);
            }
            return true;
        }

        public bool MoveScriptAndHeaderDown(Script script)
        {
            int currentIndex = _items.IndexOf(script);
            if (currentIndex == -1)
            {
                foreach (ScriptFolder folder in _subFolders)
                {
                    if (folder.MoveScriptAndHeaderDown(script)) return true;
                }
                return false;
            }
            if (!script.IsHeader)
            {
                currentIndex--;
            }
            if (currentIndex < _items.Count - 4)
            {
                Script header = _items[currentIndex];
                Script mainScript = _items[currentIndex + 1];
                _items.RemoveAt(currentIndex);
                _items.RemoveAt(currentIndex);
                _items.Insert(currentIndex + 2, mainScript);
                _items.Insert(currentIndex + 2, header);
            }
            return true;
        }
        */
        public Script FindMatchingScriptOrHeader(Script scriptOrHeader)
        {
            ScriptAndHeader scripts = GetScriptAndHeaderByName(scriptOrHeader.FileName, true);
            if (scripts == null) return null;
            if (scriptOrHeader.IsHeader) return scripts.Script;
            return scripts.Header;
        }

        protected override void FromXmlBackwardsCompatability(System.Xml.XmlNode parentNodeForBackwardsCompatability)
        {
            Init(MAIN_SCRIPT_FOLDER_NAME);
            Script header = null;
            Script script = null;
            foreach (XmlNode node in SerializeUtils.GetChildNodes(parentNodeForBackwardsCompatability, "Scripts"))
            {
                if (header == null)
                {
                    header = new Script(node);
                }
                else
                {
                    script = new Script(node);
                    _items.Add(new ScriptAndHeader(header, script));
                    header = null;
                    script = null;
                }                
            }
        }

        protected override ScriptFolder CreateFolder(XmlNode node)
        {
            return new ScriptFolder(node);
        }

        protected override ScriptAndHeader CreateItem(XmlNode node)
        {
            return new ScriptAndHeader(node);
        }

        private bool IsItem(ScriptAndHeader script, string scriptFilename)
        {
            return script.Name == Path.GetFileNameWithoutExtension(scriptFilename);
        }
    }
}
