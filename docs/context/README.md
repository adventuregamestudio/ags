# docs/context

Project knowledge for coding agents. Entry point: `/AGENTS.md`. Policy: `/CONTRIBUTING.md`.

| File | Load when |
|---|---|
| `repository.md` | locating code; branch, build and test facts; what must not break; where a change goes |
| `workflows.md` | building, testing, documenting, submitting |
| `decisions.md` | designing; checking whether an idea is settled, positioned or rejected |
| `roadmap.md` | state of 3.x and 4.0; Beta checklist; open PRs; proposals; parked ideas |
| `sources.md` | who said what; where to search; fetch commands; reading list |

Editing rules:

- every claim: source (issue/PR number, wiki page, forum board + title, or `path@commit`) and date
- status words only: **decided**, **in code**, **proposed**, **open**, **rejected**; a maintainer's design is "proposed" until merged
- shipped → one line "in code" (details are in `Changes.txt`); rejected → one line under "Parked", with the maintainer's reason; no entry without a reason
- verify paths with `git grep`/`ls` on the branch described; note `master` vs `ags4` differences
- ≤3k tokens per file; snapshot date at the top of each file
- refresh: `roadmap.md` at each release or milestone change; `decisions.md` when a maintainer settles something; `repository.md` when layout or build systems move
- these files describe; `CONTRIBUTING.md` decides
