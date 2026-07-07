using System;
using System.Diagnostics;

namespace AGS.CommandLine.Services
{
    public struct ProcessResult
    {
        public int ExitCode;
        public string Stdout;
        public string Stderr;
        public bool TimedOut;
    }

    /// <summary>
    /// Runs external processes synchronously, capturing stdout/stderr.
    /// </summary>
    public static class BuildRunner
    {
        private const int DEFAULT_TIMEOUT_MS = 120000; // 2 minutes

        public static ProcessResult Run(
            string exePath,
            string arguments,
            string workingDir,
            int timeoutMs = DEFAULT_TIMEOUT_MS)
        {
            var psi = new ProcessStartInfo
            {
                FileName               = exePath,
                Arguments              = arguments,
                WorkingDirectory       = workingDir,
                UseShellExecute        = false,
                RedirectStandardOutput = true,
                RedirectStandardError  = true,
                CreateNoWindow         = true
            };

            var result = new ProcessResult();

            using (var process = new Process { StartInfo = psi })
            {
                process.Start();
                bool exited = process.WaitForExit(timeoutMs);

                result.Stdout = process.StandardOutput.ReadToEnd();
                result.Stderr = process.StandardError.ReadToEnd();

                if (!exited)
                {
                    try { process.Kill(); } catch { }
                    result.TimedOut  = true;
                    result.ExitCode  = -1;
                    result.Stderr   += "\n[Process timed out after " + (timeoutMs / 1000) + "s]";
                }
                else
                {
                    result.ExitCode = process.ExitCode;
                }
            }

            return result;
        }
    }
}
