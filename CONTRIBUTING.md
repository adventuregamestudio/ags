# Contributing

To contribute to AGS project please follow the following guidelines.

## Issues

If you have any questions, problem reports, or suggestions for a change, please open an issue in our [Issue Tracker](https://github.com/adventuregamestudio/ags/issues). We recommend to first search the tracker for some common terms, or use Labels to filter issues thematically and see if there's any similar issues open already.

If you have a fix for a bug that you've found (or one reported in the tracker), then please open a pull request. How to do this is briefly explained in the [respective section](CONTRIBUTING.md#pull-requests) below.

Similarly, if you'd like to suggest a *minor* program change, then you also may create a pull request right away.

On the other hand, if you have an idea of a completely new feature, or a bigger change to our program, we ask to first submit an issue describing your proposal in detail. There we can check out and discuss the proposed changes with the development team to ensure these will be consistent with existing program behavior and future development plans. This will also save your time in case the proposal is not acceptable for us.

If there's an issue in the tracker that you would like to work on, please let us know that by commenting on it. This helps to avoid a duplicating effort.

## Pull Requests

If it is your first pull request and you don't know where to start we advise looking for online tutorials on using github and contributing to open source projects.
Following is an example of such resource: [How to Contribute to an Open Source Project on GitHub](https://egghead.io/courses/how-to-contribute-to-an-open-source-project-on-github)

To start, make a personal fork of our repository, and create your own feature/fix branch. Make sure that your branch is based on the correct branch of our repo, corresponding to the program version that you like to submit the changes to. See [Branch Organization and Releases](CONTRIBUTING.md#branch-organization-and-releases) section for details.

After you have finished creating your commits, send a pull request. We'll review your pull request and may ask you to make some changes to the code before we merge it.

## Use of AI Tooling

By "AI tooling" we mean large language models, diffusion models, and the agentic coding tools built on top of them - the kind of program that writes code, prose, or images on your behalf. This section is not about linters, static analysis, code completion drawing on your own project, or spell checkers.

These divide into two cases, and we treat them differently.

**Program code** written with the help of such tools is accepted, within the limits set out below. We neither encourage nor discourage this: a patch is judged on what it does and how well it is made, not on how it was typed. What we do care about is that a person stands behind it.

**Generated content is not accepted at all.** Artwork, audio and user-facing text that comes out of a diffusion model, an image or sound generator, or a language model asked to write prose has no place in AGS. This is not a matter of degree or of disclosure - please don't submit it.

### You are the author

Whatever you submit, you are its author and you are answerable for it. That means you understand the change, you can explain any part of it in your own words, and you have satisfied yourself that it is correct. If a reviewer asks why a particular line is there, the answer needs to come from you. Please don't pass our questions back to a model and relay its reply - a discussion in which nobody on either side knows what the code does is of no use to anyone.

This matters more here than it might elsewhere. As noted under [Further Information](CONTRIBUTING.md#further-information), large parts of AGS are old, and the ties between different parts of the program are not always obvious. Generated code tends to look confident and idiomatic while quietly missing that context, and that is usually where it goes wrong.

### Rules

Everything above is advice. The following are rules:

* **Contributions must be made by a person.** We do not accept pull requests opened by bots or agents, automated merges, or any arrangement that puts code into this repository without a human deciding to put it there.
* **Creative content must be human-authored.** Diffusion models and other generators of images, audio or prose are not to be used for anything that ships as part of AGS. That means artwork, sprites, icons, fonts, sound and music; it also means user interface design, the text of Editor labels, dialogs and messages, and documentation. If a person will see it or hear it, a person should have made it. We apply the same expectation to the material we distribute alongside AGS, such as the game templates and the demo game.
* **Scope and design are agreed in advance**, as described under [Issues](CONTRIBUTING.md#issues). A tool being able to produce a large change quickly is not a reason to submit one - if anything it makes prior discussion more important. Sweeping refactors, changes to the architecture of a program, or edits reaching across many files need an issue first.

### Experiments

Using these tools to explore an idea in your own fork is fine, and can be a good way to find out whether something is worth doing at all - a new backend or a rendering path, for instance.

Prototype code does not come upstream as it stands. Treat such an experiment as something you learned from rather than something you submit: rewrite it, test it, and put it forward on the same terms as any other contribution.

### Disclosure

If you used AI tooling to produce code in a pull request, please say so in the pull request description; the [pull request template](.github/pull_request_template.md) has a section for it.

We ask for this so that reviewers know where to look more carefully. It is not held against a contribution, and it does not lower the bar that contribution has to clear. Please keep the disclosure in the pull request description rather than in commit messages, and don't add tool attribution trailers to your commits - the history records who wrote the change, and that is you.

There is also an [AGENTS.md](AGENTS.md) in the repository root, which states these restrictions in a form that coding tools themselves can read.


## Branch Organization and Releases

The [`master`][master-br] branch is where the next planned version is being developed. It may temporarily contain unstable or untested code.

Currently, `master` corresponds to 3.\* generation of the engine/IDE and maintains backward compatibility with previous releases - see also [Compatibility](README.md#ags-game-compatibility). According to current plans, this branch should only receive improvements to the backend, system support, and performance. Changes to data formats and game scripts should be kept to a strict minimum to fill in the critical gaps in the engine's functionality.

There's an [`ags4`][ags4-br] branch also active where we develop a future version AGS 4.0. There we introduce greater changes and cut much of the old version support.

According to our plans, in the future `master` branch will be merged with `ags4`, while the backward compatible generation will remain as the `ags3` branch and only receive fixes and minor enhancements. But there's still some work to do in AGS 3.\*, so the exact moment that happens is unknown.

For "official" releases we create `release-X.X.X` branches, that is to prepare the code for the final release and continue making patches to that release if a need arises. 

Because of the low number of active developers we tend to only update the one latest release branch. If bugs are found in one of the older versions, then we advise you to update to the latest version first.

Please note that while the `master` branch may contain changes to game data format and new script functions, we cannot guarantee that these will remain unchanged until the actual release. We only support data formats and script APIs that are in published releases. For that reason, it's best to use one of the actual releases if you'd like to make your own game with this tool.

There may be other temporary development branches meant for preparing and testing large changes, but these are situational.

## Commits

What is said here is a recommendation, by following which you make the maintenance and development of this project easier.

Make separate commits with changes to different programs. For example: changes to the Editor as one commit, and changes to the Engine as another commit.

Don't mix different kind of changes: separate bug fixes, implementing new functionality, performance optimization, and code refactor in separate commits. When making bug fixes make separate commits for each separate bug fix (unless multiple bugs are tied to each other). Such separation lets us to cherry pick changes from one branch to another when necessary, and makes it much easier to understand the history of changes (which comes useful when fixing regressions as well). There's no precise measurement here, but if you feel that a single commit becomes too big, consider spliting it into multiple ones, for example - one commit per subtask of a larger task.

But the important rule is: a program that you modify must build at every commit you make.
(Sometimes a code is shared between different programs, in which case, while working on one program, you may ignore if other programs break, so long as you fix them later.)

*Commit description* is to be divided into the commit title and details. Commit title is the first line of description, and should give a clear and simple note about your change. We recommend keeping the title at *72 characters max*, as that's a traditional limit used by Git itself and most Git frontends when displaying a history of commits (if it's longer, then it's going to be truncated with "..." appended in the end). After you wrote this title, make two linebreaks and then write the full description as you see fit. The latter is optional, do this if you think that the changes deserve further explanation. It may be of any length.

Regarding commit title, we suggest to start it with a name of a program you are modifying, for example: "Engine: ", "Editor: " and so forth. Historically we used following prefixes, but we do not limit to these:
  * "Editor:" - changes to the editor program
  * "Engine:" - changes to the engine program
  * "Script API:" - collective changes implementing new game script command
  * "Compiler:" - changes to the game script compiler
  * "Tool:" (or name of the particular command line tool)
  * "Plugin:" (or name of the particular engine plugin)
  * (Name of a third party library) - changes to the library code which we have embedded in project
  * "ci: " - changes to continuous intergration scripts
  * "Makefile" - changes to Makefiles
  * "CMake" - changes to CMake scripts
  * "Readme", etc - changes to doc files (readme, changelog, and so forth)

Examples of commit titles:
  * "Engine: improve pathfinder's performance"
  * "Editor: fixed main menu not working properly"
  * "Script API: add Character.Jump()"

If your commit fixes or reverts changes made by a particular older commit, and you know which one, please mention that in description by pasting the older commit's hash (this helps to know which versions of the program have been affected by a bug).

## Testing

AGS has automated tests for the shared code, the engine, the script compiler and the command line tools, which are run through CTest, and a separate NUnit suite for the Editor. The platform build instructions linked from the [readme](README.md#building-and-running) cover how to build and run them.

Please check that the existing tests still pass before opening a pull request, and add tests for the behaviour you changed where the surrounding code makes that practical. This carries particular weight on `master`, which has to keep working with games made in every version of AGS since 2.50 - a test is often the only thing standing between a small fix and a regression in a fifteen year old game.

Not all of AGS is testable in its present state, and we don't expect anyone to restructure half the engine in order to add one test. Where a change cannot reasonably be covered, please say so in the pull request and describe how you verified it by hand instead.


## Licensing

AGS is released under the Artistic License 2.0; see [License.txt](License.txt). By submitting a contribution you confirm that you have the right to license it to us under those terms.

Please only submit code whose origin you can account for. Code taken from another project carries that project's license with it, and licenses imposing conditions we cannot meet - the GPL and LGPL among them - cannot be absorbed into AGS. Linking against such a library as a separate component is a different matter. Techniques, algorithms and general programming patterns are not the concern here; copying source is.

This applies to every contribution, however it was written. If you used a tool that may reproduce code from the material it was trained on, the responsibility for checking still rests with you.


## Further Information

Please be aware that big parts of the engine are still written in a old and often "dirty" code, and it may not be easy to understand ties between different program parts. Because there's a low number of active developers involved in this project our plans or design ideas are not always well documented, unfortunately. If you're in doubt - please discuss your ideas with us first.

For information on the project there's a [AGS Knowledge Base](https://github.com/adventuregamestudio/ags/wiki)

We've got a Coding Convention for the engine, please check it before writing the engine code: [github.com/adventuregamestudio/ags/wiki/AGS-Coding-Conventions-(Cpp)](https://github.com/adventuregamestudio/ags/wiki/AGS-Coding-Conventions-(Cpp))


[master-br]: https://github.com/adventuregamestudio/ags/tree/master
[ags4-br]: https://github.com/adventuregamestudio/ags/tree/ags4
