using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Scripts : IEnumerable<Script>, IEnumerable
    {
        private ScriptsAndHeaders _scripts;

        public Scripts(ScriptsAndHeaders listScriptAndHeaders)
        {
            _scripts = listScriptAndHeaders;
        }

        public Scripts()
            :this(new ScriptsAndHeaders())
        {

        }

        public Scripts(ScriptFolders scriptFolders)
            :this(new ScriptsAndHeaders(scriptFolders))
        {
        }

        public void Clear()
        {
            _scripts.Clear();
        }

        /// <summary>
        /// Helper private function to add a script. The index is the internal data structure index
        /// to which we would like to add the script.
        /// The function automatically finds the ScriptAndHeader that matches the current script file name
        /// and adds the script to the pair, if it doesn't exist.
        /// If the pair is not found, a new item is added
        /// </summary>
        /// <param name="newScript">The script to add</param>
        /// <param name="index">The desired index where -1 indicates no specific location.</param>
        private void AddAt(Script newScript, int index)
        {
            int indexExisting;
            ScriptAndHeader scriptAndHeaderExisting = _scripts.GetScriptAndHeaderByFilename(newScript.FileName, out indexExisting);
            if (scriptAndHeaderExisting == null)
            {
                // Script does not exist, add a new item
                if (newScript.IsHeader)
                {
                    _scripts.AddAt(new ScriptAndHeader(newScript, null), index);
                }
                else
                {
                    _scripts.AddAt(new ScriptAndHeader(null, newScript), index);
                }
            }
            else
            {
                // Update to the existing index if the requested index is -1
                // (default value), otherwise, add it to the requested index    
                if (index != -1) indexExisting = index;
                if (scriptAndHeaderExisting.Header != null
                     && scriptAndHeaderExisting.Script == null
                     && !newScript.IsHeader)
                {
                    // Header found, script does not exist and trying to add a script, so add it as its pair.
                    // Note that ScriptAndHeader is immutable so we remove the existing one and add a new one
                    _scripts.AddAt(new ScriptAndHeader(scriptAndHeaderExisting.Header, newScript), indexExisting);                    _scripts.Remove(scriptAndHeaderExisting);
                }
                else if (scriptAndHeaderExisting.Header == null
                         && scriptAndHeaderExisting.Script != null
                         && newScript.IsHeader)
                {
                    // Script found, header does not exist and trying to add a script, so add it as its pair
                    // Note that ScriptAndHeader is immutable so we remove the existing one and add a new one
                    _scripts.AddAt(new ScriptAndHeader(newScript, scriptAndHeaderExisting.Script), indexExisting);
                    _scripts.Remove(scriptAndHeaderExisting);

                }
            }

            // Will not add duplicate file name scripts, this is a change from previous behavior
            // for Add and AddAtTop, but it is not expected to be a real use case
        }

        [Obsolete("Adding individual scripts is deprecated, and may not be supported by future versions of AGS. Use ScriptsAndHeaders instead.")]
        public void Add(Script newScript)
        {
            AddAt(newScript, -1);
        }

        [Obsolete("Adding individual scripts is deprecated, and may not be supported by future versions of AGS. Use ScriptsAndHeaders instead.")]
        public void AddAtTop(Script newScript)
        {
            AddAt(newScript, 0);
        }

        [Obsolete("Removing individual scripts is deprecated, and may not be supported by future versions of AGS. Use ScriptsAndHeaders instead.")]
        public void Remove(Script script)
        {
            int index;
            ScriptAndHeader scriptAndHeader = _scripts.FindScriptAndHeader(script, out index);
            if (scriptAndHeader != null)
            {
                _scripts.Remove(scriptAndHeader);

                // Check if there is an existing item, if so add a ScriptAndHeader to keep it.
                if (script.IsHeader && scriptAndHeader.Script != null)
                {
                    _scripts.AddAt(new ScriptAndHeader(null, scriptAndHeader.Script), index);
                }
                else if (!script.IsHeader && scriptAndHeader.Header != null)
                {
                    _scripts.AddAt(new ScriptAndHeader(scriptAndHeader.Header, null), index);
                }

            }
        }

        public Script FindMatchingScriptOrHeader(Script scriptOrHeader)
        {
            ScriptAndHeader scriptAndHeader = _scripts.FindScriptAndHeader(scriptOrHeader);
            if (scriptAndHeader != null)
            {
                if (scriptOrHeader.Equals(scriptAndHeader.Header)) return scriptAndHeader.Script;
                else return scriptAndHeader.Header;
            }
            else
            {
                return null;
            }
        }

        public void MoveScriptAndHeaderUp(Script script)
        {
            _scripts.MoveScriptAndHeaderUp(_scripts.FindScriptAndHeader(script));
        }

        public void MoveScriptAndHeaderDown(Script script)
        {
            _scripts.MoveScriptAndHeaderDown(_scripts.FindScriptAndHeader(script));
        }

        /// <summary>
        /// Returns the number of script items in the list.
        /// </summary>
        /// <remarks>
        /// This property scans the entire list (O(N) complexity)
        /// </remarks>
        public int Count
        {
            get
            {
                int count = 0;
                foreach (Script script in this)
                {
                    count++;
                }
                return count;
            }
        }

        /// <summary>
        /// Returns a script based on its index number.
        /// </summary>
        /// <remarks>
        /// This method scans the entire list (O(N) complexity) and thus it is not recommended
        /// to be used when traversing the list. Use the enumerator instead to perform for loops.
        /// </remarks>
        public Script this[int index]
        {
            get
            {
                int count = 0;
                foreach (Script script in this)
                {
                    if (count == index)
                    {
                        return script;
                    }
                    count++;
                }

                return null;
            }
        }

        public Script GetScriptByFilename(string filename)
        {
            return _scripts.GetScriptByFilename(filename);
        }

        /// <summary>
        /// Returns an enumerator to traverse the scripts
        /// </summary>
        /// <remarks>
        /// The class must use this version of GetEnumerator and not
        /// <see cref="System.Collections.Generic.IEnumerable{T}"/> of <see cref="AGS.Types.Script"/>.
        /// Otherwise existing plugins will break.
        /// </remarks>
        /// <returns>A reference for IEnumerator</returns>
        public IEnumerator GetEnumerator()
        {
            return (this as IEnumerable<Script>).GetEnumerator();
        }

        IEnumerator<Script> IEnumerable<Script>.GetEnumerator()
        {
            foreach (ScriptAndHeader script in _scripts)
            {
                if (script.Header != null) yield return script.Header;
                if (script.Script != null) yield return script.Script;
            }
        }

        public Scripts(XmlNode node)
            : this(new ScriptsAndHeaders(node))
        {
        }

        public void ToXml(XmlTextWriter writer)
        {
            _scripts.ToXml(writer);
        }
    }
}
