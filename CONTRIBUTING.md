# Contributing
To contribute to AGS please follow the following guidelines

# Code of Conduct
* Examples of acceptable behavior
  - Using welcoming and respectful language
  - Gracefully accepting constructive criticism
  - Being respectful of differing viewpoints

* Examples of unacceptable behavior
  - Sexualized language or imagery
  - Insulting or derogatory comments
  - Harassment
  - Other conduct that would reasonably be considered inappropriate in a professional setting

## Our Responsibility and Enforcement
Project maintainers are responsible for clarifying the standards of acceptable behavior and will take appropriate and fair actions in response to unacceptable behavior.
Project maintainers have the right to remove, edit, or reject comments, commits, code, or other contributions that are made in bad faith. Temporary or permanent bans may result from inappropriate, threatening, or offensive conduct.

Concerns and reports can be made to ((Insert appropriate contact info here))

## Attribution
This Code of Conduct is partly adapted from the [Contributor Covenant](https://www.contributor-covenant.org/version/1/4/code-of-conduct/) and is subject to change

## Branch Organization and Releases
The [`master`][master-br] branch is where the next planned version is being developed. It may contain unstable or untested code.

Currently, `master` corresponds to 3.\* generation of the engine/IDE and maintains backward compatibility with previous releases - see also [Compatibility](#ags-game-compatibility). According to current plans, this branch should only receive improvements to the backend, system support, and performance. Changes to data formats and game scripts should be kept to a strict minimum to fill in the critical gaps in the engine's functionality.

There's an [`ags4`][ags4-br] branch also active where we develop a future version currently named simply "ags4". There we introduce greater changes and cut much of the old version support.

According to our plans, in the future `master` branch will be merged with `ags4`, while the backward compatible generation will remain as the `ags3` branch and only receive fixes and minor enhancements. But there's still some work to do in AGS 3.\*, so the exact moment that happens is unknown.

For "official" releases we create `release-X.X.X` branches, that is to prepare the code for the final release and continue making patches to that release if a need arises. 

Because of the low number of active developers we tend to only update the one latest release branch. If bugs are found in one of the older versions, then we advise you to update to the latest version first.

Please note that while the `master` branch may contain changes to game data format and new script functions, we cannot guarantee that these will remain unchanged until the actual release. We only support data formats and script APIs that are in published releases. For that reason, it's best to use one of the actual releases if you'd like to make your own game with this tool.

There may be other temporary development branches meant for preparing and testing large changes, but these are situational.

# Bugs and Issues
If you find a critical bug or issue with the current release and have a fix for it, please make a pull request. How to do this is discussed below in this guide. If you do not have a fix, please submit an issue in the [Issue Tracker](https://github.com/adventuregamestudio/ags/issues) so that we are made aware of the problem and can take steps to fix it.

# Feature Requests and Program Changes
If you would like a particular feature or change, please submit an issue with the [Issue Tracker](https://github.com/adventuregamestudio/ags/issues) so we can talk about the proposed change.

# Issues
If you have an issue in the tracker that you would like to work on, please let us know that you are working on the issue by commenting on it. This includes Issues you have created. Please tell us so that no one is duplicating effort.

# Pull Requests
If it is your first pull request and you don't know where to start [contributing to an Open Source Project](https://egghead.io/courses/how-to-contribute-to-an-open-source-project-on-github) is a great resource for first-time contributors.

To start, make a fork of the branch you are working on and create your own feature/fix branch. After you have finished your commits, please send a pull request. We'll review your pull request and may ask you to make some changes to the code before we merge it.
For information on the project [AGS Info and Knowledge Base](https://github.com/adventuregamestudio/ags/wiki) has information on the history and current state of the project.

Information on our coding conventions can be found at [AGS Coding Conventions](https://github.com/adventuregamestudio/ags/wiki/AGS-Coding-Conventions-(Cpp))
Please be aware that many parts of the engine are still written in very old and often "dirty" code, and it may not be easy to understand the ties between different program parts. If you are having problems and can't find the needed information in the [AGS Info and Knowledge Base](https://github.com/adventuregamestudio/ags/wiki), please reach out to us, and we may be able to help.
