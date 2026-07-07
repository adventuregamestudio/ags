# How to Build AGS.CommandLine from Source

`AGS.CommandLine.exe` is the MCP server component added in this branch. It is a C# console
application that depends on `AGS.Types` (C#) and `AGS.Native` (C++/CLI, Win32).

---

## Prerequisites

- **Visual Studio 2022 Community** — https://visualstudio.microsoft.com  
  (VS 2019 also works; adjust MSBuild path accordingly)
- **.NET Framework 4.6 targeting pack** — install via VS Installer
- **.NET Framework 4.6.1 targeting pack** — install via VS Installer
- **MSVC v142 build tools** — install via VS Installer (required for AGS.Native C++)
- **Git** — to clone the repository
- **NuGet** — to restore packages (download `nuget.exe` from https://dist.nuget.org/win-x86-commandline/latest/nuget.exe if not available)

### VS Installer — Individual Components to Check

```
VS Installer → Modify → Individual Components
  ☑ .NET Framework 4.6 targeting pack
  ☑ .NET Framework 4.6.1 targeting pack
  ☑ MSVC v142 - VS 2019 C++ x64/x86 build tools
```

---

## Step 1: Clone the Repository

```powershell
git clone https://github.com/adventuregamestudio/ags.git
cd ags
```

---

## Step 2: Restore NuGet Packages

AGS.CommandLine requires `Newtonsoft.Json 13.0.1`. The `packages/` folder is not committed,
so you must restore it before building:

```powershell
# Download nuget.exe if you don't have it
Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile "$env:TEMP\nuget.exe"

# Restore packages for the full solution
& "$env:TEMP\nuget.exe" restore Solutions\AGS.Editor.Full.sln
```

This creates `Solutions\packages\Newtonsoft.Json.13.0.1\...`.

---

## Step 3: Build AGS.Native (C++/CLI Bridge)

AGS.CommandLine references `AGS.Native.dll`, which must be compiled before the C# project.
Build it via the solution (so MSBuild maps the C# platform correctly):

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

& $msbuild Solutions\AGS.Editor.Full.sln `
    /t:AGS_Native `
    /p:Configuration=Release `
    /p:Platform=Win32 `
    /v:minimal
```

Output: `Editor\AGS.Native\Release\AGS.Native.dll`

This step also builds `Common.Lib` and `Compiler.Lib` as dependencies. Expect ~60–90 seconds.

> **Note:** You will see several compiler warnings (signed/unsigned mismatch, deprecated
> POSIX names, etc.). These are pre-existing in the codebase and do not affect functionality.

---

## Step 4: Build AGS.CommandLine

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

& $msbuild Editor\AGS.CommandLine\AGS.CommandLine.csproj `
    /p:Configuration=Release `
    /p:Platform=AnyCPU `
    /v:minimal
```

Output: `Editor\AGS.CommandLine\bin\x86\Release\AGS.CommandLine.exe`

The build also copies all required DLLs alongside the exe:

| File | Source |
|------|--------|
| `AGS.CommandLine.exe` | Built by this project |
| `AGS.Native.dll` | `Editor\AGS.Native\Release\` |
| `AGS.Types.dll` | `Editor\AGS.Types\bin\Release\` |
| `Newtonsoft.Json.dll` | `Solutions\packages\Newtonsoft.Json.13.0.1\lib\net45\` |

---

## Step 5: Verify the Build

Run the exe to confirm it starts:

```powershell
.\Editor\AGS.CommandLine\bin\x86\Release\AGS.CommandLine.exe
```

You should see:
```
[AGS MCP] Server starting...
```

The process waits for stdin input (MCP JSON-RPC messages). Press `Ctrl+C` to exit.

---

## Alternative: Build via Visual Studio GUI

1. Open `Solutions\AGS.Editor.Full.sln` in Visual Studio 2022
2. Restore NuGet packages when prompted (or right-click solution → Restore NuGet Packages)
3. Set configuration to **Release | Win32**
4. Right-click **AGS.Native** → Build (builds C++ first)
5. Right-click **AGS.CommandLine** → Build

---

## Building the Full Editor (Optional)

If you also want the AGS Editor GUI (`AGSEditor.exe`), you additionally need:
- **irrKlang 1.6 (32-bit) for .NET** — place `irrKlang.NET4.dll` and `ikpMP3.dll`
  into `Editor\References\` (see `Editor\References\references.txt`)

Then build the full solution:

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

& $msbuild Solutions\AGS.Editor.Full.sln `
    /p:Configuration=Release `
    /p:Platform=Win32 `
    /v:minimal `
    /m
```

> The Editor is strictly 32-bit (Win32). AGS.CommandLine is AnyCPU but forced to x86
> at runtime to match AGS.Native.

---

## Troubleshooting

### `error MSB8020: The build tools for v142 cannot be found`

Install **MSVC v142 build tools** via VS Installer → Individual Components.

### `error CS0246: The type or namespace name 'JObject' could not be found`

NuGet packages were not restored. Run the `nuget restore` step (Step 2) again.

### `Could not locate the assembly "AGS.Native"`

AGS.Native.dll was not built. Complete Step 3 before Step 4.

### `error MSB3027: Could not copy — file in use`

Something is holding the exe (Kiro MCP session, terminal, etc.).
Stop all processes using the old exe, then rebuild.

### The exe crashes immediately on launch

All DLLs must be in the same folder as `AGS.CommandLine.exe`. Verify the four files
listed in Step 4 are all present in `bin\x86\Release\`.

---

## What's in AGS.CommandLine?

```
Program.cs          — stdin/stdout MCP loop (JSON-RPC 2.0)
McpProtocol.cs      — tool dispatch and response serialization
Tools/
  ProjectTools.cs   — project_info
  QueryTools.cs     — all read tools (character_list, room_hotspots, etc.)
  WriteTools.cs     — all write tools (character_add, room_add, dialog_add, etc.)
  EventTools.cs     — event_bind_*, script_function_*, declare_variable
  BuildTools.cs     — game_build_check, game_test_run, game_export
Services/
  GameLoader.cs     — loads Game.agf via AGS.Types deserialization
  GameSaver.cs      — saves Game.agf
  ScriptEditor.cs   — reads/writes .asc script files
  NativeRoomLoader.cs — loads/saves .crm binaries via AGS.Native
  BuildRunner.cs    — file validation and zip packaging
  ToolLocator.cs    — locates AGS Editor and acwin.exe
Resources/
  blank_room.crm    — embedded 320x200 empty room template
```

References:
- `AGS.Types` — shared data model (Game, Character, Room, Dialog, etc.)
- `AGS.Native` — C++/CLI bridge for reading/writing `.crm` room binaries
- `Newtonsoft.Json` — JSON-RPC serialization

No GUI. No irrKlang. Stdio only.
