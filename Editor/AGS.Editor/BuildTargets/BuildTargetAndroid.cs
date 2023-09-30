using AGS.Editor.Preferences;
using AGS.Types;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Xml;
using AGS.Editor.Utils;
using System.Collections.ObjectModel;

namespace AGS.Editor
{
    enum IconAssetType
    {
        NoIconFiles,
        AllIconFiles,
        AllNonRoundIconFiles
    }

    class BuildTargetAndroid : BuildTargetBase
    {
        public const string ANDROID_DIR = "Android";

        private const string ANDROID_CFG = "android.cfg";

        private const string PROJECT_DIR = "icons\\android";
        private const string ICON_FILENAME = "ic_launcher.png";
        private const string ICON_ROUND_FILENAME = "ic_launcher_round.png";
        private const string ICON_RES_DIR = "app\\src\\main\\res";
        public static readonly IList<string> ICON_DIRS = new ReadOnlyCollection<string> (new List<string> {
            "mipmap-hdpi", "mipmap-mdpi", "mipmap-xhdpi", "mipmap-xxhdpi", "mipmap-xxxhdpi",
        });
        private bool UseGradleDaemon = false;

        private void WriteAndroidCfg(string outputDir)
        {
            string configPath = Path.Combine(outputDir, ANDROID_CFG);

            RuntimeSetup setup = Factory.AGSEditor.CurrentGame.DefaultSetup;

            // These option values are not present in the Setup properties at the moment,
            // so we read them from the file, in case the file has been modified by a user manually
            // (this is a temporary measure, to avoid not letting a user to define these values).
            var cfg = new Dictionary<string, Dictionary<string, string>>();
            NativeProxy.Instance.ReadIniFile(configPath, cfg);
            string config_enabled = Utilities.GetConfigString(cfg, "misc", "config_enabled", "1");
            string clear_cache = Utilities.GetConfigString(cfg, "compatibility", "clear_cache_on_room_change", "0");
            string sound_enabled = Utilities.GetConfigString(cfg, "sound", "enabled", "1");
            string sound_cache_size = Utilities.GetConfigString(cfg, "sound", "cache_size", "32768");
            string frame_drop = Utilities.GetConfigString(cfg, "video", "framedrop", "0");
            string super_sampling = Utilities.GetConfigString(cfg, "graphics", "super_sampling", "0");
            string logging = Utilities.GetConfigString(cfg, "debug", "logging", "0");

            cfg = new Dictionary<string, Dictionary<string, string>>();
            cfg.Add("compatibility", new Dictionary<string, string>());
            cfg.Add("controls", new Dictionary<string, string>());
            cfg.Add("debug", new Dictionary<string, string>());
            cfg.Add("graphics", new Dictionary<string, string>());
            cfg.Add("misc", new Dictionary<string, string>());
            cfg.Add("sound", new Dictionary<string, string>());
            cfg.Add("video", new Dictionary<string, string>());

            cfg["misc"]["config_enabled"] = config_enabled;

            // Misc options
            int rotation = (int)setup.Rotation;
            cfg["misc"]["rotation"] = rotation.ToString();
            cfg["misc"]["translation"] = setup.Translation;
            cfg["compatibility"]["clear_cache_on_room_change"] = clear_cache;

            // Touch-to-mouse options
            int mouse_emulation = (int)setup.TouchToMouseEmulation;
            int mouse_speed = (int)Math.Round(setup.MouseSpeed * 10.0f);
            int mouse_control_mode = (int)setup.TouchToMouseMotionMode;
            cfg["controls"]["mouse_emulation"] = mouse_emulation.ToString();
            cfg["controls"]["mouse_speed"] = mouse_speed.ToString();
            cfg["controls"]["mouse_method"] = mouse_control_mode.ToString();

            // Sound options
            cfg["sound"]["enabled"] = sound_enabled;
            cfg["sound"]["cache_size"] = sound_cache_size;

            // Video options
            cfg["video"]["framedrop"] = frame_drop;

            // Graphic options
            if (setup.GraphicsDriver == GraphicsDriver.Software) {
                cfg["graphics"]["renderer"] = "0";
            } else {
                cfg["graphics"]["renderer"] = "1";
            }

            if (setup.GraphicsFilter == "StdScale") {
                cfg["graphics"]["smoothing"] = "0";
            } else {
                cfg["graphics"]["smoothing"] = "1";
            }

            if (setup.FullscreenGameScaling == GameScaling.ProportionalStretch) {
                cfg["graphics"]["scaling"] = "1";
            } else if (setup.FullscreenGameScaling == GameScaling.StretchToFit) {
                cfg["graphics"]["scaling"] = "2";
            } else {
                cfg["graphics"]["scaling"] = "0";
            }

            cfg["graphics"]["super_sampling"] = super_sampling;
            cfg["graphics"]["smooth_sprites"] = setup.AAScaledSprites ? "1" : "0";

            // Debug options
            if (Factory.AGSEditor.CurrentGame.Settings.DebugMode) // Make sure to not have debug options in production
            {
                cfg["debug"]["show_fps"] = setup.ShowFPS ? "1" : "0";
                cfg["debug"]["logging"] = logging;
            } 
            else
            {
                cfg["debug"]["show_fps"] = "0";
                cfg["debug"]["logging"] = "0";
            }

            NativeProxy.Instance.WriteIniFile(configPath, cfg, true);
        }

