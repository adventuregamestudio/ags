using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Scripts : IEnumerable
    {
        private List<Script> _scripts;

        public Scripts()
        {
            _scripts = new List<Script>();
        }

        public void Clear()
        {
            _scripts.Clear();
        }

        public void Add(Script newScript)
        {
            _scripts.Add(newScript);
        }

        public void AddAtTop(Script newScript)
        {
            _scripts.Insert(0, newScript);
        }

        public void Remove(Script script)
        {
            _scripts.Remove(script);
        }

		public Script FindMatchingScriptOrHeader(Script scriptOrHeader)
		{
			int currentIndex = _scripts.IndexOf(scriptOrHeader);
			if (scriptOrHeader.IsHeader)
			{
				return _scripts[currentIndex + 1];
			}
			return _scripts[currentIndex - 1];
		}

        public void MoveScriptAndHeaderUp(Script script)
        {
            int currentIndex = _scripts.IndexOf(script);
            if (!script.IsHeader)
            {
                currentIndex--;
            }
            if (currentIndex > 1)
            {
                Script header = _scripts[currentIndex];
                Script mainScript = _scripts[currentIndex + 1];
                _scripts.RemoveAt(currentIndex);
                _scripts.RemoveAt(currentIndex);
                _scripts.Insert(currentIndex - 2, mainScript);
                _scripts.Insert(currentIndex - 2, header);
            }
        }

        public void MoveScriptAndHeaderDown(Script script)
        {
            int currentIndex = _scripts.IndexOf(script);
            if (!script.IsHeader)
            {
                currentIndex--;
            }
            if (currentIndex < _scripts.Count - 4)
            {
                Script header = _scripts[currentIndex];
                Script mainScript = _scripts[currentIndex + 1];
                _scripts.RemoveAt(currentIndex);
                _scripts.RemoveAt(currentIndex);
                _scripts.Insert(currentIndex + 2, mainScript);
                _scripts.Insert(currentIndex + 2, header);
            }
        }


        public int Count
        {
            get { return _scripts.Count; }
        }

        public Script this[int index]
        {
            get { return _scripts[index]; }
        }

        public Script GetScriptByFilename(string filename)
        {
            foreach (Script script in _scripts)
            {
                if (script.FileName == filename)
                {
                    return script;
                }
            }
            return null;
        }

        public IEnumerator GetEnumerator()
        {
            return _scripts.GetEnumerator();
        }

        public Scripts(XmlNode node)
        {
            _scripts = new List<Script>();
            foreach (XmlNode child in node.SelectSingleNode("Scripts").ChildNodes)
            {
                _scripts.Add(new Script(child));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Scripts");

            foreach (Script script in _scripts)
            {
                script.ToXml(writer);
            }

            writer.WriteEndElement();
        }
    }
}
