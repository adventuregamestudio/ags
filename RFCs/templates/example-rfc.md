# RFC 0001: Engine Log Panel Integration for AGS Editor

- **RFC Number**: 0001
- **Title**: Engine Log Panel Integration for AGS Editor
- **Author(s)**: Erico Porto (@ericoporto)
- **Status**: Implemented
- **Type**: EDITOR AND ENGINE
- **Created**: 2023-05-08
- **AGS Version Target**: 3.6.1

## Summary

Add a log panel to the AGS Editor that displays real-time engine log output during game testing and debugging. This panel integrates engine logging directly into the Editor interface, providing developers with immediate access to game runtime information without requiring external tools or file monitoring.

## Motivation

Game developers frequently need to monitor log output during development and debugging. Currently, AGS logs are written to files or console output, requiring developers to switch between the Editor and external tools to view runtime information. This creates a fragmented debugging experience and slows down the development workflow.

### Problems Addressed

- Developers must monitor separate log files or console windows during debugging
- No integrated way to view engine log output within the Editor
- Difficult to correlate Editor actions with engine log messages
- Limited filtering and search capabilities for log analysis
- No persistent log viewing configuration across Editor sessions

### Success Criteria

- Log messages appear in real-time within the Editor during game testing
- Developers can filter log messages by level and category
- Log panel integrates seamlessly with Editor's docking system
- Minimal performance impact on both Editor and Engine
- Configuration persists across Editor sessions

## Detailed Design

### Overview

The log panel feature creates a bidirectional communication channel between the AGS Engine and Editor. The Engine sends log messages through a new debugger interface, while the Editor displays these messages in a dockable panel with filtering, search, and management capabilities.

### Component Impact

#### AGS Editor Changes

- **UI/UX Changes**: New dockable Log Panel with toolbar and rich text display
- **File Format Changes**: Log configuration stored in Game.agf.user file
- **Plugin System**: New DebugLogComponent for panel management
- **Build System**: Integration with existing debugger interface

#### AGS Engine Changes

- **Runtime Behavior**: New DebuggerLogOutputTarget sends messages to Editor
- **Scripting API**: No changes to script API - transparent logging
- **Platform Support**: Available on all platforms where Editor debugging is supported
- **Performance**: Minimal impact - only active during Editor debugging sessions
- **Save/Load**: No impact on game save/load functionality

#### Cross-Component Changes

- **Communication Protocol**: Extension of existing IDE debugger interface
- **Shared Data Structures**: Log message format and filtering parameters
- **Build Pipeline**: Coordinated logging initialization during game testing

### Technical Specification

#### API Changes

##### Engine API (C++)

```cpp
// Engine: New log output target
class DebuggerLogOutputTarget : public IOutputTarget {
public:
    void PrintMessage(const LogMessage &msg) override;
    void Initialize(const ConfigNode &config) override;
    void Shutdown() override;
};

// Command line argument support
// --log-debugger=OUTPUT_LEVEL
```

##### Editor API (C#)

```csharp
// Editor: New debug log component
public class DebugLogComponent : IEditorComponent {
    public void Initialize(AGSEditor editor);
    public void GameSettingsChanged();
    public void RefreshDataFromGame();
    // ... other IEditorComponent members
}

// Editor: Log panel dockable content
public class LogPanel : DockContent {
    public LogBuffer LogBuffer { get; }
    public LogFilterConfig FilterConfig { get; set; }
    public void UpdateLogDisplay();
    public void ClearLog();
}
```

##### AGS Script API

```cpp
// New LogLevel enum added in SCRIPT_API_v360
enum LogLevel {
  eLogAlert = 1,
  eLogFatal = 2,
  eLogError = 3,
  eLogWarn = 4,
  eLogInfo = 5,
  eLogDebug = 6
};

// New function added to System struct in SCRIPT_API_v360:
// builtin struct System {
//   import static void Log(LogLevel level, const string format, ...);
// }

// Usage examples:
System.Log(LogLevel.Info, "This message appears in Editor log panel");
System.Log(LogLevel.Warn, "Warning message with formatting: %d", someValue);
System.Log(LogLevel.Error, "Error occurred in function %s", functionName);

// All existing AbortGame calls with messages also appear in log panel
AbortGame("Game aborted - message appears in log panel");
```

**Script API Impact:**

