using AGS.Editor.Preferences;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

namespace AGS.Editor.Utils
{
    public enum GradleTasks
    {
        bundleRelease,
        assembleRelease
    }

    public class AndroidKeystoreCertificateData
    {
        protected string _firstAndLastName;
        protected string _organizationalUnit;
        protected string _organizationName;
        protected string _cityOrLocality;
        protected string _stateOrProvince;
        protected string _countryCode;

        public string FirstAndLastName
        {
            set { _firstAndLastName = FilterCertificateString(value); }
            get { return _firstAndLastName; }
        }
        public string OrganizationalUnit
        {
            set { _organizationalUnit = FilterCertificateString(value); }
            get { return _organizationalUnit; }
        }
        public string OrganizationName
        {
            set { _organizationName = FilterCertificateString(value); }
            get { return _organizationName; }
        }
        public string CityOrLocality
        {
            set { _cityOrLocality = FilterCertificateString(value); }
            get { return _cityOrLocality; }
        }
        public string StateOrProvince
        {
            set { _stateOrProvince = FilterCertificateString(value); }
            get { return _stateOrProvince; }
        }
        public string CountryCode
        {
            set { _countryCode = FilterCertificateString(value); }
            get { return _countryCode; }
        }

        protected static string FilterCertificateString(string st)
        {
            return st.Replace("\"", "").Replace("^", "").Replace(",", "").Replace(";", "").Replace("=", "").Replace("%", "").Trim();
        }
    }

    public class AndroidKeystoreData : AndroidKeystoreCertificateData
    {
        private string _keystorePath;
        private string _password;
        private string _keyAlias;
        private string _keyPassword;
        private int _validityInYears;

        protected static string FilterString(string st)
        {
            st = FilterCertificateString(st);
            return st.Replace(" ", "").Replace("'", "");
        }

        public string KeystorePath
        {
            set { _keystorePath = FilterCertificateString(value); }
            get { return _keystorePath; }
        }

        public string Password
        {
            set { _password = FilterString(value); }
            get { return _password; }
        }

        public string KeyAlias
        {
            set { _keyAlias = FilterString(value); }
            get { return _keyAlias; }
        }

        public string KeyPassword
        {
            set { _keyPassword = FilterString(value); }
            get { return _keyPassword; }
        }

        public int ValidityInYears
        {
            set { _validityInYears = value < 0 ? 1 : value; }
            get { return _validityInYears; }
        }

        public AndroidKeystoreData Copy()
        {
            AndroidKeystoreData ks = new AndroidKeystoreData
            {
                _keystorePath = _keystorePath,
                _password = _password,
                _keyAlias = _keyAlias,
                _keyPassword = _keyPassword,
                _validityInYears = _validityInYears,

                _firstAndLastName = _firstAndLastName,
                _organizationalUnit = _organizationalUnit,
                _organizationName = _organizationName,
                _cityOrLocality = _cityOrLocality,
                _stateOrProvince = _stateOrProvince,
                _countryCode = _countryCode
            };

            return ks;
        }
    }


    internal class AndroidUtilities
    {

        private static bool RunCommand(string command, string args, string working_dir, bool show_window = true)
        {
            using (Process proc = new Process
            {
                StartInfo =
                {
                    UseShellExecute = false,
                    FileName = command,
                    Arguments = args,
                    CreateNoWindow = !show_window,
                    WorkingDirectory = working_dir
                }
            })
            {
                try
                {
                    proc.Start();
                    proc.WaitForExit();
                    return true;
                }
                catch
                {
                    return false;
                }
            }
        }

        private static bool RunInCmdEXE(string command, string working_dir, bool show_window = true)
        {
            return RunCommand(
                       command: "cmd.exe",
                       args: "/C " + command,
                       working_dir: working_dir,
                       show_window: show_window
                       );
        }

        private static string GetPrefixEnv()
        {
            string cmd = "";
            cmd += "set \"JAVA_HOME=" + GetJavaHome() + "\" & ";
            cmd += "set \"ANDROID_HOME=" + GetAndroidHome() + "\" & ";
            cmd += "set \"PATH=%JAVA_HOME%\\bin:%PATH%\" & ";
            return cmd;
        }

        private static bool RunGradlewTaskByName(string task_str, string working_dir, bool show_window = true)
        {
            string cmd = GetPrefixEnv();
            cmd += "gradlew.bat " + task_str + " & ";
            cmd += "if ERRORLEVEL 1 pause";

            return RunInCmdEXE(cmd, working_dir, show_window);
        }