        private IconAssetType GetGameIconType()
        {
            string projIconDir = PROJECT_DIR;

            if (!Directory.Exists(projIconDir)) return IconAssetType.NoIconFiles;

            bool hasAllIcIcons = true;
            bool hasAllRoundIcIcons = true;
            foreach(string icondir in ICON_DIRS)
            {
                string icPath = Path.Combine(projIconDir, icondir, ICON_FILENAME);
                string icRoundPath = Path.Combine(projIconDir, icondir, ICON_ROUND_FILENAME);

                hasAllIcIcons = hasAllIcIcons && File.Exists(icPath);
                hasAllRoundIcIcons = hasAllRoundIcIcons && File.Exists(icRoundPath);
            }

            if (hasAllIcIcons && hasAllRoundIcIcons) return IconAssetType.AllIconFiles;

            if (hasAllIcIcons) return IconAssetType.AllNonRoundIconFiles;

            return IconAssetType.NoIconFiles;
        }

        private void SetGameIcons(IconAssetType icType, string destDir)
        {
            string projIconDir = PROJECT_DIR;
            string destIconDir = Path.Combine(destDir, ICON_RES_DIR);

            switch (icType)
            {
                case IconAssetType.AllIconFiles:
                    foreach (string icondir in ICON_DIRS)
                    {
                        string icPathPartial = Path.Combine(icondir, ICON_FILENAME);
                        string icRoundPathPartial = Path.Combine(icondir, ICON_ROUND_FILENAME);
                        File.Copy(Path.Combine(projIconDir, icPathPartial), Path.Combine(destIconDir, icPathPartial), overwrite:true);
                        File.Copy(Path.Combine(projIconDir, icRoundPathPartial), Path.Combine(destIconDir, icRoundPathPartial), overwrite: true);
                    }
                    break;
                case IconAssetType.AllNonRoundIconFiles:
                    foreach (string icondir in ICON_DIRS)
                    {
                        string toRemoveRoundPath = Path.Combine(destIconDir, icondir, ICON_ROUND_FILENAME);
                        string icPathPartial = Path.Combine(icondir, ICON_FILENAME);
                        Utilities.TryDeleteFile(toRemoveRoundPath);
                        File.Copy(Path.Combine(projIconDir, icPathPartial), Path.Combine(destIconDir, icPathPartial), overwrite: true);
                    }
                    break;
            }
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

        private void WriteStringToFile(string fileName, string fileText)
        {
            Utilities.TryDeleteFile(fileName);

            FileStream stream = File.Create(fileName);
            byte[] bytes = Encoding.UTF8.GetBytes(fileText);
            stream.Write(bytes, 0, bytes.Length);
            stream.Close();
        }

        private void StopGradle()
        {
            if (!UseGradleDaemon) return;

            string prjDir = GetAndroidProjectInCompiledDir();
            string gradle_path = Path.Combine(prjDir, "gradlew.bat");
            if (!Directory.Exists(prjDir) || !File.Exists(gradle_path)) return;

            AndroidUtilities.RunGradlewStop(prjDir);
        }

        private void InstallSdkToolsIfNeeded()
        {
            string prjDir = GetAndroidProjectInCompiledDir();

            string packages = "\"build-tools;34.0.0\" \"ndk;25.2.9519653\" \"platforms;android-34\"";

            AndroidUtilities.RunSdkManager(packages, prjDir);
        }

        private string GetAndroidProjectInCompiledDir()
        {
            return GetCompiledPath(ANDROID_DIR, "mygame");
        }

        private string GetAssetEmbeddedDir()
        {
            return Path.Combine(GetAndroidProjectInCompiledDir(), "app\\src\\main\\assets");
        }

        private string GetAssetNonEmbeddedDir()
        {
            return Path.Combine(GetAndroidProjectInCompiledDir(), "game\\src\\main\\assets");
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
            string applicationId = gameSettings.AndroidApplicationId;
            string[] nameParts = applicationId.Split('.');
            int lastPart = nameParts.Length - 1;
            if (lastPart < 0) lastPart = 0;
            string ext = ".aab";
            if (gameSettings.AndroidBuildFormat == AndroidBuildFormat.ApkEmbedded) ext = ".apk";
            return "app-" + nameParts[lastPart] + "-release" + ext;
        }

        private bool IsProjectSane(CompileMessages errors)
        {
            foreach (string error in AndroidUtilities.GetPreferencesErrors())
            {
                errors.Add(new CompileError("Android Build: " + error));
            }

            return errors.Count == 0;
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
        
        private void WriteProjectXml(string dest_dir)
        {
            string fileName = Path.Combine(dest_dir, "app\\src\\main\\res\\values\\project.xml");

            using (TextWriter writer = File.CreateText(fileName))
            {
                string gameName = Factory.AGSEditor.CurrentGame.Settings.GameName;

                XmlDocument doc = new XmlDocument();
                XmlNode docNode = doc.CreateXmlDeclaration("1.0", "UTF-8", null);
                doc.AppendChild(docNode);

                XmlNode resourcesNode = doc.CreateElement("resources");
                doc.AppendChild(resourcesNode);

                XmlNode stringNode = doc.CreateElement("string");
                XmlAttribute stringAttribute = doc.CreateAttribute("name");
                stringAttribute.Value = "app_name";
                stringNode.Attributes.Append(stringAttribute);
                stringNode.InnerText = gameName;

                resourcesNode.AppendChild(stringNode);

                doc.Save(writer);
            }
        }

        private void WriteProjectProperties(string dest_dir)
        {
            string fileName = Path.Combine(dest_dir, "project.properties");

            try
            {
                Utilities.TryDeleteFile(fileName);
            }
            catch
            {
                StopGradle();
            }

            Settings gameSettings = Factory.AGSEditor.CurrentGame.Settings;
            string androidPackageName = gameSettings.AndroidApplicationId;
            string androidAppVersionId = gameSettings.AndroidAppVersionCode.ToString();
            string androidAppVersionName = gameSettings.AndroidAppVersionName;
            if (string.IsNullOrEmpty(androidPackageName)) androidPackageName = "com.mystudio.mygame";
            if (string.IsNullOrEmpty(androidAppVersionId)) androidAppVersionId = "1";
            if (string.IsNullOrEmpty(androidAppVersionName)) androidAppVersionName = gameSettings.Version; // use desktop version when empty

            string fileText = "applicationId=" + androidPackageName + "\n" +
                              "versionCode=" + androidAppVersionId + "\n" +
                              "versionName=" + androidAppVersionName + "\n";

            WriteStringToFile(fileName, fileText);
        }

        private void WriteLocalStaticProperties(string dest_dir)
        {
            string fileName = Path.Combine(dest_dir, "local.static.properties");

            try
            {
                Utilities.TryDeleteFile(fileName);
            }
            catch
            {
                StopGradle();
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
                "library\\runtime\\libs\\arm64-v8a\\libSDL2.so",
                "library\\runtime\\libs\\armeabi-v7a\\libags.so",
                "library\\runtime\\libs\\armeabi-v7a\\libengine.so",
                "library\\runtime\\libs\\armeabi-v7a\\libSDL2.so",
                "library\\runtime\\libs\\x86\\libags.so",
                "library\\runtime\\libs\\x86\\libengine.so",
                "library\\runtime\\libs\\x86\\libSDL2.so",
                "library\\runtime\\libs\\x86_64\\libags.so",
                "library\\runtime\\libs\\x86_64\\libengine.so",
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
                "library\\runtime\\src\\main\\java\\org\\libsdl\\app\\SDLSurface.java",
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
            DeleteCommonGameFiles(GetAssetsDir(), name);
        }

        public override bool Build(CompileMessages errors, bool forceRebuild)
        {
            if (!base.Build(errors, forceRebuild)) return false;

            UseGradleDaemon = Factory.AGSEditor.Settings.AndroidBuildGradleDaemon;
            string andProjDir = GetAndroidProjectInCompiledDir();

            if(!IsProjectSane(errors))
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
            GenerateConfigFile(assetsDir);
            WriteAndroidCfg(assetsDir);

            foreach (KeyValuePair<string, string> pair in GetRequiredLibraryPaths())
            {
                string fileName = pair.Key;
                string originDir = pair.Value;

                string dest = GetCompiledPath(ANDROID_DIR, Path.GetDirectoryName(fileName));

                if(!Directory.Exists(dest))
                {
                    // doesn't exist, let's create it
                    Directory.CreateDirectory(dest);
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

            string destDir = GetCompiledPath(ANDROID_DIR, "mygame");

            WriteProjectProperties(destDir);
            WriteLocalStaticProperties(destDir);
            WriteProjectXml(destDir);
            IconAssetType iconType = GetGameIconType();
            if(iconType != IconAssetType.NoIconFiles) SetGameIcons(iconType, destDir);

            GradleTasks gradleTask = GradleTasks.bundleRelease;
            if (buildFormat == AndroidBuildFormat.ApkEmbedded) gradleTask = GradleTasks.assembleRelease;

            if (!AndroidUtilities.RunGradlewTask(gradleTask, andProjDir, use_daemon:UseGradleDaemon))
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
