using System.Windows.Forms;
using System.ComponentModel;
namespace Scintilla
{
    partial class ScintillaControl
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }

            // Fix taken from ScintillaNet code: https://scintillanet.codeplex.com/SourceControl/changeset/36876#Branches/2.0/ScintillaNET/Scintilla.cs
            if (disposing && IsHandleCreated)
            {
                //	Since we eat the destroy message in WndProc
                //	we have to manually let Scintilla know to
                //	clean up its resources.
                Message destroyMessage = new Message();
                destroyMessage.Msg = WinAPI.WM_DESTROY;
                destroyMessage.HWnd = Handle;
                base.DefWndProc(ref destroyMessage);                
            }

            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            components = new System.ComponentModel.Container();
        }

        #endregion
    }
}
