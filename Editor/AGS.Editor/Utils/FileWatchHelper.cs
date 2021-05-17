using AGS.Editor.Preferences;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// Helper class to watch a set of related files, like the room files.
    /// </summary>
    public class FileWatchHelpers : KeyedCollection<string, FileWatchHelper>, IDisposable
    {
        public void Dispose()
        {
            foreach (FileWatchHelper item in Items)
                item.Dispose();
        }

        public void AddRange(IEnumerable<FileWatchHelper> items)
        {
            foreach (FileWatchHelper item in items)
                Add(item);
        }

        /// <summary>
        /// Temporarily disables the file watchers while performing an action, like saving files
        /// from the AGS editor.
        /// </summary>
        public void TemporarilyDisable(Action doWhenDisabled)
        {
            Dictionary<string, bool> backup = Items.ToDictionary(i => i.FileName, i => i.Enabled);

            foreach (FileWatchHelper item in Items)
                item.Enabled = false;

            doWhenDisabled();

            foreach (FileWatchHelper item in Items)
                item.Enabled = backup[item.FileName];
        }

        protected override string GetKeyForItem(FileWatchHelper item) => item.FileName;

        protected override void ClearItems()
        {
            Dispose();
            base.ClearItems();
        }

        protected override void SetItem(int i, FileWatchHelper item)
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
    public class FileWatchHelper : IDisposable
    {
        private delegate void LoadFileOnGUIThread();

        private readonly Action _loadFile;
        private readonly GUIController _guiController;
        private readonly IAppSettings _settings;
        private readonly EditorEvents _events;

        private bool _changed;

        public FileWatchHelper(string fileName, Action loadFile)
        {
            FileName = fileName;
            _loadFile = loadFile;

            _guiController = Factory.GUIController;
            _guiController.OnMainWindowActivated += OnMainWindowActivated;

            _settings = Factory.AGSEditor.Settings;

            _events = Factory.Events;
            _events.FileChangedInGameFolder += OnFileChanged;
        }

        public string FileName { get; }
        public DateTime ChangedAt { get; set; }
        public bool Enabled { get; set; } = true;

        /// <summary>
        /// Use this flag to suppress the file watcher when saving is handled from AGS
        /// </summary>
        //public bool SavingInternally { get; set; } = false;

        public void Dispose()
        {
            _guiController.OnMainWindowActivated -= OnMainWindowActivated;
            _events.FileChangedInGameFolder -= OnFileChanged;
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
        private void OnFileChanged(string fileName)
        {
            if (FileName.Equals(fileName, StringComparison.OrdinalIgnoreCase))
            {
                DateTime now = DateTime.Now;

                if (Enabled)
                {
                    if (now.Subtract(ChangedAt).TotalSeconds > 2 &&
                        !Utilities.IsMonoRunning() &&
                        Utilities.IsThisApplicationCurrentlyActive())
                    {
                        //On Mono can't use the Win API to check if application is in focus.
                        //Hopefully the prompt will be triggered by its second usage,
                        //when the main window is activated.
                        _guiController.Invoke((LoadFileOnGUIThread)LoadFile); // GuiController.Invoke runs on UI thread
                    }
                    else
                    {
                        _changed = true;
                    }
                }

                ChangedAt = now;
            }
        }

        private void LoadFile()
        {
            if (ShouldLoad())
            {
                _loadFile();
            }
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
