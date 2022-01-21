using AGS.Types;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    class BuildTargetAndroid : BuildTargetBase
    {
        public const string ANDROID_DIR = "Android";

        private string EscapeFilenamePathStringAsNeeded(string str)
        {
            int len = str.Length;
            for (int i = 0; i < len; i++)
            {
                char c = str[i];
                if (c == '\\')
                {
                    char next_c = '\0';
                    char prev_c = '\0';

                    if(i+1 < len) next_c = str[i + 1];
                    if(i-1 > 0)   prev_c = str[i - 1];

                    if (next_c != '\\' && prev_c != '\\')
                    {
                        str = str.Insert(i, "\\");
                        len = str.Length;
                    }
                }
            }

            return str;
        }

        private void WriteStringToFile(string fileName, string fileText)
        {
            if (File.Exists(fileName))
            {
                File.Delete(fileName);
            }

            FileStream stream = File.Create(fileName);
            byte[] bytes = Encoding.UTF8.GetBytes(fileText);
            stream.Write(bytes, 0, bytes.Length);
            stream.Close();
        }

        private void WriteProjectProperties(string dest_dir)
        {
            string fileName = Path.Combine(dest_dir, "project.properties");

            Settings gameSettings = Factory.AGSEditor.CurrentGame.Settings;
            string androidPackageName = gameSettings.AndroidPackageName;
            string androidAppVersionId = gameSettings.AndroidAppVersionCode.ToString();
            string androidAppVersionName = gameSettings.AndroidAppVersionName;
            if (string.IsNullOrEmpty(androidPackageName)) androidPackageName = "com.mystudio.mygame";
            if (string.IsNullOrEmpty(androidAppVersionId)) androidAppVersionId = "1";
            if (string.IsNullOrEmpty(androidAppVersionName)) androidAppVersionName = "1.0";

            string fileText = "applicationId=" + androidPackageName + "\n" +
                              "versionCode=" + androidAppVersionId + "\n" +
                              "versionName=" + androidAppVersionName + "\n";

            WriteStringToFile(fileName, fileText);
        }

        private void WriteLocalStaticProperties(string dest_dir)
        {
            string fileName = Path.Combine(dest_dir, "local.static.properties");

            // this should NOT GET KEYS FROM SETTINGS
            Settings gameSettings = Factory.AGSEditor.CurrentGame.Settings;
            string storeFile = EscapeFilenamePathStringAsNeeded(gameSettings.AndroidKeystoreFile);
            string storePassword = gameSettings.AndroidKeystorePassword;
            string keyAlias = gameSettings.AndroidKeystoreKeyAlias;
            string keyPassword = gameSettings.AndroidKeystoreKeyPassword;

            string fileText = "storeFile=" + storeFile + "\n" +
                              "storePassword=" + storePassword + "\n" +
                              "keyAlias=" + keyAlias + "\n" +
                              "keyPassword=" + keyPassword + "\n";

            WriteStringToFile(fileName, fileText);
        }

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();

            string androidDir = Path.Combine(Factory.AGSEditor.EditorDirectory, ANDROID_DIR);

            List<string> files = new List<string>()
            {
                "gradle\\buildCacheSettings.gradle",
                "library\\runtime\\.gitignore",
                "library\\runtime\\build.gradle",
                "library\\runtime\\gradle.properties",
                "library\\runtime\\gradlew",
                "library\\runtime\\gradlew.bat",
                "library\\runtime\\proguard-rules.pro",
                "library\\runtime\\gradle\\wrapper\\gradle-wrapper.jar",
                "library\\runtime\\gradle\\wrapper\\gradle-wrapper.properties",
                "library\\runtime\\libs\\libs_go_here.txt",
                "library\\runtime\\libs\\arm64-v8a\\libags.so",
                "library\\runtime\\libs\\arm64-v8a\\libengine.so",
                "library\\runtime\\libs\\arm64-v8a\\libhidapi.so",
                "library\\runtime\\libs\\arm64-v8a\\libSDL2.so",
                "library\\runtime\\libs\\armeabi-v7a\\libags.so",
                "library\\runtime\\libs\\armeabi-v7a\\libengine.so",
                "library\\runtime\\libs\\armeabi-v7a\\libhidapi.so",
                "library\\runtime\\libs\\armeabi-v7a\\libSDL2.so",
                "library\\runtime\\libs\\x86\\libags.so",
                "library\\runtime\\libs\\x86\\libengine.so",
                "library\\runtime\\libs\\x86\\libhidapi.so",
                "library\\runtime\\libs\\x86\\libSDL2.so",
                "library\\runtime\\libs\\x86_64\\libags.so",
                "library\\runtime\\libs\\x86_64\\libengine.so",
                "library\\runtime\\libs\\x86_64\\libhidapi.so",
                "library\\runtime\\libs\\x86_64\\libSDL2.so",
                "library\\runtime\\src\\androidTest\\java\\uk\\co\\adventuregamestudio\\runtime\\ExampleInstrumentedTest.java",
                "library\\runtime\\src\\main\\AndroidManifest.xml",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\HIDDevice.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\HIDDeviceBLESteamController.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\HIDDeviceManager.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\HIDDeviceUSB.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\SDL.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\SDLActivity.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\SDLAudioManager.java",
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\SDLControllerManager.java",
                "library\\runtime\\src\\main\\java\\uk\\co\\adventuregamestudio\\runtime\\AGSRuntimeActivity.java",
                "library\\runtime\\src\\main\\java\\uk\\co\\adventuregamestudio\\runtime\\AgsSettingsFragment.java",
                "library\\runtime\\src\\main\\java\\uk\\co\\adventuregamestudio\\runtime\\CreditsActivity.java",
                "library\\runtime\\src\\main\\java\\uk\\co\\adventuregamestudio\\runtime\\NativeHelper.java",
                "library\\runtime\\src\\main\\java\\uk\\co\\adventuregamestudio\\runtime\\PreferencesActivity.java",
                "library\\runtime\\src\\main\\java\\uk\\co\\adventuregamestudio\\runtime\\ReadOnlyINI.java",
                "library\\runtime\\src\\main\\res\\drawable\\ic_launcher_background.xml",
                "library\\runtime\\src\\main\\res\\layout\\credits.xml",
                "library\\runtime\\src\\main\\res\\layout\\loading.xml",
                "library\\runtime\\src\\main\\res\\layout\\settings_activity.xml",
                "library\\runtime\\src\\main\\res\\mipmap-hdpi\\ic_launcher.png",
                "library\\runtime\\src\\main\\res\\mipmap-hdpi\\ic_launcher_round.png",
                "library\\runtime\\src\\main\\res\\mipmap-mdpi\\ic_launcher.png",
                "library\\runtime\\src\\main\\res\\mipmap-mdpi\\ic_launcher_round.png",
                "library\\runtime\\src\\main\\res\\mipmap-xhdpi\\ic_launcher.png",
                "library\\runtime\\src\\main\\res\\mipmap-xhdpi\\ic_launcher_round.png",
                "library\\runtime\\src\\main\\res\\mipmap-xxhdpi\\ic_launcher.png",
                "library\\runtime\\src\\main\\res\\mipmap-xxhdpi\\ic_launcher_round.png",
                "library\\runtime\\src\\main\\res\\mipmap-xxxhdpi\\ic_launcher.png",
                "library\\runtime\\src\\main\\res\\mipmap-xxxhdpi\\ic_launcher_round.png",
                "library\\runtime\\src\\main\\res\\values\\arrays.xml",
                "library\\runtime\\src\\main\\res\\values\\colors.xml",
                "library\\runtime\\src\\main\\res\\values\\strings.xml",
                "library\\runtime\\src\\main\\res\\values\\styles.xml",
                "library\\runtime\\src\\main\\res\\xml\\preferences.xml",
                "library\\runtime\\src\\main\\res\\menu\\default_ingame.xml",
                "library\\runtime\\src\\test\\java\\uk\\co\\adventuregamestudio\\runtime\\ExampleUnitTest.java",
                "mygame\\.gitignore",
                "mygame\\build.gradle",
                "mygame\\gradle.properties",
                "mygame\\gradlew",
                "mygame\\gradlew.bat",
                "mygame\\gradle\\wrapper\\gradle-wrapper.jar",
                "mygame\\gradle\\wrapper\\gradle-wrapper.properties",
                "mygame\\settings.gradle",
                "mygame\\app\\build.gradle",
                "mygame\\app\\src\\main\\AndroidManifest.xml",
                "mygame\\app\\src\\main\\assets\\android.cfg",
                "mygame\\app\\src\\main\\java\\com\\mystudioname\\mygamename\\App.java",
                "mygame\\app\\src\\main\\java\\com\\mystudioname\\mygamename\\MainActivity.java",
                "mygame\\app\\src\\main\\res\\drawable\\download_landscape.png",
                "mygame\\app\\src\\main\\res\\drawable\\download_portrait.png",
                "mygame\\app\\src\\main\\res\\layout\\downloader_ui.xml",
                "mygame\\app\\src\\main\\res\\layout-land\\downloader_ui.xml",
                "mygame\\app\\src\\main\\res\\mipmap-hdpi\\ic_launcher.png",
                "mygame\\app\\src\\main\\res\\mipmap-hdpi\\ic_launcher_round.png",
                "mygame\\app\\src\\main\\res\\mipmap-mdpi\\ic_launcher.png",
                "mygame\\app\\src\\main\\res\\mipmap-mdpi\\ic_launcher_round.png",
                "mygame\\app\\src\\main\\res\\mipmap-xhdpi\\ic_launcher.png",
                "mygame\\app\\src\\main\\res\\mipmap-xhdpi\\ic_launcher_round.png",
                "mygame\\app\\src\\main\\res\\mipmap-xxhdpi\\ic_launcher.png",
                "mygame\\app\\src\\main\\res\\mipmap-xxhdpi\\ic_launcher_round.png",
                "mygame\\app\\src\\main\\res\\mipmap-xxxhdpi\\ic_launcher.png",
                "mygame\\app\\src\\main\\res\\mipmap-xxxhdpi\\ic_launcher_round.png",
                "mygame\\app\\src\\main\\res\\values\\colors.xml",
                "mygame\\app\\src\\main\\res\\values\\project.xml",
                "mygame\\app\\src\\main\\res\\values\\strings.xml",
                "mygame\\app\\src\\main\\res\\values\\styles.xml",
                "mygame\\game\\build.gradle",
                "mygame\\game\\src\\main\\assets\\.gitkeep",
                "mygame\\play_licensing_library\\.gitignore",
                "mygame\\play_licensing_library\\build.gradle",
                "mygame\\play_licensing_library\\COPYING",
                "mygame\\play_licensing_library\\src\\main\\AndroidManifest.xml",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\AESObfuscator.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\APKExpansionPolicy.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\DeviceLimiter.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\ILicenseResultListener.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\ILicensingService.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\LicenseChecker.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\LicenseCheckerCallback.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\LicenseValidator.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\NullDeviceLimiter.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\Obfuscator.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\Policy.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\PreferenceObfuscator.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\ResponseData.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\ServerManagedPolicy.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\StrictPolicy.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\ValidationException.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\util\\Base64.java",
                "mygame\\play_licensing_library\\src\\main\\java\\com\\google\\android\\vending\\licensing\\util\\Base64DecoderException.java"
            };

            foreach(string file in files)
            {
                paths.Add(file, androidDir);
            }
            return paths;
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[] { GetCompiledPath() };
        }

        public override void DeleteMainGameData(string name)
        {
            string filename = Path.Combine(Path.Combine(OutputDirectoryFullPath, ANDROID_DIR), name + ".ags");
            Utilities.DeleteFileIfExists(filename);
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;

            string assets_dir = GetCompiledPath(ANDROID_DIR, "mygame\\app\\src\\main\\assets");
            string mygame_dir = GetCompiledPath(ANDROID_DIR, "mygame");

            if (!Directory.Exists(assets_dir))
            {
                // doesn't exist, let's create it
                Directory.CreateDirectory(assets_dir);
            }

            foreach (string fileName in Directory.GetFiles(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY)))
            {
                if ((File.GetAttributes(fileName) & (FileAttributes.Hidden | FileAttributes.System | FileAttributes.Temporary)) != 0)
                    continue;
                if ((!fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals(AGSEditor.CONFIG_FILE_NAME, StringComparison.OrdinalIgnoreCase)))
                {
                    string dest_filename = Path.Combine(assets_dir, Path.GetFileName(fileName));
                    Utilities.HardlinkOrCopy(dest_filename, fileName, true);
                }
            }


            // Update config file with current game parameters
            Factory.AGSEditor.WriteConfigFile(assets_dir);

            foreach (KeyValuePair<string, string> pair in GetRequiredLibraryPaths())
            {
                string fileName = pair.Key;
                string origin_dir = pair.Value;

                string dest_dir = GetCompiledPath(ANDROID_DIR, Path.GetDirectoryName(fileName));

                if(!Directory.Exists(dest_dir))
                {
                    // doesn't exist, let's create it
                    Directory.CreateDirectory(dest_dir);
                }

                string dest_file = GetCompiledPath(ANDROID_DIR, fileName);
                string origin_file = Path.Combine(origin_dir, fileName);
                string destFileName = Utilities.ResolveSourcePath(dest_file);
                string sourceFileName = Utilities.ResolveSourcePath(origin_file);

                if (fileName.EndsWith(".so")) { 
                    Utilities.HardlinkOrCopy(dest_file, origin_file, true);
                } 
                else
                {
                    File.Copy(sourceFileName, destFileName, true);
                }
            }

            WriteProjectProperties(GetCompiledPath(ANDROID_DIR, "mygame"));
            WriteLocalStaticProperties(GetCompiledPath(ANDROID_DIR, "mygame"));


            using (Process proc = new Process
            {
                StartInfo =
                {
                    UseShellExecute = false,
                    FileName = "cmd.exe",
                    Arguments = "/C gradlew.bat assembleRelease & pause",
                    CreateNoWindow = false,
                    WorkingDirectory = mygame_dir
                }
            })
            {
                try
                {
                    proc.Start();
                    proc.WaitForExit();
                }
                catch
                {
                    return false;
                }
            }

            return true;
        }

        public override string Name
        {
            get
            {
                return "Android";
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return ANDROID_DIR;
            }
        }
    }
}