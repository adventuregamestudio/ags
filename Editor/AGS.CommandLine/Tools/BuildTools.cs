using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using Newtonsoft.Json.Linq;
using AGS.Types;
using AGS.CommandLine.Services;

namespace AGS.CommandLine.Tools
{
    public static class BuildTools
    {
        private static JObject Err(string msg)
            => new JObject { ["success"] = false, ["error"] = msg };

        private static string GetProjectPath(JObject args)
        {
            string p = args["project_path"] != null ? args["project_path"].ToString() : null;
            if (string.IsNullOrEmpty(p)) throw new ArgumentException("project_path is required");
            return p;
        }

        // ── game_build_check ───────────────────────────────────────────────────

        public static JToken GameBuildCheck(JObject args)
        {
            try
            {
                string projectPath = GetProjectPath(args);
                bool checkScripts = args["check_scripts"] == null || (bool)args["check_scripts"];
                bool checkSprites = args["check_sprites"] == null || (bool)args["check_sprites"];
                bool checkRooms   = args["check_rooms"]   == null || (bool)args["check_rooms"];
                bool checkAudio   = args["check_audio"]   == null || (bool)args["check_audio"];

                // Load game to get entity lists
                Game game = new GameLoader().Load(projectPath);

                var checks  = new JArray();
                bool canBuild = true;

                // Script check: verify all .asc files exist on disk
                if (checkScripts)
                {
                    var errors = new JArray();
                    foreach (ScriptAndHeader sh in game.ScriptsAndHeaders)
                    {
                        if (sh.Script != null)
                        {
                            string full = Path.Combine(projectPath, sh.Script.FileName);
                            if (!File.Exists(full))
                                errors.Add(new JObject { ["message"] = string.Format("Script file missing: {0}", sh.Script.FileName) });
                        }
                    }
                    bool passed = errors.Count == 0;
                    if (!passed) canBuild = false;
                    checks.Add(new JObject
                    {
                        ["category"] = "scripts",
                        ["passed"]   = passed,
                        ["message"]  = passed ? "All script files present on disk" : null,
                        ["errors"]   = errors
                    });
                }

                // Sprite check: verify sprite file exists
                if (checkSprites)
                {
                    string sprFile = Path.Combine(projectPath, "acsprset.spr");
                    bool passed = File.Exists(sprFile);
                    if (!passed) canBuild = false;
                    checks.Add(new JObject
                    {
                        ["category"] = "sprites",
                        ["passed"]   = passed,
                        ["message"]  = passed ? "Sprite file present" : null,
                        ["errors"]   = passed ? new JArray() : new JArray(new JObject { ["message"] = "acsprset.spr not found" })
                    });
                }

                // Room check: verify .crm files exist for all rooms
                if (checkRooms)
                {
                    var errors = new JArray();
                    foreach (IRoom r in game.Rooms)
                    {
                        string crm = Path.Combine(projectPath, r.FileName);
                        if (!File.Exists(crm))
                            errors.Add(new JObject { ["message"] = string.Format("Room file missing: {0}", r.FileName) });
                    }
                    bool passed = errors.Count == 0;
                    if (!passed) canBuild = false;
                    checks.Add(new JObject
                    {
                        ["category"] = "rooms",
                        ["passed"]   = passed,
                        ["message"]  = passed ? string.Format("All {0} room files present", game.Rooms.Count) : null,
                        ["errors"]   = errors
                    });
                }

                // Audio check: verify audio source files exist
                if (checkAudio)
                {
                    var errors  = new JArray();
                    int missing = 0;
                    foreach (AudioClip clip in game.AudioClips)
                    {
                        if (!string.IsNullOrEmpty(clip.SourceFileName))
                        {
                            string full = Path.IsPathRooted(clip.SourceFileName)
                                ? clip.SourceFileName
                                : Path.Combine(projectPath, clip.SourceFileName);
                            if (!File.Exists(full))
                            {
                                missing++;
                                errors.Add(new JObject { ["message"] = string.Format("Audio file missing: {0}", clip.SourceFileName) });
                            }
                        }
                    }
                    bool passed = errors.Count == 0;
                    if (!passed) canBuild = false;
                    checks.Add(new JObject
                    {
                        ["category"] = "audio",
                        ["passed"]   = passed,
                        ["message"]  = passed ? "All audio source files present (or no audio)" : null,
                        ["errors"]   = errors
                    });
                }

                return new JObject
                {
                    ["success"]   = true,
                    ["can_build"] = canBuild,
                    ["message"]   = canBuild ? "Project is ready to build" : "Project has validation errors",
                    ["checks"]    = checks
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── game_build ─────────────────────────────────────────────────────────
        // Attempts to invoke the AGS Editor headlessly or reports the last known
        // compiled output. Since the AGS Editor has no headless build mode, we
        // check for an existing Compiled/ output and report its state.
        // To actually build, the user must use the AGS Editor GUI.

        public static JToken GameBuild(JObject args)
        {
            try
            {
                string projectPath = GetProjectPath(args);
                string outputDir   = args["output_dir"] != null
                    ? args["output_dir"].ToString()
                    : Path.Combine(projectPath, "Compiled", "Windows");

                // Check if pre-built output exists
                if (!Directory.Exists(outputDir))
                    return new JObject
                    {
                        ["success"] = true,
                        ["built"]   = false,
                        ["warning"] = "No rebuild performed. Use AGS Editor to recompile scripts and assets.",
                        ["message"] = string.Format(
                            "Output directory not found: {0}. Build the project with the AGS Editor first, " +
                            "or use game_test_run to launch an existing build.", outputDir),
                        ["output_dir"] = outputDir
                    };

                // Find .ags game package in output dir
                var agsFiles = Directory.GetFiles(outputDir, "*.ags");
                var exeFiles = Directory.GetFiles(outputDir, "*.exe");

                if (agsFiles.Length == 0)
                    return new JObject
                    {
                        ["success"] = true,
                        ["built"]   = false,
                        ["warning"] = "No rebuild performed. Use AGS Editor to recompile scripts and assets.",
                        ["message"] = "No .ags package found in output directory. Build with the AGS Editor first.",
                        ["output_dir"] = outputDir
                    };

                var runtimeFiles = new JArray();
                foreach (string f in Directory.GetFiles(outputDir))
                    runtimeFiles.Add(f);

                long totalSize = 0;
                foreach (string f in Directory.GetFiles(outputDir))
                    totalSize += new FileInfo(f).Length;

                return new JObject
                {
                    ["success"]  = true,
                    ["built"]    = true,
                    ["warning"]  = "No rebuild performed. Use AGS Editor to recompile scripts and assets.",
                    ["message"]  = "Existing compiled build found",
                    ["output_dir"] = outputDir,
                    ["output_files"] = new JObject
                    {
                        ["game_package"]   = agsFiles[0],
                        ["game_exe"]       = exeFiles.Length > 0 ? exeFiles[0] : null,
                        ["runtime_files"]  = runtimeFiles
                    },
                    ["file_sizes"] = new JObject
                    {
                        ["total_kb"] = totalSize / 1024
                    }
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── game_test_run ──────────────────────────────────────────────────────

        public static JToken GameTestRun(JObject args)
        {
            try
            {
                string projectPath  = GetProjectPath(args);
                bool windowed       = args["windowed_mode"] == null || (bool)args["windowed_mode"];
                string outputDir    = Path.Combine(projectPath, "Compiled", "Windows");

                // Find the game exe
                string[] exeFiles = Directory.Exists(outputDir)
                    ? Directory.GetFiles(outputDir, "*.exe")
                    : new string[0];

                // Filter out winsetup.exe
                string gameExe = null;
                foreach (string f in exeFiles)
                    if (!Path.GetFileName(f).Equals("winsetup.exe", StringComparison.OrdinalIgnoreCase))
                    { gameExe = f; break; }

                if (gameExe == null)
                    return new JObject
                    {
                        ["success"]  = true,
                        ["launched"] = false,
                        ["message"]  = "No compiled game exe found. Build the project with the AGS Editor first.",
                        ["output_dir"] = outputDir
                    };

                // Check if build is stale (source files modified after exe)
                DateTime exeTime = File.GetLastWriteTime(gameExe);
                var staleSources = new JArray();
                bool buildIsStale = false;

                // Check .asc files (script source)
                Game game = new GameLoader().Load(projectPath);
                foreach (ScriptAndHeader sh in game.ScriptsAndHeaders)
                {
                    if (sh.Script != null)
                    {
                        string ascPath = Path.IsPathRooted(sh.Script.FileName)
                            ? sh.Script.FileName
                            : Path.Combine(projectPath, sh.Script.FileName);
                        if (File.Exists(ascPath) && File.GetLastWriteTime(ascPath) > exeTime)
                        {
                            buildIsStale = true;
                            staleSources.Add(sh.Script.FileName);
                        }
                    }
                }

                // Check .crm files (compiled rooms)
                foreach (IRoom r in game.Rooms)
                {
                    string crmPath = Path.Combine(projectPath, r.FileName);
                    if (File.Exists(crmPath) && File.GetLastWriteTime(crmPath) > exeTime)
                    {
                        buildIsStale = true;
                        staleSources.Add(r.FileName);
                    }
                }

                // Check Game.agf (project file)
                string agfPath = Path.Combine(projectPath, "Game.agf");
                if (File.Exists(agfPath) && File.GetLastWriteTime(agfPath) > exeTime)
                {
                    buildIsStale = true;
                    staleSources.Add("Game.agf");
                }

                // Build arguments
                string launchArgs = windowed ? "--windowed" : string.Empty;

                var psi = new ProcessStartInfo
                {
                    FileName         = gameExe,
                    Arguments        = launchArgs,
                    WorkingDirectory = outputDir,
                    UseShellExecute  = false
                };

                var proc = Process.Start(psi);

                var result = new JObject
                {
                    ["success"]    = true,
                    ["launched"]   = true,
                    ["message"]    = string.Format("Game launched: {0}", Path.GetFileName(gameExe)),
                    ["process_id"] = proc != null ? proc.Id : 0,
                    ["exe_path"]   = gameExe,
                    ["build_is_stale"] = buildIsStale
                };

                if (buildIsStale)
                {
                    result["stale_hint"] = "The build is stale — some source files were modified after the exe was built. " +
                                           "Rebuild with the AGS Editor to apply changes.";
                    result["stale_sources"] = staleSources;
                }

                return result;
            }
            catch (Exception ex) { return Err(ex.Message); }
        }

        // ── game_export ────────────────────────────────────────────────────────

        public static JToken GameExport(JObject args)
        {
            try
            {
                string projectPath = GetProjectPath(args);
                string exportDir   = args["export_dir"] != null ? args["export_dir"].ToString() : null;
                if (string.IsNullOrEmpty(exportDir))
                    return Err("export_dir is required");

                string version   = args["version"] != null ? args["version"].ToString() : "1.0.0";
                string sourceDir = Path.Combine(projectPath, "Compiled", "Windows");

                if (!Directory.Exists(sourceDir))
                    return new JObject
                    {
                        ["success"]   = true,
                        ["exported"]  = false,
                        ["message"]   = "No compiled output found. Build with the AGS Editor first.",
                        ["source_dir"] = sourceDir
                    };

                string[] agsFiles = Directory.GetFiles(sourceDir, "*.ags");
                if (agsFiles.Length == 0)
                    return new JObject
                    {
                        ["success"]  = true,
                        ["exported"] = false,
                        ["message"]  = "No .ags package in Compiled/Windows. Build with the AGS Editor first."
                    };

                // Load game for metadata
                Game game = new GameLoader().Load(projectPath);
                string gameName = game.Settings.GameName ?? Path.GetFileName(projectPath);

                // Create export directory
                Directory.CreateDirectory(exportDir);

                // Copy Compiled/Windows → exportDir/Windows/
                string destDir = Path.Combine(exportDir, "Windows");
                if (Directory.Exists(destDir)) Directory.Delete(destDir, true);
                Directory.CreateDirectory(destDir);

                foreach (string file in Directory.GetFiles(sourceDir))
                    File.Copy(file, Path.Combine(destDir, Path.GetFileName(file)), overwrite: true);

                // Create zip
                string zipPath = Path.Combine(exportDir, string.Format("{0}-{1}-Windows.zip",
                    gameName.Replace(" ", "_"), version));
                if (File.Exists(zipPath)) File.Delete(zipPath);
                ZipFile.CreateFromDirectory(destDir, zipPath);

                long zipSize = new FileInfo(zipPath).Length;

                // Write README.txt
                string readmePath = Path.Combine(exportDir, "README.txt");
                File.WriteAllText(readmePath, string.Format(
                    "{0}\nVersion: {1}\nBuilt with Adventure Game Studio\nBuild date: {2}\n",
                    gameName, version, DateTime.Now.ToString("yyyy-MM-dd")));

                // Write VERSION.txt
                string versionPath = Path.Combine(exportDir, "VERSION.txt");
                File.WriteAllText(versionPath, string.Format(
                    "{0}\n{1}\n", version, DateTime.UtcNow.ToString("o")));

                return new JObject
                {
                    ["success"]    = true,
                    ["exported"]   = true,
                    ["message"]    = string.Format("{0} v{1} exported successfully", gameName, version),
                    ["export_dir"] = exportDir,
                    ["files"] = new JObject
                    {
                        ["windows_zip"] = zipPath,
                        ["readme"]      = readmePath,
                        ["version_txt"] = versionPath
                    },
                    ["archive_sizes"] = new JObject
                    {
                        ["windows_kb"] = zipSize / 1024,
                        ["total_kb"]   = zipSize / 1024
                    }
                };
            }
            catch (Exception ex) { return Err(ex.Message); }
        }
    }
}
