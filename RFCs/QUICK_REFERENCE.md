# AGS RFC Quick Reference

## TL;DR - How to Submit an RFC

1. **Copy the template**: `cp RFCs/templates/rfc-template.md RFCs/drafts/0000-my-feature.md`
2. **Fill it out**: Replace placeholder text with your proposal details
3. **Choose component type**: Mark as `[EDITOR]`, `[ENGINE]`, `[EDITOR AND ENGINE]`, `[INFRASTRUCTURE]`, or `[DOCUMENTATION]`
4. **Submit PR**: Open a pull request with your RFC
5. **Iterate**: Respond to feedback and refine your proposal

## RFC Types & Examples

| Type | When to Use | Example |
|------|-------------|---------|
| `[EDITOR]` | Changes to AGS Editor interface, tools, or functionality | New room editor features, plugin system changes |
| `[ENGINE]` | Changes to AGS runtime, scripting, or game execution | New script functions, graphics features, platform support |
| `[EDITOR AND ENGINE]` | Changes affecting both Editor and Engine | File format changes, cross-component architecture |
| `[INFRASTRUCTURE]` | Build system, CI/CD, project structure | CMake changes, new deployment processes |
| `[DOCUMENTATION]` | Major documentation initiatives | API documentation overhaul, tutorial series |

## Common Mistakes to Avoid

❌ **Too vague**: "Make AGS better"  
✅ **Specific**: "Add real-time lighting system to AGS Engine"

❌ **No motivation**: Jumping straight to technical details  
✅ **Clear motivation**: Explain the problem you're solving

❌ **Missing alternatives**: Only one solution considered  
✅ **Consider alternatives**: Compare different approaches

❌ **No backwards compatibility plan**: Breaking changes without migration path  
✅ **Migration strategy**: How existing users adapt to changes

## Template Checklist

### Required Sections

- [ ] **Summary**: One paragraph overview
- [ ] **Motivation**: Why this change is needed
- [ ] **Detailed Design**: Technical specification
- [ ] **Component Impact**: Editor/Engine changes
- [ ] **Backwards Compatibility**: Breaking changes and migration
- [ ] **Alternatives Considered**: Other approaches evaluated

### Recommended Sections

- [ ] **Implementation Plan**: Phased development approach
- [ ] **Testing Strategy**: How to verify correctness
- [ ] **Performance Impact**: Resource consumption changes
- [ ] **Security Considerations**: Security implications
- [ ] **Community Impact**: Effect on users and ecosystem
- [ ] **Disclosures**: External references, Stack Overflow answers, AI assistance used

## Review Process

```txt
Draft → Active → FCP → Accepted → Implemented
  ↓       ↓       ↓       ↓
Rejected/Postponed (any stage)
```

### Status Meanings

- **Draft**: Initial submission, gathering feedback
- **Active**: Under active discussion and refinement
- **FCP**: Final Comment Period - last chance for feedback
- **Accepted**: Approved for implementation
- **Implemented**: Successfully implemented and deployed
- **Rejected**: Not moving forward
- **Postponed**: Valid but not current priority

## File Organization

```txt
RFCs/
├── drafts/           # Your RFC starts here
├── active/           # Under discussion
├── accepted/         # Approved, awaiting implementation
├── implemented/      # Completed RFCs
│   ├── editor/      # Editor-specific
│   └── engine/      # Engine-specific
├── rejected/         # Not accepted
└── postponed/       # Future consideration
```

## Writing Tips

### Structure Your Argument

1. **Problem Statement**: What's broken or missing?
2. **Solution Overview**: High-level approach
3. **Technical Details**: Implementation specifics
4. **Trade-offs**: Why this approach vs alternatives
5. **Migration**: How users adapt to changes

### Code Examples

Include code snippets for:

- API changes (C++, C#, AGS Script)
- Data structure modifications
- File format changes
- Configuration examples

### Be Specific

- Use concrete examples over abstract descriptions
- Include measurable success criteria
- Specify version targets and timelines
- Consider edge cases and error scenarios

## Common RFC Patterns

### New Feature RFCs

Focus on: User scenarios, API design, integration points

### Performance RFCs

Focus on: Benchmarks, metrics, trade-offs, profiling

### Breaking Change RFCs

Focus on: Migration path, deprecation timeline, tooling support

### Architecture RFCs

Focus on: System design, component interactions, future extensibility

## Getting Help

- **Template Questions**: Check `RFCs/templates/example-rfc.md`
- **Process Questions**: See main `RFCs/README.md`
- **Technical Questions**: Open GitHub issue or discuss with maintainers
- **Community Discussion**: Use AGS forums or Discord

## Quick Start Commands

```bash
# Create new RFC
cp RFCs/templates/rfc-template.md RFCs/drafts/0000-my-feature.md

# Edit your RFC
$EDITOR RFCs/drafts/0000-my-feature.md

# Check against example
diff RFCs/templates/example-rfc.md RFCs/drafts/0000-my-feature.md
```

Remember: RFCs are conversations, not demands. Be open to feedback and iteration!