- **New Functions**: `System.Log(LogLevel level, const string format, ...)` - added to System struct for structured logging
- **New Enums**: `LogLevel` enum with six severity levels (Alert, Fatal, Error, Warn, Info, Debug)
- **Modified Structs**: `System` struct extended with new `Log` function in SCRIPT_API_v360
- **Modified Functions**: Existing `AbortGame()` messages now also appear in Editor log panel
- **Deprecated Functions**: None
- **Breaking Changes**: None - fully backwards compatible
- **Backwards Compatibility**: All existing scripts work unchanged; new `System.Log` provides enhanced logging capabilities

#### Data Structures

```csharp
// Circular buffer for efficient log storage
public class LogBuffer {
    private readonly ConcurrentCircularBuffer<LogEntry> _buffer;
    public int Capacity { get; }
    public void AddEntry(LogEntry entry);
    public IEnumerable<LogEntry> GetEntries();
}

// Log filtering configuration
public class LogFilterConfig {
    public LogLevel MinLevel { get; set; }
    public Dictionary<LogGroup, LogLevel> GroupLevels { get; set; }
    public bool EnableTimestamps { get; set; }
}
```

#### File Format Changes

```xml
<!-- Game.agf.user - Log panel configuration -->
<GameSettings>
  <LogPanel>
    <MinLevel>Info</MinLevel>
    <GroupLevels>
      <Main>Debug</Main>
      <Game>Info</Game>
      <Script>Warn</Script>
    </GroupLevels>
    <AutoGlue>true</AutoGlue>
    <Font>Consolas</Font>
    <FontSize>9</FontSize>
  </LogPanel>
</GameSettings>
```

### Implementation Plan

#### Phase 1: Core Infrastructure

- [x] Create DebuggerLogOutputTarget in Engine
- [x] Implement basic message passing through debugger interface
- [x] Add LogBuffer with circular buffer implementation
- [x] Create basic LogPanel UI structure

#### Phase 2: Panel Integration

- [x] Integrate LogPanel as DockContent for window management
- [x] Add toolbar with Run, Pause, Clear functionality
- [x] Implement log filtering system
- [x] Add configuration persistence to Game.agf.user

#### Phase 3: Enhanced Features

- [x] Add "Glue" mode for automatic scrolling
- [x] Implement Copy and Copy All functionality
- [x] Add font and font size preferences
- [x] Optimize panel redrawing and text updates

### Testing Strategy

#### Unit Tests

- LogBuffer circular buffer functionality
- Log message filtering and level checking
- Configuration serialization/deserialization

#### Integration Tests

- Engine-to-Editor log message transmission
- Panel UI responsiveness during high-frequency logging
- Configuration persistence across Editor sessions

#### User Acceptance Tests

- Usability testing with game developers
- Performance impact during extended debugging sessions
- Integration with existing Editor workflow

## Backwards Compatibility

### Breaking Changes

None - this is an additive feature that doesn't affect existing functionality.

### Migration Path

Not applicable - purely additive feature. Existing projects work unchanged.

### Deprecation Timeline

Not applicable.

## Alternatives Considered

### Alternative 1: File-Based Log Monitoring

**Pros:**

- Simpler implementation
- No need for debugger interface changes
- Works with existing log files

**Cons:**

- File I/O overhead and locking issues
- Delayed log updates due to file buffering
- No real-time filtering capabilities
- Platform-specific file monitoring APIs

**Why not chosen:** Real-time integration provides better developer experience.

### Alternative 2: Separate Log Viewer Application

**Pros:**

- Independent development and deployment
- Could support multiple AGS versions
- Potentially more advanced log analysis features

**Cons:**

- Additional tool to install and manage
- No integration with Editor workflow
- Requires separate communication protocol
- Complex synchronization with Editor state

**Why not chosen:** Integrated solution provides seamless workflow.

### Alternative 3: Console Window Integration

**Pros:**

- Reuses existing console output infrastructure
- Familiar to developers from other environments

**Cons:**

- Platform-specific console handling
- Limited formatting and filtering options
- Cannot integrate with Editor's docking system
- Poor user experience on some platforms

**Why not chosen:** Dockable panel provides better integration and usability.

## Dependencies

### External Dependencies

- Windows Forms RichTextBox for log display
- .NET concurrent collections for thread-safe buffering

### Internal Dependencies

- Existing AGS Editor debugger interface
- Editor's docking panel system (WeifenLuo.WinFormsUI.Docking)
- Engine's logging system and IOutputTarget interface

### Platform Dependencies

- Windows: Full feature support
- Linux/macOS: Limited to platforms where Editor runs

## Security Considerations