        public static bool RunGradlewTask(GradleTasks task, string working_dir, bool show_window = true)
        {
            string task_str = task.ToString();
            return RunGradlewTaskByName(task_str, working_dir, show_window);
        }

        public static bool RunGradlewStop(string working_dir)
        {
            string cmd = GetPrefixEnv();
            cmd += "gradlew.bat --stop";
            return RunInCmdEXE(cmd, working_dir, show_window: false);
        }

        public static bool RunSdkManager(string parameters, string working_dir)
        {
            string sdkManager = GetSdkManagerPath();

            string cmd = GetPrefixEnv();
            cmd += sdkManager + " " + parameters;

            return RunInCmdEXE(cmd, working_dir, show_window: true);
        }

        private static string GetEnvironmentPreferences(string env_variable_name, string preferences_variable_value)
        {
            string variable_process = Environment.GetEnvironmentVariable(env_variable_name, EnvironmentVariableTarget.Process);
            string variable_user = Environment.GetEnvironmentVariable(env_variable_name, EnvironmentVariableTarget.User);
            string variable_machine = Environment.GetEnvironmentVariable(env_variable_name, EnvironmentVariableTarget.Machine);

            string[] vars_in_pref_order = new string[] {
                preferences_variable_value, variable_process, variable_user, variable_machine };

            for (int i = 0; i < vars_in_pref_order.Length; i++)
            {
                string variable = vars_in_pref_order[i];
                if (!string.IsNullOrEmpty(variable) && Directory.Exists(variable))
                {
                    return variable;
                }

            }

            return null;
        }

        private static string GetJavaHome()
        {
            AppSettings settings = Factory.AGSEditor.Settings;
            return GetEnvironmentPreferences("JAVA_HOME", settings.AndroidJavaHome);
        }

        private static string GetAndroidHome()
        {
            AppSettings settings = Factory.AGSEditor.Settings;
            return GetEnvironmentPreferences("ANDROID_HOME", settings.AndroidHome);
        }

        private static string GetSdkManagerPath()
        {
            return Path.Combine(GetAndroidHome(), "tools\\bin\\sdkmanager");
        }

        private static string GetJavacPath()
        {
            return Path.Combine(GetJavaHome(), "bin\\javac");
        }

        private static string GetKeytoolPath()
        {
            return Path.Combine(GetJavaHome(), "bin\\keytool");
        }

        private static bool IsExecutableAvailable(string path)
        {
            return (File.Exists(path) || File.Exists(path + ".exe") || File.Exists(path + ".bat"));
        }

        public static bool IsKeytoolAvailable()
        {
            return IsExecutableAvailable(GetKeytoolPath());
        }

        public static bool IsJdkFound(string dir)
        {
            string path = GetEnvironmentPreferences("JAVA_HOME", dir);

            if (string.IsNullOrEmpty(path) || !Directory.Exists(path)) return false;

            return IsExecutableAvailable(Path.Combine(path, "bin\\javac")) && IsExecutableAvailable(Path.Combine(path, "bin\\keytool"));
        }

        public static bool IsSdkFound(string dir)
        {
            string path = GetEnvironmentPreferences("ANDROID_HOME", dir);

            if (string.IsNullOrEmpty(path) || !Directory.Exists(path)) return false;

            return IsExecutableAvailable(Path.Combine(path, "tools\\bin\\sdkmanager"));
        }

        private static bool IsJavacAvailable()
        {
            return IsExecutableAvailable(GetJavacPath());
        }

        private static bool IsSdkManagerAvailable()
        {
            return IsExecutableAvailable(GetSdkManagerPath());
        }

        private static string FilterString(string st)
        {
            return Regex.Replace(st, @"[^\w\d _]", "").Trim();
        }

        public static string GetKeystoreCertificateInputString(AndroidKeystoreCertificateData data)
        {
            string strCN = data.FirstAndLastName;
            string strOU = data.OrganizationalUnit;
            string strO = data.OrganizationName;
            string strL = data.CityOrLocality;
            string cnST = data.StateOrProvince;
            string cnC = data.CountryCode;

            string cert = "";
            if (!string.IsNullOrEmpty(strCN)) cert += "cn=" + strCN + ", ";
            if (!string.IsNullOrEmpty(strOU)) cert += "ou=" + strOU + ", ";
            if (!string.IsNullOrEmpty(strO)) cert += "o=" + strO + ", ";
            if (!string.IsNullOrEmpty(strL)) cert += "l=" + strL + ", ";
            if (!string.IsNullOrEmpty(cnST)) cert += "st=" + cnST + ", ";
            if (!string.IsNullOrEmpty(cnC)) cert += "c=" + cnC;

            if (cert.Length > 2) return cert;

            return string.Empty;
        }

