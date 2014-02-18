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

            //	Since we eat the destroy message in WndProc
            //	we have to manually let Scintilla know to
            //	clean up its resources.
            Message destroyMessage = new Message();
            destroyMessage.Msg = WinAPI.WM_DESTROY;
            //Usually a big no-no, but here while we can do a cross-thread operation from the finalizer thread,
            //it's guaranteed all other threads are suspended (because we're in the finalizer thread), so
            //for this very special scenario, cross-thread is ok.
            Control.CheckForIllegalCrossThreadCalls = false;
            try
            {
                destroyMessage.HWnd = Handle;
                base.DefWndProc(ref destroyMessage);
            }
            catch (Win32Exception) { } //Can fail to create handle during application close
            finally
            {
                Control.CheckForIllegalCrossThreadCalls = true;
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
