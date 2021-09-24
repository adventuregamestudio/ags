using AGS.Editor.Preferences;
using AGS.Types.Interfaces;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// Helper class to watch a set of related files, like the room files.
    /// </summary>
    public class FileWatcherCollection : KeyedCollection<string, FileWatcher>, IDisposable
    {
        public void Dispose()
        {
            foreach (FileWatcher item in Items)
                item.Dispose();
        }

        public void AddRange(IEnumerable<FileWatcher> items)
        {
            foreach (FileWatcher item in items)
                Add(item);
        }

        /// <summary>
        /// Temporarily disables the file watchers while performing an action, like saving files
        /// from the AGS editor.
        /// </summary>
        public void TemporarilyDisable(Action doWhenDisabled)
        {
            Dictionary<string, bool> backup = Items.ToDictionary(i => i.FileName, i => i.Enabled);

            foreach (FileWatcher item in Items)
                item.Enabled = false;

            doWhenDisabled();

            foreach (FileWatcher item in Items)
                item.Enabled = backup[item.FileName];
        }

        protected override string GetKeyForItem(FileWatcher item) => item.FileName;

        protected override void ClearItems()
        {
            Dispose();
            base.ClearItems();
        }

        protected override void SetItem(int i, FileWatcher item)
        {
            this[i].Dispose();
            base.SetItem(i, item);
        }

        protected override void RemoveItem(int i)
        {
            this[i].Dispose();
            base.RemoveItem(i);
        }
    }

    /// <summary>
    /// Helper utility to watch specific files.
    /// </summary>
    public class FileWatcher : IDisposable
    {
        private delegate void LoadFileOnGUIThread();

        private readonly FileSystemWatcher _fileWatcher;
        private readonly ISaveable _saveable;
        private readonly Action _loadFile;
        private readonly GUIController _guiController;
        private readonly IAppSettings _settings;

        private bool _changed;

        public FileWatcher(string fileName, ISaveable saveable, Action loadFile)
        {
            AGSEditor agsEditor = Factory.AGSEditor;

            string path = agsEditor.CurrentGame.DirectoryPath;
            // It doesn' seem to work for unknown reason when I use the filename with fullt path
            // as a filter in the FileSystemWatcher constructor, if we could make that work we
            // can guarantee the watcher only reacts to the correct file.
            string filter = Path.GetFileName(fileName);
            _fileWatcher = new FileSystemWatcher(path, filter);
            _fileWatcher.Changed += OnFileChanged;
            _fileWatcher.NotifyFilter = NotifyFilters.LastWrite;
            _fileWatcher.EnableRaisingEvents = true;
            _fileWatcher.IncludeSubdirectories = true;

            FileName = fileName;
            _saveable = saveable;
            _loadFile = loadFile;

            _guiController = Factory.GUIController;
            _guiController.OnMainWindowActivated += OnMainWindowActivated;

            _settings = Factory.AGSEditor.Settings;
        }

        public string FileName { get; }

        public bool Enabled
        {
            get { return _fileWatcher.EnableRaisingEvents; }
            set { _fileWatcher.EnableRaisingEvents = value; }
        }

        public void Dispose()
        {
            _fileWatcher.Dispose();
            _guiController.OnMainWindowActivated -= OnMainWindowActivated;
        }

        /// <summary>
        /// If editor window is not the active window we wait to update the UI until
        /// this event has fired
        /// </summary>
        /// <remarks>
        /// Runs on the UI thread so we can use this to run our code updating UI
        /// </remarks>
        private void OnMainWindowActivated(object sender, EventArgs e)
        {
            if (_changed)
            {
                LoadFile();
                _changed = false;
            }
        }

        /// <summary>
        /// This is the method used to control file change events triggered by the file watcher.
        /// </summary>
        /// <remarks>
        /// Any actual code updating UI cannot be invoked here, becuse the file watcher and the UI
        /// fires on different thread which is illegal and will cause crashes, or file streams
        /// returning empty file reads for some reason. Make sure any code loading data or
        /// updating UI loads on the GUI thread.
        /// </remarks>
        private void OnFileChanged(object sender, FileSystemEventArgs e)
        {
            if (Enabled && !_saveable.IsBeingSaved && FileName.Equals(e.Name, StringComparison.OrdinalIgnoreCase))
            {
                if (!Utilities.IsMonoRunning() && Utilities.IsThisApplicationCurrentlyActive())
                {
                    //On Mono can't use the Win API to check if application is in focus.
                    //Hopefully the prompt will be triggered by its second usage,
                    //when the main window is activated.
                    _guiController.Invoke((LoadFileOnGUIThread)LoadFile); // GuiController.Invoke runs on UI thread
                }
                else
                {
                    _changed = true;
                    // File changes are made in batches so disable file watcher to avoid spammed Changed events
                    _fileWatcher.EnableRaisingEvents = false;
                }
            }
        }

        private void LoadFile()
        {
            if (ShouldLoad())
            {
                _loadFile();
            }

            _fileWatcher.EnableRaisingEvents = true;
        }

        private bool ShouldLoad()
        {
            switch (_settings.ReloadScriptOnExternalChange)
            {
                case ReloadScriptOnExternalChange.Prompt:
                    string question = $"The file '{FileName}' has been modified externally. Do you want to reload it?";
                    if (_guiController.ShowQuestion(question, MessageBoxIcon.Question) != DialogResult.Yes)
                    {
                        break;
                    }
                    return true;
                case ReloadScriptOnExternalChange.Always:
                    return true;
                case ReloadScriptOnExternalChange.Never:
                default:
                    break;
            }

            return false;
        }
    }
}
