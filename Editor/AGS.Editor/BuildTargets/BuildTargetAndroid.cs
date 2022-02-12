using AGS.Editor.Preferences;
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
        public const string ANDROID_CFG = "android.cfg";

        private void WriteAndroidCfg(string outputDir)
        {
            string configPath = Path.Combine(outputDir, ANDROID_CFG);

            RuntimeSetup setup = Factory.AGSEditor.CurrentGame.DefaultSetup;
            int rotation = (int)setup.Rotation;

            NativeProxy.WritePrivateProfileString("misc", "config_enabled", "1", configPath);
            NativeProxy.WritePrivateProfileString("misc", "rotation", rotation.ToString(), configPath);
            NativeProxy.WritePrivateProfileString("misc", "translation", setup.Translation, configPath);

            NativeProxy.WritePrivateProfileString("controls", "mouse_method", "0", configPath);
            NativeProxy.WritePrivateProfileString("controls", "mouse_longclick", "1", configPath);

            NativeProxy.WritePrivateProfileString("compatibility", "clear_cache_on_room_change", "0", configPath);

            NativeProxy.WritePrivateProfileString("sound", "samplerate", "44100", configPath);
            NativeProxy.WritePrivateProfileString("sound", "enabled", "1", configPath);
            NativeProxy.WritePrivateProfileString("sound", "threaded", "1", configPath);
            NativeProxy.WritePrivateProfileString("sound", "cache_size", "64", configPath);

            NativeProxy.WritePrivateProfileString("midi", "enabled", "0", configPath);
            NativeProxy.WritePrivateProfileString("midi", "preload_patches", "0", configPath);

            NativeProxy.WritePrivateProfileString("video", "framedrop", "0", configPath);

            if (setup.GraphicsDriver == GraphicsDriver.Software) {
                NativeProxy.WritePrivateProfileString("graphics", "renderer", "0", configPath);
            } else {
                NativeProxy.WritePrivateProfileString("graphics", "renderer", "1", configPath);
            }

            if (setup.GraphicsFilter == "StdScale") {
                NativeProxy.WritePrivateProfileString("graphics", "smoothing", "0", configPath);
            } else {
                NativeProxy.WritePrivateProfileString("graphics", "smoothing", "1", configPath);
            }

            if (setup.FullscreenGameScaling == GameScaling.ProportionalStretch) {
                NativeProxy.WritePrivateProfileString("graphics", "scaling", "1", configPath);
            } else if (setup.FullscreenGameScaling == GameScaling.StretchToFit) {
                NativeProxy.WritePrivateProfileString("graphics", "scaling", "2", configPath);
            } else {
                NativeProxy.WritePrivateProfileString("graphics", "scaling", "0", configPath);
            }

            NativeProxy.WritePrivateProfileString("graphics", "super_sampling", "0", configPath);
            NativeProxy.WritePrivateProfileString("graphics", "smooth_sprites", setup.AAScaledSprites ? "1" : "0", configPath);

            NativeProxy.WritePrivateProfileString("debug", "show_fps", "0", configPath);
            NativeProxy.WritePrivateProfileString("debug", "logging", "0", configPath);
        }

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

        private string GetEnvironmentPreferences(string env_variable_name, string preferences_variable_value)
        {
            string variable_process = Environment.GetEnvironmentVariable(env_variable_name, EnvironmentVariableTarget.Process);
            string variable_user = Environment.GetEnvironmentVariable(env_variable_name, EnvironmentVariableTarget.User);
            string variable_machine = Environment.GetEnvironmentVariable(env_variable_name, EnvironmentVariableTarget.Machine);

            string[] vars_in_pref_order = new string[] { 
                preferences_variable_value, variable_process, variable_user, variable_machine };

            for(int i=0; i<vars_in_pref_order.Length; i++)
            {
                string variable = vars_in_pref_order[i];
                if (!string.IsNullOrEmpty(variable) && Directory.Exists(variable))
                {
                    return variable;
                }

            }

            return null;
        }

        private string GetJavaHome()
        {
            AppSettings settings = Factory.AGSEditor.Settings;
            return GetEnvironmentPreferences("JAVA_HOME", settings.AndroidJavaHome);
        }

        private string GetAndroidHome()
        {
            AppSettings settings = Factory.AGSEditor.Settings;
            return GetEnvironmentPreferences("ANDROID_HOME", settings.AndroidHome);
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

        private bool RunCommand(string command, string working_dir, bool show_window = true)
        {

            using (Process proc = new Process
            {
                StartInfo =
                {
                    UseShellExecute = false,
                    FileName = "cmd.exe",
                    Arguments = command,
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
            return false;
        }

        private void StopGradle()
        {
            string prjDir = GetProjectDir();
            string gradle_path = Path.Combine(prjDir, "gradlew.bat");
            if (!Directory.Exists(prjDir) || !File.Exists(gradle_path)) return;

            RunCommand("/C gradlew.bat --stop", prjDir, false);
        }

        private void InstallSdkToolsIfNeeded()
        {
            string androidHome = GetAndroidHome();
            string javaHome = GetJavaHome();
            string prjDir = GetProjectDir();

            string packages = "\"build-tools;30.0.3\" \"ndk;21.3.6528147\" \"platforms;android-29\"";

            RunCommand("/C " + "set \"JAVA_HOME=" + javaHome + "\" & " +
                      "set \"ANDROID_HOME=" + androidHome + "\" & "    +
                      androidHome + "\\tools\\bin\\sdkmanager " + packages, prjDir, true);
        }

        private string GetProjectDir()
        {
            return GetCompiledPath(ANDROID_DIR, "mygame");
        }

        private string GetAssetEmbeddedDir()
        {
            return Path.Combine(GetProjectDir(), "app\\src\\main\\assets");
        }

        private string GetAssetNonEmbeddedDir()
        {
            return Path.Combine(GetProjectDir(), "game\\src\\main\\assets");
        }

        private string GetAssetsDir()
        {
            AndroidBuildFormat buildFormat = Factory.AGSEditor.CurrentGame.Settings.AndroidBuildFormat;
            bool destinationIsMainAssets = buildFormat != AndroidBuildFormat.Aab;
            return destinationIsMainAssets ? GetAssetEmbeddedDir() : GetAssetNonEmbeddedDir();
        }

        private void ClearInvalidAssetDirIfNeeded()
        {
            AndroidBuildFormat buildFormat = Factory.AGSEditor.CurrentGame.Settings.AndroidBuildFormat;
            bool clearEmbeddedDir = buildFormat == AndroidBuildFormat.Aab;
            string dirToClear = clearEmbeddedDir ? GetAssetEmbeddedDir() : GetAssetNonEmbeddedDir();
            if(Directory.Exists(dirToClear) && Directory.GetFiles(dirToClear).Length != 0)
            {
                Directory.Delete(dirToClear, true);
                Directory.CreateDirectory(dirToClear);
            }
        }

        private string GetFinalAppName()
        {
            Settings gameSettings = Factory.AGSEditor.CurrentGame.Settings;
            string androidPackageName = gameSettings.AndroidPackageName;
            string[] nameParts = androidPackageName.Split('.');
            int lastPart = nameParts.Length - 1;
            if (lastPart < 0) lastPart = 0;
            string ext = ".aab";
            if (gameSettings.AndroidBuildFormat == AndroidBuildFormat.ApkEmbedded) ext = ".apk";
            return "app-" + nameParts[lastPart] + "-release" + ext;
        }

        private string GetOutputDir()
        {
            Settings gameSettings = Factory.AGSEditor.CurrentGame.Settings;
            string outputBuildDir = GetCompiledPath(ANDROID_DIR, "mygame");
            if (gameSettings.AndroidBuildFormat == AndroidBuildFormat.ApkEmbedded)
            {
                outputBuildDir = Path.Combine(outputBuildDir, "app\\build\\outputs\\apk\\release");
            } 
            else
            {
                outputBuildDir = Path.Combine(outputBuildDir, "app\\build\\outputs\\bundle\\release");
            }
            return outputBuildDir;
        }
        
        private void WriteProjectProperties(string dest_dir)
        {
            string fileName = Path.Combine(dest_dir, "project.properties");

            if(File.Exists(fileName))
            {
                try
                {
                    File.Delete(fileName);
                }
                catch
                {
                    StopGradle();
                }
            }

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

            if (File.Exists(fileName))
            {
                try
                {
                    File.Delete(fileName);
                }
                catch
                {
                    StopGradle();
                }
            }

            // this should NOT GET KEYS FROM SETTINGS
            AppSettings settings = Factory.AGSEditor.Settings;
            string storeFile = EscapeFilenamePathStringAsNeeded(settings.AndroidKeystoreFile);
            string storePassword = settings.AndroidKeystorePassword;
            string keyAlias = settings.AndroidKeystoreKeyAlias;
            string keyPassword = settings.AndroidKeystoreKeyPassword;

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
            string assetsDir = GetAssetsDir();
            string filename = Path.Combine(assetsDir, name + ".ags");
            Utilities.DeleteFileIfExists(filename);
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;

            string prjDir = GetProjectDir();
            string java_home = GetJavaHome();
            string android_home = GetAndroidHome();

            if (string.IsNullOrEmpty(java_home))
            {
                errors.Add(new CompileError("Android Build: JAVA_HOME is not set in Preferences or Environment variables. Can't find Java."));
            }
            if (string.IsNullOrEmpty(android_home))
            {
                errors.Add(new CompileError("Android Build: ANDROID_HOME is not set in Preferences or Environment variables. Can't find Android SDK."));
            }
            if(errors.Count > 0)
            {
                return false;

            }

            InstallSdkToolsIfNeeded();

            AndroidBuildFormat buildFormat = Factory.AGSEditor.CurrentGame.Settings.AndroidBuildFormat;
            string appName = GetFinalAppName();

            string assetsDir = GetAssetsDir();
            if (!Directory.Exists(assetsDir)) Directory.CreateDirectory(assetsDir);
            ClearInvalidAssetDirIfNeeded();

            foreach (string fileName in Directory.GetFiles(Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY)))
            {
                if ((File.GetAttributes(fileName) & (FileAttributes.Hidden | FileAttributes.System | FileAttributes.Temporary)) != 0)
                    continue;
                if ((!fileName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase)) &&
                    (!Path.GetFileName(fileName).Equals(AGSEditor.CONFIG_FILE_NAME, StringComparison.OrdinalIgnoreCase)))
                {
                    string dest_filename = Path.Combine(assetsDir, Path.GetFileName(fileName));
                    Utilities.HardlinkOrCopy(dest_filename, fileName, true);
                }
            }

            // Update config file with current game parameters
            Factory.AGSEditor.WriteConfigFile(assetsDir);
            WriteAndroidCfg(assetsDir);

            foreach (KeyValuePair<string, string> pair in GetRequiredLibraryPaths())
            {
                string fileName = pair.Key;
                string originDir = pair.Value;

                string destDir = GetCompiledPath(ANDROID_DIR, Path.GetDirectoryName(fileName));

                if(!Directory.Exists(destDir))
                {
                    // doesn't exist, let's create it
                    Directory.CreateDirectory(destDir);
                }

                string destFile = GetCompiledPath(ANDROID_DIR, fileName);
                string originFile = Path.Combine(originDir, fileName);
                string destFileName = Utilities.ResolveSourcePath(destFile);
                string sourceFileName = Utilities.ResolveSourcePath(originFile);

                if (fileName.EndsWith(".so")) { 
                    Utilities.HardlinkOrCopy(destFile, originFile, true);
                } else {
                    File.Copy(sourceFileName, destFileName, true);
                }
            }

            WriteProjectProperties(GetCompiledPath(ANDROID_DIR, "mygame"));
            WriteLocalStaticProperties(GetCompiledPath(ANDROID_DIR, "mygame"));

            string gradle_task = "bundleRelease";
            if (buildFormat == AndroidBuildFormat.ApkEmbedded) gradle_task = "assembleRelease";

            string buildCmd = "";
            buildCmd += "/C ";
            buildCmd += "set \"JAVA_HOME=" + java_home + "\" & ";
            buildCmd += "set \"ANDROID_HOME=" + android_home + "\" & ";
            buildCmd += "set \"PATH=%JAVA_HOME%\\bin:%PATH%\" & ";
            buildCmd += "gradlew.bat " + gradle_task + " & ";
            buildCmd += "pause";

            if (!RunCommand(buildCmd, prjDir))
            {
                errors.Add(new CompileError("There was an error running gradle to build the Android App."));
                return false;
            }

            string appFinalFile = GetFinalAppName();
            string appSrcDir = GetOutputDir();
            string generatedAppFile = Path.Combine(appSrcDir, appFinalFile);

            if(File.Exists(generatedAppFile))
            {
                try
                {
                    Utilities.HardlinkOrCopy(GetCompiledPath(ANDROID_DIR, appFinalFile), generatedAppFile, true);
                }
                catch { }

                return true;
            }

            errors.Add(new CompileError("Failed to generate Android App."));

            return false;
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
