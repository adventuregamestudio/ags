# AGS.CommandLine — MCP Server

`AGS.CommandLine.exe` is a [Model Context Protocol (MCP)](https://modelcontextprotocol.io/) server that lets AI assistants (e.g. Kiro, Claude Desktop) build Adventure Game Studio games programmatically over stdio.

It exposes **47 tools** covering the full game development workflow: creating rooms, characters, hotspots, dialogs, inventory items, audio clips, script functions, event bindings, build validation, and game export.

---

## Quick Start

### Build

See [`docs/BUILD_INSTRUCTIONS.md`](docs/BUILD_INSTRUCTIONS.md) for full prerequisites and steps.

**Short version (Visual Studio 2022):**

```powershell
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"

# Step 1: restore NuGet packages (first time only)
nuget restore Solutions\AGS.Editor.Full.sln

# Step 2: build AGS.Native (C++/CLI bridge, required by CommandLine)
& $msbuild Solutions\AGS.Editor.Full.sln /t:AGS_Native /p:Configuration=Release /p:Platform=Win32 /v:minimal

# Step 3: build the MCP server
& $msbuild Editor\AGS.CommandLine\AGS.CommandLine.csproj /p:Configuration=Release /p:Platform=AnyCPU /v:minimal
```

Output: `Editor\AGS.CommandLine\bin\x86\Release\AGS.CommandLine.exe`

### Configure in Kiro

Create or update `.kiro/settings/mcp.json` in your AGS game workspace:

```json
{
  "mcpServers": {
    "ags": {
      "command": "C:\\path\\to\\AGS.CommandLine.exe",
      "args": [],
      "disabled": false,
      "autoApprove": []
    }
  }
}
```

See [`docs/mcp.json.example`](docs/mcp.json.example) for a full annotated template.

### Use

Pull `#MCP_INDEX.md` (from `docs/steering/`) in a Kiro chat session to get the quick-reference tool list. From there, pull the specific steering files you need.

---

## Project Structure

```
AGS.CommandLine/
├── Program.cs              — stdin/stdout MCP loop
├── McpProtocol.cs          — JSON-RPC 2.0 dispatch
├── Tools/
│   ├── ProjectTools.cs     — project_info
│   ├── QueryTools.cs       — character_list, room_list, dialog_list, ...
│   ├── WriteTools.cs       — character_add, room_add, dialog_add, ...
│   ├── EventTools.cs       — event_bind_*, script_function_*, declare_variable
│   └── BuildTools.cs       — game_build_check, game_test_run, game_export
├── Services/
│   ├── GameLoader.cs       — loads Game.agf via AGS.Types
│   ├── GameSaver.cs        — saves Game.agf
│   ├── ScriptEditor.cs     — reads/writes .asc files
│   ├── NativeRoomLoader.cs — loads/saves .crm via AGS.Native (C++/CLI)
│   ├── BuildRunner.cs      — build check and export logic
│   └── ToolLocator.cs      — finds AGS Editor and acwin.exe
├── Resources/
│   └── blank_room.crm      — embedded template used when creating new rooms
└── docs/                   — documentation (this folder)
```

## Dependencies

| Assembly | Source |
|----------|--------|
| `AGS.Types.dll` | Built from `Editor/AGS.Types` (same solution) |
| `AGS.Native.dll` | Built from `Editor/AGS.Native` (C++/CLI, Win32 only) |
| `Newtonsoft.Json.dll` | NuGet — `Newtonsoft.Json 13.0.1` |

No GUI components. No irrKlang. Pure stdio MCP server.

---

## Key Constraints

- **Cannot compile scripts.** The AGS Editor must rebuild the project after any `.asc` changes.
- **Windows only** (inherits AGS.Native's Win32 constraint).
- **32-bit** (`Prefer32Bit = true`) to match AGS.Native's x86 build.

---

## Documentation

| File | Contents |
|------|----------|
| [`docs/BUILD_INSTRUCTIONS.md`](docs/BUILD_INSTRUCTIONS.md) | How to build from source |
| [`docs/mcp.json.example`](docs/mcp.json.example) | MCP configuration template |
| [`docs/steering/MCP_INDEX.md`](docs/steering/MCP_INDEX.md) | All tools at a glance + critical rules |
| [`docs/steering/MCP_TOOLS_READ.md`](docs/steering/MCP_TOOLS_READ.md) | Read tool parameters and return shapes |
| [`docs/steering/MCP_TOOLS_WRITE.md`](docs/steering/MCP_TOOLS_WRITE.md) | Write tool parameters, naming rules |
| [`docs/steering/MCP_TOOLS_EVENTS.md`](docs/steering/MCP_TOOLS_EVENTS.md) | Event names, event_bind_* tools, script tools |
| [`docs/steering/MCP_TOOLS_BUILD.md`](docs/steering/MCP_TOOLS_BUILD.md) | Build check, test run, export |
| [`docs/steering/MCP_WORKFLOWS.md`](docs/steering/MCP_WORKFLOWS.md) | Step-by-step game dev recipes |
| [`docs/steering/MCP_SCRIPTING.md`](docs/steering/MCP_SCRIPTING.md) | AGS Script syntax and patterns |
| [`docs/steering/MCP_TROUBLESHOOTING.md`](docs/steering/MCP_TROUBLESHOOTING.md) | Error messages → root cause → fix |
