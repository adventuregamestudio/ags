/* Adventure Game Studio Editor
 * Copyright (c) 2006-2010 Chris Jones
 * Use of this source must be strictly in accordance with the LICENSE.TXT file
 * that accompanies this source distribution.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using System.Reflection;
using System.Runtime.InteropServices;

namespace AGS.Editor
{
    static class Program
    {
        private static string[] _commandLineArgs;
        private static ApplicationController _application;
        private static int _exitCode = 0;

        public static void SetExitCode(int exit_code)
        {
            _exitCode = exit_code;
        }

        [STAThread]
        static int Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            RunApplication(args);
            return _exitCode;
        }

        private static void RunApplication(string[] args)
        {
#if !DEBUG
            try
#endif
            {
                Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
                Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);
                AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);
                AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(CurrentDomain_AssemblyResolve);

                string filePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
                Directory.SetCurrentDirectory(Path.GetDirectoryName(filePath));

                _commandLineArgs = args;
                SplashScreen splash = new SplashScreen();
                splash.Load += new EventHandler(splash_Load);
                Application.Run(splash);

                // If Application Controller failed to init, then bail out early
                if (_application == null)
                {
                    Program.SetExitCode(1);
                    return;
                }

                // Need to re-add these because the end of Application.Run removes them
                Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);
                AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);

                _application.StartGUI();
            }
#if !DEBUG
            catch (Exception ex)
            {
                ApplicationController.ShowMessage("An unexpected error occurred. See below for details." + Environment.NewLine + ex.ToString(), "Application Error", MessageBoxIcon.Stop);
            }
#endif
        }

        private static void splash_Load(object sender, EventArgs e)
        {
            // The splash screen hasn't appeared yet, so we need to start
            // a timer to enable us to do our actual loading tasks once
            // it has done so.
            Timer timer = new Timer();
            timer.Interval = 40;
            timer.Tick += new EventHandler(startupTimer_Tick);
            timer.Start();
        }

        private static void startupTimer_Tick(object sender, EventArgs e)
        {
            ((Timer)sender).Stop();
            ((Timer)sender).Dispose();

            // This line does all the work loading the app
            _application = new ApplicationController(_commandLineArgs);

            // Close the splash screen; control will return to
            // the RunApplication function where it will start
            // the main form.
            Application.OpenForms[0].Close();
        }

        private static void Application_ThreadException(object sender, System.Threading.ThreadExceptionEventArgs e)
        {
			HandleException(e.Exception);
        }

        private static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            HandleException((Exception)e.ExceptionObject);
        }

        private static Assembly CurrentDomain_AssemblyResolve(object sender, ResolveEventArgs args)
        {
            AssemblyName assembly = new AssemblyName(args.Name);
            Assembly resolved = null;

            if (assembly.GetPublicKey() == null)
            {
                switch(assembly.Name)
                {
                    case "AGS.Controls":
                    case "AGS.CScript.Compiler":
                    case "AGS.Native":
                    case "AGS.Types":
                        // UnsafeLoadFrom prevents the assembly being blocked
                        // by ZoneIdentifiers in NTFS alternate stream data
                        resolved = Assembly.UnsafeLoadFrom(assembly.Name + ".dll");
                        break;
                }
            }

            return resolved;
        }

		private static void HandleException(Exception ex)
		{
			if (_application != null)
			{
				_application.UnhandledErrorHandler(ex);
			}
			else
			{
                string error = ex.Message;
                try
                {
                    error = ex.ToString();
                }
                catch (SEHException sehException)
                {
                    error = string.Format("\nStack Trace SEHException: HResult Error Code: {0}, Exception: {1}",
                        sehException.ErrorCode, error);
                }
                ApplicationController.ShowMessage("An unexpected error occurred trying to start up the AGS Editor. Please consult the details below and post the error to the AGS Technical Forum.\n\n" + error, "Error", MessageBoxIcon.Stop);

                // Close the Splash Screen if it was open while the exception occured
                if (Application.OpenForms.Count > 0)
                {
                    Application.OpenForms[0].Close();
                }
            }
		}
    }
}