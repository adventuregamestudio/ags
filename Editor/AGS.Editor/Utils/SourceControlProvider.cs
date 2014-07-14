using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Microsoft.Win32;
using AGS.Native;
using AGS.Types;

namespace AGS.Editor
{
    public class SourceControlProvider : ISourceControlProvider
    {
        private string _providerName = null;
        private string _providerDLL = null;
        private string _projectAuxPath = null;
        private string _projectName = null;
        private SourceCodeControl _scc;

        public SourceControlProvider()
        {
            GetSourceControlProviderDLL();
            _scc = new SourceCodeControl();
        }

        public bool Initialize(IntPtr mainWindowHandle)
        {
            if (_providerDLL != null)
            {
                try
                {
                    if (!_scc.Initialize(_providerDLL, mainWindowHandle.ToInt32()))
                    {
                        _providerDLL = null;
                        _providerName = null;
                        return false;
                    }
                }
                catch (Exception ex)
                {
                    _providerDLL = null;
                    throw new AGSEditorException(ex.Message, ex);
                }
            }
            return true;
        }

        public void Dispose()
        {
            if (_providerDLL != null)
            {
                _scc.Shutdown();
                _providerDLL = null;
            }
        }

        public string ProviderName
        {
            get { return (_providerName == null) ? "(unknown provider)" : _providerName; }
        }

        public bool Available
        {
            get { return (_providerDLL != null); }
        }

        public bool ProjectUnderControl
        {
            get { return (_projectAuxPath != null); }
        }

        public string ProjectSourceControlPath
        {
            get { return _projectAuxPath; }
        }

        public string ProjectSourceControlName
        {
            get { return _projectName; }
        }

        public bool AddToSourceControl()
        {
            SourceControlProject project = _scc.AddToSourceControl();
            if (project != null)
            {
                _projectAuxPath = project.AuxPath;
                _projectName = project.ProjectName;
                return _scc.OpenProject(project);
            }
            return false;
        }

        public bool OpenProject(string sourceControlPath, string projectName)
        {
            _projectAuxPath = sourceControlPath;
            _projectName = projectName;

            SourceControlProject project = new SourceControlProject(sourceControlPath, projectName);
            if (!_scc.OpenProject(project))
            {
                _projectAuxPath = null;
                _projectName = null;
                return false;
            }

            _projectAuxPath = project.AuxPath;
            return true;
        }

        public void CloseProject()
        {
            if (_projectAuxPath != null)
            {
                _scc.CloseProject();
                _projectAuxPath = null;
                _projectName = null;
            }
        }

        public bool IsFileUnderSourceControl(string fileName)
        {
            SourceControlFileStatus status = GetFileStatuses(new string[] { fileName })[0];
            if ((status & SourceControlFileStatus.Controlled) != 0)
            {
                return true;
            }
            return false;
        }

        public SourceControlFileStatus[] GetFileStatuses(string[] fileNames)
        {
            if (_projectAuxPath == null)
            {
                SourceControlFileStatus[] results = new SourceControlFileStatus[fileNames.Length];
                for (int i = 0; i < results.Length; i++)
                {
                    results[i] = SourceControlFileStatus.NotControlled;
                }
                return results;
            }
            return _scc.GetFileStatuses(fileNames);
        }

        public void AddFilesToSourceControl(string[] fileNames, string comment)
        {
            if (_projectAuxPath == null)
            {
                throw new AGSEditorException("No source control project is open");
            }
            _scc.AddFilesToSourceControl(fileNames, comment);
        }

        public void CheckInFiles(string[] fileNames, string comment)
        {
            if (_projectAuxPath == null)
            {
                throw new AGSEditorException("No source control project is open");
            }
            _scc.CheckInFiles(fileNames, comment);
        }

        public void CheckOutFiles(string[] fileNames, string comment)
        {
            if (_projectAuxPath == null)
            {
                throw new AGSEditorException("No source control project is open");
            }
            _scc.CheckOutFiles(fileNames, comment);
        }

        public void RenameFileOnDiskAndInSourceControl(string currentName, string newName)
        {
            string sourcePath = Path.GetFullPath(currentName);
            string destPath = Path.GetFullPath(newName);

            if (this.IsFileUnderSourceControl(sourcePath))
            {
                this.RenameFile(sourcePath, destPath);
            }

            File.Move(sourcePath, destPath);
        }

        public void RemoveFiles(string[] fileNames)
        {
            if (_projectAuxPath == null)
            {
                throw new AGSEditorException("No source control project is open");
            }
            _scc.DeleteFiles(fileNames, string.Empty);
        }

        /// <summary>
        /// Renames a file in source control. The supplied filenames must
        /// be fully qualified paths.
        /// </summary>
        public void RenameFile(string currentPath, string newPath)
        {
            if (_projectAuxPath == null)
            {
                throw new AGSEditorException("No source control project is open");
            }
            _scc.RenameFile(currentPath, newPath);
        }

        private void GetSourceControlProviderDLL()
        {
            RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\SourceCodeControlProvider");
            if (key != null)
            {
                string providerPath = (string)key.GetValue("ProviderRegKey");
                key.Close();
                if (providerPath != null)
                {
                    key = Registry.LocalMachine.OpenSubKey(providerPath);
                    if (key != null)
                    {
                        _providerName = (string)key.GetValue("SCCServerName");
                        _providerDLL = (string)key.GetValue("SCCServerPath");
                        key.Close();
                    }
                }
            }
        }
    }
}