        private static string GetKeystoreGenerationArguments(AndroidKeystoreData d)
        {
            string cert = GetKeystoreCertificateInputString(d);

            string days = (d.ValidityInYears * 365).ToString();

            if (string.IsNullOrEmpty(cert)) cert = "cn=" + d.KeyAlias;

            var cmd = new ArgumentsBuilder(spacesBetweenFlagParam: true);

            cmd.AppendFlag("-genkeypair");
            cmd.AppendFlag("-noprompt");

            cmd.AppendFlagAndParameter("-keyalg", "RSA");
            cmd.AppendFlagAndParameter("-keysize", "4096");

            cmd.AppendFlagAndParameter("-dname", cert, quoteAlways: true);

            cmd.AppendFlagAndParameter("-alias", d.KeyAlias);
            cmd.AppendFlagAndParameter("-keypass", d.KeyPassword);
            cmd.AppendFlagAndParameter("-storepass", d.Password);
            cmd.AppendFlagAndParameter("-keystore", d.KeystorePath, quoteAlways: true);
            cmd.AppendFlagAndParameter("-validity", days);

            return cmd.ToString();
        }

        public static IEnumerable<string> GetKeystoreErrors(AndroidKeystoreData d)
        {
            List<string> errors = new List<string>();

            if (string.IsNullOrEmpty(d.KeystorePath)) errors.Add("Key store path can't be empty.");
            if (!string.IsNullOrEmpty(d.KeystorePath) && Directory.GetParent(d.KeystorePath) == null) errors.Add("Key store path directory doesn't exist!");
            if (!string.IsNullOrEmpty(d.KeystorePath) && string.IsNullOrEmpty(Path.GetExtension(d.KeystorePath))) errors.Add("Key store path must include desired filename with extension.");
            if (!string.IsNullOrEmpty(d.KeystorePath) && !Path.IsPathRooted(d.KeystorePath)) errors.Add("Key store path must be absolute.");
            if (!string.IsNullOrEmpty(d.KeystorePath) && File.Exists(d.KeystorePath)) errors.Add("Key store path must be a new file, it cannot overwrite an existing file.");

            if (string.IsNullOrEmpty(d.Password)) errors.Add("Password can't be empty.");
            if (string.IsNullOrEmpty(d.KeyPassword)) errors.Add("Key Password can't be empty.");
            if (d.KeyPassword != d.Password) errors.Add("Passwords don't match.");
            if (string.IsNullOrEmpty(d.KeyAlias)) errors.Add("Key alias cannot be empty.");

            if (d.ValidityInYears < 1) errors.Add("Validity cannot be lower than 1 year.");

            return errors;
        }

        public static bool RunGenerateKeystore(AndroidKeystoreData d)
        {
            if (!IsKeytoolAvailable()) return false;
            string keytool = GetKeytoolPath();
            string args = GetKeystoreGenerationArguments(d);
            string wdir = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);

            return RunCommand(command: keytool, args: args, working_dir: wdir, show_window: false);
        }

        public static IEnumerable<string> GetPreferencesErrors()
        {
            AppSettings preferences = Factory.AGSEditor.Settings;
            string javaHome = GetJavaHome();
            string androidHome = GetAndroidHome();

            List<string> errors = new List<string>();

            if (string.IsNullOrEmpty(javaHome))
            {
                errors.Add("JAVA_HOME is not set in Preferences or Environment variables.");
            }
            else
            {
                if (!Directory.Exists(javaHome) || !IsJavacAvailable())
                {
                    errors.Add("JAVA_HOME is not a valid JDK path. Can't find javac.");
                }
            }

            if (string.IsNullOrEmpty(androidHome))
            {
                errors.Add("ANDROID_HOME is not set in Preferences or Environment variables.");
            }
            else
            {
                if (!Directory.Exists(androidHome) || !IsSdkManagerAvailable())
                {
                    errors.Add("ANDROID_HOME is not a valid SDK path. Can't find sdkmanager.");
                }
            }

            if (string.IsNullOrEmpty(preferences.AndroidKeystoreFile))
            {
                errors.Add("Keystore File path was not set in Preferences.");
            }
            else
            {
                if (!File.Exists(preferences.AndroidKeystoreFile))
                {
                    errors.Add("Keystore File path is invalid.");
                }
            }

            return errors;
        }
    }
}
