# AGS Request for Comments (RFC) Process

## What is an RFC?

Request for Comments (RFC) is a process for proposing major changes to the AGS (Adventure Game Studio) project. RFCs provide a consistent and controlled path for new features and significant changes to enter the project, so that all stakeholders can be confident about the direction AGS is evolving in.

## When to Submit an RFC

You should consider writing an RFC if you intend to make "substantial" changes to AGS or its documentation. What constitutes a "substantial" change is evolving based on community norms and varies depending on what part of the ecosystem you are proposing to change, but may include the following:

### AGS Editor RFCs

- New major features or significant changes to the Editor UI/UX
- Changes to the Editor's plugin system
- New file formats or major changes to existing formats
- Significant changes to the build system or project structure
- Major refactoring of Editor architecture

### AGS Engine RFCs

- New runtime features that affect game behavior
- Changes to the scripting language syntax or semantics
- New platform support or significant platform-specific changes
- Performance improvements that change APIs or behavior
- Changes to the save/load system or compatibility
- New graphics, audio, or input features

### General AGS RFCs

- Cross-cutting changes that affect both Editor and Engine
- Changes to development processes or project governance
- Major dependency updates or architectural decisions

## RFC Lifecycle

### 1. Pre-RFC Discussion

Before submitting an RFC, consider discussing your idea in:

- GitHub Issues for initial feedback
- Community forums or Discord channels
- Direct discussion with maintainers

### 2. RFC Submission

1. Fork the AGS repository
2. Copy `RFCs/templates/rfc-template.md` to `RFCs/drafts/0000-my-feature.md`
3. Fill in the RFC template with your proposal
4. Submit a pull request

### 3. RFC Review Process

- **Draft**: Initial submission and community feedback
- **Active**: RFC is being actively discussed and refined
- **Final Comment Period (FCP)**: Last chance for feedback before decision
- **Accepted**: RFC is approved for implementation
- **Implemented**: RFC has been successfully implemented
- **Rejected**: RFC is not moving forward
- **Postponed**: RFC is valid but not prioritized currently

### 4. Implementation

- Accepted RFCs move to implementation phase
- Implementation details may evolve during development
- RFCs are moved to appropriate folders based on status

## Directory Structure

```sh
RFCs/
├── README.md                 # This file
├── templates/
│   └── rfc-template.md      # RFC template
├── drafts/                  # RFCs under initial development
├── active/                  # RFCs under active discussion
├── accepted/                # Accepted RFCs awaiting implementation
├── implemented/             # Successfully implemented RFCs
│   ├── editor/             # Editor-specific implementations
│   └── engine/             # Engine-specific implementations
├── rejected/                # RFCs that were not accepted
└── postponed/              # RFCs postponed for future consideration
```

## RFC Numbering

RFCs are numbered sequentially starting from 0001. The number is assigned when the RFC is first created. The format is:

- `NNNN-brief-description.md` (e.g., `0001-new-scripting-features.md`)

## Component Classification

Each RFC must clearly identify which components it affects:

- **`[EDITOR]`**: Changes primarily affecting the AGS Editor
- **`[ENGINE]`**: Changes primarily affecting the AGS Engine/Runtime
- **`[BOTH]`**: Changes affecting both Editor and Engine
- **`[INFRASTRUCTURE]`**: Changes to build systems, CI/CD, or project structure
- **`[DOCUMENTATION]`**: Significant documentation changes or proposals

## Quality Guidelines

### Writing Style

- Be clear and concise
- Use proper grammar and spelling
- Include code examples where appropriate
- Consider multiple perspectives and use cases

### Technical Depth

- Provide sufficient detail for implementation
- Consider backwards compatibility
- Address performance implications
- Include testing strategies

### Community Consideration

- Acknowledge alternative approaches
- Consider impact on existing users
- Be open to feedback and iteration

## Decision Making

- RFCs require consensus from core maintainers
- Community feedback is highly valued and considered
- Technical merit and project alignment are key factors
- Backwards compatibility is a strong consideration

## FAQ

### How long does the RFC process take?

The process varies based on complexity and consensus. Simple RFCs might be resolved in weeks, while complex architectural changes could take months.

### Can I implement before RFC acceptance?

You can create proof-of-concept implementations to support your RFC, but substantial implementation work should wait for acceptance.

### What if my RFC is rejected?

Rejection doesn't mean the idea is bad. Common reasons include timing, resource constraints, or need for more development. You can revise and resubmit.

### How do I get feedback on my RFC?

- Comment on the RFC pull request
- Engage in community forums
- Reach out to relevant maintainers
- Present at community meetings if available

## Contributing

We encourage participation from all community members. Whether you're proposing changes, reviewing RFCs, or providing feedback, your involvement helps shape AGS's future.

---

For questions about the RFC process, please open an issue or reach out to the maintainers.