- **Attack Vectors**: Log messages could potentially contain sensitive game data
- **Mitigation Strategies**: Only enabled during debugging sessions, not in release builds
- **Trust Boundaries**: Log panel has access to all engine log output during debugging

## Performance Impact

### Memory Usage

- Fixed-size circular buffer (configurable, default ~1000 entries)
- Approximately 100KB additional memory per active debugging session
- Automatic cleanup when buffer is full

### CPU Usage

- Minimal impact during normal operation
- Text rendering optimization to prevent UI freezing
- Background thread for log message processing

### Storage

- Configuration stored in Game.agf.user (few KB)
- No persistent log storage - runtime only

### Network

Not applicable - local communication only.

## Documentation Impact

### User Documentation

- New section in AGS Editor manual covering log panel usage
- Tutorial on debugging workflow with integrated logging
- Configuration options and filtering documentation

### Developer Documentation

- API documentation for DebuggerLogOutputTarget
- Integration guide for custom log output targets
- Panel customization and extension examples

### Examples and Tutorials

- Sample debugging scenarios using log panel
- Log filtering best practices
- Integration with existing debugging workflows

## Community Impact

### User Experience

Significantly improved debugging experience with integrated log viewing and real-time filtering.

### Learning Curve

Minimal - uses familiar logging concepts with intuitive panel interface.

### Ecosystem Impact

- Enables better debugging practices in AGS community
- May inspire additional Editor panel integrations
- Provides foundation for future debugging tools

## Open Questions

- ~~Should the log panel support custom color themes?~~ (Resolved: Uses Editor's existing theme system)
- ~~What should be the default buffer size for the circular buffer?~~ (Resolved: 1000 entries)
- ~~Should there be keyboard shortcuts for log panel operations?~~ (Resolved: Standard Editor shortcuts apply)

## Future Possibilities

This RFC enables future enhancements like:

- Advanced log search and filtering capabilities
- Log message bookmarking and annotations
- Export functionality for log analysis
- Integration with external debugging tools
- Performance profiling based on log timing data

## References

- [PR #1999: Editor: feature log panel](https://github.com/adventuregamestudio/ags/pull/1999)
- [Issue #1362: Read engine Log in the Editor](https://github.com/adventuregamestudio/ags/issues/1362)
- [Previous attempt PR #1518](https://github.com/adventuregamestudio/ags/pull/1518)
- [Previous attempt PR #1989](https://github.com/adventuregamestudio/ags/pull/1989)
- [WeifenLuo.WinFormsUI.Docking Documentation](https://github.com/dockpanelsuite/dockpanelsuite)

## Disclosures

### External References

- **Documentation**: AGS Editor documentation, WeifenLuo.WinFormsUI.Docking library documentation
- **Stack Overflow**: [C# RichTextBox performance optimization](https://stackoverflow.com/questions/1259355/how-to-prevent-flickering-in-listview-when-updating-a-single-listviewitems-text) - for UI optimization techniques
- **Other Sources**: Microsoft .NET Framework documentation for concurrent collections and WinForms components

### AI Assistance

- **AI Used**: Yes
- **Application/Service**: Claude (Anthropic)
- **Model Version**: Claude 3.5 Sonnet
- **Usage Description**: AI was used to help structure and write portions of this example RFC document, including technical sections, formatting, and documentation best practices. The AI analyzed the actual PR #1999 implementation details and AGS script definitions to create accurate technical content.
- **Human Review**: All AI-generated content was reviewed for technical accuracy against the actual implementation, with corrections made for API details, script function signatures, and implementation specifics.

*Note: This disclosure helps maintain transparency about the sources and methods used in creating this RFC, ensuring proper attribution and allowing reviewers to assess the content appropriately.*

---

## Revision History

| Date | Author | Changes |
|------|--------|---------|
| 2023-05-08 | Erico Porto | Initial draft based on previous attempts |
| 2023-05-25 | Erico Porto | Updated with DockContent implementation and panel integration |
| 2023-05-27 | Erico Porto | Final implementation notes and Wine compatibility fixes |

## Implementation Tracking

### Implementation Status

- [x] Design finalized
- [x] Core implementation
- [x] Testing complete
- [x] Documentation updated
- [x] Released

### Implementation Notes

Successfully implemented in AGS 3.6.1. The log panel integrates seamlessly with the Editor's docking system and provides real-time log viewing during game testing. Some minor Wine-specific issues were discovered but don't affect the core functionality on supported platforms.
