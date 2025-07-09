# RFC NNNN: [Title]

- **RFC Number**: NNNN
- **Title**: [Brief, descriptive title]
- **Author(s)**: [Your name and contact info]
- **Status**: Draft
- **Type**: [EDITOR | ENGINE | EDITOR AND ENGINE | INFRASTRUCTURE | DOCUMENTATION]
- **Created**: [YYYY-MM-DD]
- **AGS Version Target**: [e.g., 4.x, 3.x]

## Summary

[One paragraph explanation of the feature or change being proposed.]

## Motivation

[Why are we doing this? What use cases does it support? What is the expected outcome?]

### Problems Addressed

- [Problem 1]
- [Problem 2]
- [Problem 3]

### Success Criteria

- [Measurable success criterion 1]
- [Measurable success criterion 2]

## Detailed Design

### Overview

[High-level architectural overview of the proposed changes.]

### Component Impact

#### AGS Editor Changes

[If applicable, describe changes to the Editor]

- **UI/UX Changes**: [Describe any user interface modifications]
- **File Format Changes**: [Any new or modified file formats]
- **Plugin System**: [Impact on plugin architecture]
- **Build System**: [Changes to compilation or packaging]

#### AGS Engine Changes

[If applicable, describe changes to the Engine]

- **Runtime Behavior**: [How game execution is affected]
- **Scripting API**: [New or modified script functions/properties]
- **Platform Support**: [Platform-specific considerations]
- **Performance**: [Expected performance impact]
- **Save/Load**: [Impact on game state persistence]

#### Cross-Component Changes

[If applicable, describe changes that affect both Editor and Engine]

- **Communication Protocol**: [How Editor and Engine interact]
- **Shared Data Structures**: [Common formats or APIs]
- **Build Pipeline**: [Integration between components]

### Technical Specification

#### API Changes

##### Engine API (C++)

```cpp
// Example C++ API changes for Engine
class NewFeature {
public:
    void DoSomething(int param);
    int GetValue() const;
};
```

##### Editor API (C#)

```csharp
// Example C# API changes for Editor
public class EditorFeature
{
    public void ProcessData(string input);
    public bool IsEnabled { get; set; }
}
```

##### AGS Script API

```cpp
builtin struct System {
  import void Example(int param);
}
```

**Script API Impact:**

- **New Functions**: List any new script functions added
- **Modified Functions**: List functions with changed behavior or parameters
- **Deprecated Functions**: List functions being deprecated
- **Breaking Changes**: List any script-breaking changes
- **Backwards Compatibility**: How existing scripts are affected

#### Data Structures

[Define new data structures or modifications to existing ones]

#### File Format Changes

[If applicable, document changes to .agf, .crm, or other file formats]

```json
{
  "newField": "value",
  "existingField": "modified_structure"
}
```

### Implementation Plan

#### Phase 1: [Foundation]

- [ ] Task 1
- [ ] Task 2
- [ ] Task 3

#### Phase 2: [Core Implementation]

- [ ] Task 1
- [ ] Task 2

#### Phase 3: [Integration & Polish]

- [ ] Task 1
- [ ] Task 2

### Testing Strategy

#### Unit Tests

- [Describe unit testing approach]

#### Integration Tests

- [Describe integration testing approach]

#### User Acceptance Tests

- [Describe user testing scenarios]

#### Regression Tests

- [Describe backwards compatibility testing]

## Backwards Compatibility

### Breaking Changes

[List any breaking changes and their justification]

### Migration Path

[Describe how existing projects/users can migrate]

### Deprecation Timeline

[If applicable, outline deprecation schedule]

## Alternatives Considered

### Alternative 1: [Name]

**Pros:**

- [Advantage 1]
- [Advantage 2]

**Cons:**

- [Disadvantage 1]
- [Disadvantage 2]

**Why not chosen:** [Explanation]

### Alternative 2: [Name]

[Similar structure as above]

### Do Nothing

**Pros:**

- [Advantage of status quo]

**Cons:**

- [Problems that remain unsolved]

## Dependencies

### External Dependencies

- [New libraries or tools required]

### Internal Dependencies

- [Dependencies on other AGS components or RFCs]

### Platform Dependencies

- [Platform-specific requirements]

## Security Considerations

[Analyze any security implications of the proposed changes]

- **Attack Vectors**: [Potential security risks]
- **Mitigation Strategies**: [How risks are addressed]
- **Trust Boundaries**: [Changes to security model]

## Performance Impact

### Memory Usage

[Expected impact on memory consumption]

### CPU Usage

[Expected impact on processing performance]

### Storage

[Expected impact on file sizes or disk usage]

### Network

[If applicable, network performance considerations]

## Documentation Impact

### User Documentation

- [Documentation that needs to be updated]

### Developer Documentation

- [API documentation updates]

### Examples and Tutorials

- [New examples or tutorial updates needed]

## Community Impact

### User Experience

[How this affects game developers using AGS]

### Learning Curve

[Complexity added or removed for users]

### Ecosystem Impact

[Effect on plugins, tools, or community resources]

## Open Questions

- [Unresolved question 1]
- [Unresolved question 2]
- [Decision point that needs community input]

## Future Possibilities

[What this RFC makes possible in the future, without committing to implementation]

## References

- [Link to relevant discussions]
- [Related RFCs or issues]
- [External documentation or standards]

## Disclosures

### External References

- **Documentation**: [List any official documentation, manuals, or specifications consulted]
- **Stack Overflow**: [List any Stack Overflow questions/answers referenced, with links]
- **Other Sources**: [Books, articles, blog posts, or other materials referenced]

### AI Assistance

- **AI Used**: [Yes/No - if Yes, complete the following]
- **Application/Service**: [e.g., ChatGPT, Claude, GitHub Copilot, etc.]
- **Model Version**: [e.g., GPT-4, Claude 3.5 Sonnet, etc.]
- **Usage Description**: [How AI was used - e.g., "Used for initial draft writing", "Code review and optimization suggestions", "Research assistance", "Technical explanation clarification", etc.]
- **Human Review**: [Describe how AI-generated content was reviewed, validated, and modified by human authors]

*Note: This disclosure helps maintain transparency about the sources and methods used in creating this RFC, ensuring proper attribution and allowing reviewers to assess the content appropriately.*

---

## Revision History

| Date | Author | Changes |
|------|--------|---------|
| YYYY-MM-DD | [Author] | Initial draft |

## Implementation Tracking

[This section is updated during implementation]

### Implementation Status

- [ ] Design finalized
- [ ] Core implementation
- [ ] Testing complete
- [ ] Documentation updated
- [ ] Released

### Implementation Notes

[Notes about implementation decisions that differ from the RFC]
